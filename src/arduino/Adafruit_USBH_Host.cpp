/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022, Ha Thach (tinyusb.org) for Adafruit
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// ESP32 out-of-sync
#ifdef ARDUINO_ARCH_ESP32
#include "arduino/ports/esp32/tusb_config_esp32.h"
#define MSBFIRST SPI_MSBFIRST
#endif

#include "tusb_option.h"

#if CFG_TUH_ENABLED

#include "Adafruit_TinyUSB_API.h"
#include "Adafruit_USBH_Host.h"

#if defined(CFG_TUH_MAX3421) && CFG_TUH_MAX3421
static void max3421_isr(void);
#endif

Adafruit_USBH_Host *Adafruit_USBH_Host::_instance = NULL;

Adafruit_USBH_Host::Adafruit_USBH_Host(void) {
  Adafruit_USBH_Host::_instance = this;
  _spi = NULL;
  _cs = _intr = -1;
}

Adafruit_USBH_Host::Adafruit_USBH_Host(SPIClass *spi, int8_t cs, int8_t intr) {
  Adafruit_USBH_Host::_instance = this;
  _spi = spi;
  _cs = cs;
  _intr = intr;
}

bool Adafruit_USBH_Host::configure(uint8_t rhport, uint32_t cfg_id,
                                   const void *cfg_param) {
  return tuh_configure(rhport, cfg_id, cfg_param);
}

#ifdef ARDUINO_ARCH_RP2040
bool Adafruit_USBH_Host::configure_pio_usb(uint8_t rhport,
                                           const void *cfg_param) {
  return configure(rhport, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, cfg_param);
}
#endif

bool Adafruit_USBH_Host::begin(uint8_t rhport) {
#if defined(CFG_TUH_MAX3421) && CFG_TUH_MAX3421
  if (_spi == NULL || _intr < 0 || _cs < 0) {
    return false;
  }

  // CS pin
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  // SPI init
  SPI.begin();

  // Interrupt pin
  pinMode(_intr, INPUT_PULLUP);
  attachInterrupt(_intr, max3421_isr, FALLING);
#endif

  return tuh_init(rhport);
}

void Adafruit_USBH_Host::task(void) { tuh_task(); }

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
TU_ATTR_WEAK void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                                   uint8_t const *desc_report,
                                   uint16_t desc_len) {
  (void)dev_addr;
  (void)instance;
  (void)desc_report;
  (void)desc_len;
}

// Invoked when device with hid interface is un-mounted
TU_ATTR_WEAK void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  (void)dev_addr;
  (void)instance;
}

// Invoked when received report from device via interrupt endpoint
TU_ATTR_WEAK void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                             uint8_t const *report,
                                             uint16_t len) {
  (void)dev_addr;
  (void)instance;
  (void)report;
  (void)len;
}

//--------------------------------------------------------------------+
// USB Host using MAX3421E
//--------------------------------------------------------------------+
#if CFG_TUH_ENABLED && defined(CFG_TUH_MAX3421) && CFG_TUH_MAX3421

#if defined(ARDUINO_ARCH_ESP32) && !defined(PLATFORMIO)
extern "C" void hcd_int_handler_esp32(uint8_t rhport, bool in_isr);
#endif

static void max3421_isr(void) {
  // ESP32 out-of-sync
#if defined(ARDUINO_ARCH_ESP32)
#if defined(PLATFORMIO)
  tuh_int_handler(1, false);
#else
  hcd_int_handler_esp32(1, false);
#endif
#else
  tuh_int_handler(1, true);
#endif
}

extern "C" {

void tuh_max3421_spi_cs_api(uint8_t rhport, bool active) {
  (void)rhport;
  if (!Adafruit_USBH_Host::_instance) {
    return;
  }

  digitalWrite(Adafruit_USBH_Host::_instance->_cs, active ? LOW : HIGH);
}

bool tuh_max3421_spi_xfer_api(uint8_t rhport, uint8_t const *tx_buf,
                              size_t tx_len, uint8_t *rx_buf, size_t rx_len) {
  (void)rhport;

  if (!Adafruit_USBH_Host::_instance) {
    return false;
  }
  Adafruit_USBH_Host *host = Adafruit_USBH_Host::_instance;

  // MAX3421e max clock is 26MHz
  // Depending on mcu ports, it may need to be clipped down
#ifdef ARDUINO_ARCH_SAMD
  // SAMD 21/51 can only work reliably at 12MHz
  uint32_t const max_clock = 12000000ul;
#else
  uint32_t const max_clock = 26000000ul;
#endif

  SPISettings config(max_clock, MSBFIRST, SPI_MODE0);
  host->_spi->beginTransaction(config);

  // TODO improve spi speed with burst transfer
  for (size_t count = 0; count < tx_len || count < rx_len; count++) {
    uint8_t data = 0x00;

    // TX
    if (count < tx_len) {
      data = tx_buf[count];
    }
    data = host->_spi->transfer(data);

    // RX
    if (count < rx_len) {
      rx_buf[count] = data;
    }
  }

  host->_spi->endTransaction();
  return true;
}

void tuh_max3421_int_api(uint8_t rhport, bool enabled) {
  (void)rhport;

  if (!Adafruit_USBH_Host::_instance) {
    return;
  }
  Adafruit_USBH_Host *host = Adafruit_USBH_Host::_instance;
  (void)host;

#ifdef ARDUINO_ARCH_SAMD
  //--- SAMD51 ---//
#ifdef __SAMD51__
  const IRQn_Type irq =
      (IRQn_Type)(EIC_0_IRQn + g_APinDescription[host->_intr].ulExtInt);

  if (enabled) {
    NVIC_EnableIRQ(irq);
  } else {
    NVIC_DisableIRQ(irq);
  }
#else
  //--- SAMD21 ---//
  if (enabled) {
    NVIC_EnableIRQ(EIC_IRQn);
  } else {
    NVIC_DisableIRQ(EIC_IRQn);
  }
#endif

#elif defined(ARDUINO_NRF52_ADAFRUIT)
  //--- nRF52 ---//
  if (enabled) {
    NVIC_EnableIRQ(GPIOTE_IRQn);
  } else {
    NVIC_DisableIRQ(GPIOTE_IRQn);
  }

#elif defined(ARDUINO_ARCH_ESP32)
  //--- ESP32 ---//
  if (enabled) {
    gpio_intr_enable((gpio_num_t)host->_intr);
  } else {
    gpio_intr_disable((gpio_num_t)host->_intr);
  }
#else
#error "MAX3421e host is not Unsupported by this architecture"
#endif
}
}
#endif

#endif
