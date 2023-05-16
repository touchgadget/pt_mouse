# USB Boot Mouse Pass Through

Simple demonstration of passing USB mouse HID boot reports from a USB mouse
through to a computer.

## Hardware

For Adafruit RP2040 Feather USB Host board. The board supports USB host and
device at the same time. Lots more information at
https://learn.adafruit.com/adafruit-feather-rp2040-with-usb-type-a-host.

* Adafruit Feather RP2040 with USB Type A Host

## Dependencies

Use this board package.

* https://github.com/earlephilhower/arduino-pico

The following libraries can be installed using the IDE library manager.

* "Adafruit TinyUSB Library" by Adafruit
* "Pico PIO USB" by sekigon-gonnoc

## IDE Tools options required

* Set "Board" to "Adafruit Feather RP2040 USB Host"
* Set "USB Stack" to "Adafruit TinyUSB"
* Set "CPU Speed" to 120MHz.
