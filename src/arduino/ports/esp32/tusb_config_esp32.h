/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018, hathach for Adafruit
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

#ifndef TUSB_CONFIG_ESP32_H_
#define TUSB_CONFIG_ESP32_H_

#ifdef __cplusplus
extern "C" {
#endif

// ESP32 Arduino core already integrated tinyusb using lib arduino_tinyusb
//    https://github.com/espressif/esp32-arduino-lib-builder/tree/master/components/arduino_tinyusb
// Although it is possible to use .c files in this library for linking instead
// of lib arduino_tinyusb. However, include path of lib arduino_tinyusb is first
// in order. Therefore, changes in this Adafruit TinyUSB library's header are
// not picked up by its .c file. Due to the version difference between the 2
// libraries, this file is used to make it compatible with ESP32 Arduino core.

// This file also contains additional configuration for EPS32 in addition to
// tools/esp32-arduino-libs/esp32xx/include/arduino_tinyusb/include/tusb_config.h

//--------------------------------------------------------------------+
// ESP32 out-of-sync
//--------------------------------------------------------------------+
#include "esp_arduino_version.h"
#include "sdkconfig.h"

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(2, 0, 8)
#error "ESP32 Arduino core version 2.0.8 or later is required"
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
// Note: it is possible to use tinyusb + max3421e as host controller
// with no OTG USB MCU such as eps32, c3 etc...
//--------------------------------------------------------------------

#if CONFIG_IDF_TARGET_ESP32S2
#define CFG_TUSB_MCU OPT_MCU_ESP32S2
#elif CONFIG_IDF_TARGET_ESP32S3
#define CFG_TUSB_MCU OPT_MCU_ESP32S3
#elif CONFIG_IDF_TARGET_ESP32P4
#define CFG_TUSB_MCU OPT_MCU_ESP32P4
#else
#define CFG_TUSB_MCU OPT_MCU_ESP32
#endif

#if CONFIG_IDF_TARGET_ESP32P4
#define CFG_TUD_MAX_SPEED OPT_MODE_HIGH_SPEED
#else
#define CFG_TUD_MAX_SPEED OPT_MODE_FULL_SPEED
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_FREERTOS
// clang-format off
#define CFG_TUSB_OS_INC_PATH freertos/
// clang-format on
#endif

#ifndef CFG_TUD_LOG_LEVEL
#define CFG_TUD_LOG_LEVEL 2
#endif

#ifndef CFG_TUH_LOG_LEVEL
#define CFG_TUH_LOG_LEVEL 2
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

// For selectively disable device log (when > CFG_TUSB_DEBUG)
// #define CFG_TUD_LOG_LEVEL 3

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN TU_ATTR_ALIGNED(4)

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
// only enabled if USB OTG is supported
//--------------------------------------------------------------------
#ifndef CFG_TUD_ENABLED
  #if defined(CONFIG_USB_OTG_SUPPORTED) && CONFIG_USB_OTG_SUPPORTED
  #define CFG_TUD_ENABLED 1
  #else
  #define CFG_TUD_ENABLED 0
  #endif
#endif

#define CFG_TUD_CDC 2
#define CFG_TUD_MSC 1
#define CFG_TUD_HID 2
#define CFG_TUD_MIDI 1
#define CFG_TUD_VENDOR 1
#define CFG_TUD_VIDEO 1
#define CFG_TUD_VIDEO_STREAMING 1

// video streaming endpoint buffer size
#define CFG_TUD_VIDEO_STREAMING_EP_BUFSIZE                                     \
  CONFIG_TINYUSB_VIDEO_STREAMING_BUFSIZE

// CDC FIFO size of TX and RX
#define CFG_TUD_CDC_RX_BUFSIZE CONFIG_TINYUSB_CDC_RX_BUFSIZE
#define CFG_TUD_CDC_TX_BUFSIZE CONFIG_TINYUSB_CDC_TX_BUFSIZE

// MSC Buffer size of Device Mass storage
#define CFG_TUD_MSC_EP_BUFSIZE CONFIG_TINYUSB_MSC_BUFSIZE

// HID buffer size Should be sufficient to hold ID (if any) + Data
#define CFG_TUD_HID_BUFSIZE CONFIG_TINYUSB_HID_BUFSIZE

// MIDI FIFO size of TX and RX
#define CFG_TUD_MIDI_RX_BUFSIZE CONFIG_TINYUSB_MIDI_RX_BUFSIZE
#define CFG_TUD_MIDI_TX_BUFSIZE CONFIG_TINYUSB_MIDI_TX_BUFSIZE

// Vendor FIFO size of TX and RX
#define CFG_TUD_VENDOR_RX_BUFSIZE CONFIG_TINYUSB_VENDOR_RX_BUFSIZE
#define CFG_TUD_VENDOR_TX_BUFSIZE CONFIG_TINYUSB_VENDOR_TX_BUFSIZE

//--------------------------------------------------------------------
// Host Configuration
//--------------------------------------------------------------------

// Enable host stack using native OTG (not MAX3421E)
#define CFG_TUH_ENABLED 1
#define CFG_TUH_MAX3421 0
#define CFG_TUH_MAX_SPEED OPT_MODE_FULL_SPEED

// Native OTG rhport mode - host at full speed
#undef CFG_TUSB_RHPORT0_MODE
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_HOST | OPT_MODE_FULL_SPEED)

#ifndef CFG_TUH_MAX3421_ENDPOINT_TOTAL
#define CFG_TUH_MAX3421_ENDPOINT_TOTAL (8 + 4 * (CFG_TUH_DEVICE_MAX - 1))
#endif

// Size of buffer to hold descriptors and other data used for enumeration
#define CFG_TUH_ENUMERATION_BUFSIZE 256

// Number of hub devices
#define CFG_TUH_HUB 0

// max device support (excluding hub device)
#define CFG_TUH_DEVICE_MAX 1

// Enable tuh_edpt_xfer() API
// #define CFG_TUH_API_EDPT_XFER       1

#define CFG_TUH_MIDI              1
#define CFG_TUH_MIDI_RX_BUFSIZE   128
#define CFG_TUH_MIDI_TX_BUFSIZE   128

// Disable unused host class drivers (only MIDI needed)
#define CFG_TUH_MSC 0
#define CFG_TUH_HID 0
#define CFG_TUH_CDC 0

#ifdef __cplusplus
}
#endif

#endif
