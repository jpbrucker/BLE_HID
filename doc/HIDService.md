# HID-over-GATT Profile

## BLE HID Service

All of the structure formats described in [HID](HID.html) are used in HID-over-GATT.
The nomenclature is not ideal, though: 

- Report Descriptor, in USB HID, is called Report Map here.
- Report Reference Characteristic Descriptor is the BLE way of setting a report characteristic's metadata. This one contains the type (Input/Output/Feature) and ID of a report.

The HID Service defines the following characteristics:

- *Protocol Mode*: default is report mode, but the device could be put in boot mode.
- *Report Map*: the HID Report descriptor, defining the possible format for Input/Output/Feature reports.
- *Report*: characteristic used as a vehicle for HID reports. Unlike USB where the ID is sent as prefix when there is more than one report per type, the ID is stored in a characteristic descriptor. This means that there will be one characteristic per report described in the Report Map.
- *Boot Keyboard Input Report*: when the device is a keyboard, it must define boot reports
- *Boot Keyboard output Report*.
- *Boot Mouse Input Report*.
- *HID Information*: HID version, localization and some capability flags.
- *HID Control point*: inform the device that host is entering/leaving suspend state.

Instead of USB interrupt pipes, input reports are sent using notifications.
They can also be read by the host.

## Implementation with BLE\_API

A custom HID Device will need to inherit from HIDServiceBase and provide it with a report map and the report structures.
KeyboardService is one possible implementation. As you can see in this file, it defines a constant byte array containing the whole report map, and passes it to HIDServiceBase during construction.

Sending key reports with KeyboardService is done through its putc or printf methods. For example, with `kbdService.printf("Hello world!")`, the string "Hello world!" will go in a circular buffer, and a ticker will consume from this buffer every 20ms.
First, the letter 'H' will be sent by writing the following values in the inputReport characteristic:

    [0x2, 0, 0x0b, 0, 0, 0, 0, 0]
    [0, 0, 0, 0, 0, 0, 0, 0]

On the next tick, we'll send two reports for letter 'e', and so on. Since we're using GATT notifications, there is no way to know if the OS got the message and understood it correctly.
