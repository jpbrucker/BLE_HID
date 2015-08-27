# Human Interface Device over Bluetooth Low Energy

Human Interface Device ([HID][USBHID]) is a standard that defines the format used to communicate human inputs to a computer. It was originally created for USB keyboards and mice, as well as joysticks, digitizers, audio controllers, etc.

The [HID-over-GATT profile][HOGP] (HOGP) can be used to send HID informations over Bluetooth Low Energy, using the same data format as USB HID.
Warning: this is not the same as the HID profile used on Bluetooth classic.

## Content

This repository contains the HID Service implementation, example code, and various documentations.

### Code

- `BLE_HID/HIDServiceBase.*`
  The HID Service implementation, requires *BLE\_API*
- `BLE_HID/KeyboardService.h`
  An example use of HIDServiceBase, that allows to send Keycode reports.
- `examples/keyboard_stream.cpp`
  An example use of KeyboardService, that sends strings through series of HID reports.

### Documentation

- [doc/HID](doc/HID.md)
  Introduction to the USB HID specification, and to the parts that are re-used in BLE.
- [doc/HIDService](doc/HIDService.md)
  Description of the BLE HID service, and how to use this implementation.


[USBHID]: http://www.usb.org/developers/hidpage/HID1_11.pdf "USB HID 1.11 specification"
[HOGP]: https://developer.bluetooth.org/TechnologyOverview/Pages/HOGP.aspx "HID-over-GATT profile"
[HIDS]: https://developer.bluetooth.org/TechnologyOverview/Pages/HIDS.aspx "BLE HID Sevice"

