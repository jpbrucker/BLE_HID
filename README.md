# Human Interface Device over Bluetooth Low Energy

Human Interface Device ([HID][USBHID]) is a standard that defines the format
used to communicate human inputs to a computer. It was originally created for
USB keyboards, mice, joysticks, digitizers, audio controllers and so on.

You can use the [HID Over GATT Profile][HOGP] (HOGP) to send HID information
over Bluetooth Low Energy, using the same data format as USB HID.

Warning: this is not the same as the HID profile used on Classic Bluetooth.

## Content

This repository contains the HID Service implementation, example code, and
various documentation.

### Code

- `BLE_HID/HIDServiceBase.*`:
  the HID Service implementation; requires *BLE\_API*.
- `BLE_HID/KeyboardService.h`:
  an example use of HIDServiceBase, which sends Keycode reports.
- `BLE_HID/MouseService.h`:
  a service that sends mouse events: linear speed along X/Y axis, scroll speed
  and clicks.
- `BLE_HID/JoystickService.h`:
  a service that sends joystick events: moves along X/Y/Z axis, rotation around
  X, and buttons.
- `examples/keyboard_stream.cpp`:
  an example use of KeyboardService, which sends strings through a series of HID
  reports.
- `examples/mouse_scroll.cpp`:
  an example use of MouseService, which sends scroll reports.

### Documentation

- [doc/HID](doc/HID.md):
  introduction to the USB HID specification, and to the parts that are reused
  in BLE.
- [doc/HIDService](doc/HIDService.md):
  description of the BLE HID service, and how to use this implementation.


[USBHID]: http://www.usb.org/developers/hidpage/HID1_11.pdf "USB HID 1.11 specification"
[HOGP]: https://developer.bluetooth.org/TechnologyOverview/Pages/HOGP.aspx "HID-over-GATT profile"
[HIDS]: https://developer.bluetooth.org/TechnologyOverview/Pages/HIDS.aspx "BLE HID Sevice"
