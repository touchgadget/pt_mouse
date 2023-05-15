/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industriesi
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/*
 * Stripped down version of hid_mouse_tremor_filter.ino that removes
 * everything except for a USB mouse pass through. This can be used
 * as a foundation for autoclickers and jigglers.
 */

/*
MIT License

Copyright (c) 2023 touchgadgetdev@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* This example demonstrates use of both device and host, where
 * - Device run on native usb controller (controller0)
 * - Host run on bit-banging 2 GPIOs with the help of Pico-PIO-USB library (controller1)
 *
 * Requirements:
 * - [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) library
 * - 2 consecutive GPIOs: D+ is defined by PIN_PIO_USB_HOST_DP, D- = D+ +1
 * - Provide VBus (5v) and GND for peripheral
 * - CPU Speed must be either 120 or 240 Mhz. Selected via "Menu -> CPU Speed"
 */

// pio-usb is required for rp2040 host
#include "pio_usb.h"
#include "pio-usb-host-pins.h"
#include "Adafruit_TinyUSB.h"

// USB Host object
Adafruit_USBH_Host USBHost;

// HID report descriptor using TinyUSB's template
// Single Report (no ID) descriptor
uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_MOUSE()
};

// USB HID object. For ESP32 these values cannot be changed after this declaration
// desc report, desc len, protocol, interval, use out endpoint
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_MOUSE, 2, false);

//--------------------------------------------------------------------+
// Setup and Loop on Core0
//--------------------------------------------------------------------+

void setup()
{
  Serial.begin(115200);
  usb_hid.begin();

  //while ( !Serial ) delay(10);   // wait for native usb
  Serial.println("USB Boot Mouse pass through");
}

void loop()
{
}

//--------------------------------------------------------------------+
// Setup and Loop on Core1
//--------------------------------------------------------------------+

void setup1() {
  //while ( !Serial ) delay(10);   // wait for native usb
  Serial.println("Core1 setup to run TinyUSB host with pio-usb");

  // Check for CPU frequency, must be multiple of 120Mhz for bit-banging USB
  uint32_t cpu_hz = clock_get_hz(clk_sys);
  if ( cpu_hz != 120000000UL && cpu_hz != 240000000UL ) {
    while ( !Serial ) delay(10);   // wait for native usb
    Serial.printf("Error: CPU Clock = %lu, PIO USB require CPU clock must be multiple of 120 Mhz\r\n", cpu_hz);
    Serial.println("Change your CPU Clock to either 120 or 240 Mhz in Menu->CPU Speed");
    while(1) delay(1);
  }

#ifdef PIN_PIO_USB_HOST_VBUSEN
  pinMode(PIN_PIO_USB_HOST_VBUSEN, OUTPUT);
  digitalWrite(PIN_PIO_USB_HOST_VBUSEN, PIN_PIO_USB_HOST_VBUSEN_STATE);
#endif

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = PIN_PIO_USB_HOST_DP;
  USBHost.configure_pio_usb(1, &pio_cfg);

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  USBHost.begin(1);
}

void loop1()
{
  USBHost.task();
}


// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
  (void) desc_report;
  (void) desc_len;
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  Serial.printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  Serial.printf("VID = %04x, PID = %04x\r\n", vid, pid);

  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  if (itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
    Serial.println("HID Mouse");
    if (!tuh_hid_receive_report(dev_addr, instance)) {
      Serial.println("Error: cannot request to receive report");
    }
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  Serial.printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
    uint8_t const *report, uint16_t len) {
  if (usb_hid.ready() && (len > 2)) {
    hid_mouse_report_t mouseRpt = {0};
    mouseRpt.buttons = report[0];
    mouseRpt.x = report[1];
    mouseRpt.y = report[2];
    if (len > 3) {
      mouseRpt.wheel = report[3];
      if (len > 4) {
        mouseRpt.pan = report[4];
      }
    }
    usb_hid.sendReport(0, (void *)&mouseRpt, sizeof(mouseRpt));
  }

  // continue to request to receive report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
    Serial.println("Error: cannot request to receive report");
  }
}
