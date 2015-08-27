# The USB HID protocol

## HID Reports

### Keyboard

The most important interface device is still the keyboard (sadly, the current
version of HID doesn't support brain-computer interface).

#### Input reports

A keyboard will send "press key" and "release key" informations to the
operating system, which will interpret them differently depending on the
current context.

Upon receiving a "press key" report, if the focus is currently on a notepad or
a text field in a browser, the OS will start writing the character associated
to this key code. It will keep writing the same key over and over until a
"release key" report is received.

The usual format for keyboard reports is the following byte array:

    [Modifier, reserved, Key1, Key2, Key3, Key4, Key6, Key7]

The reserved byte might be used by vendors, but is mostly useless.
When you press the letter 'a' on a USB keyboard, the following report will be
sent over the USB interrupt pipe:

    'a' report:     [0, 0, 4, 0, 0, 0, 0, 0]

After releasing the key, the following report will be sent:

    null report:    [0, 0, 0, 0, 0, 0, 0, 0]

This '4' value is the Keycode for the letter 'a', as described in [USB HID
Usage Tables][USBHID UsageTables] (Section 10: Keyboard/Keypad Page). That
document defines the report formats for all standardized HIDs.

For an uppercase 'A', the report will also need to contain a 'Left Shift'
modifier. The modifier byte is actually a bitmap, which means that each bit
corresponds to one key:

- bit 0: left control
- bit 1: left shift
- bit 2: left alt
- bit 3: left GUI (Win/Apple/Meta key)
- bit 4: right control
- bit 5: right shift
- bit 6: right alt
- bit 7: right GUI

With left shift pressed, out report will look like that:

    'A' report:     [2, 0, 4, 0, 0, 0, 0, 0]

You noticed that this report can send 6 simultaneous key events. Their order in
the report doesn't matter, since the OS must keep track of the timing of each
key press.

#### Output reports

In addition, we want the OS to be able to control our keyboard's LEDs, for
instance when another keyboard enables Caps Lock. To do this, the OS can write
a separate 1-byte output report that defines the state of each LED.

Another type of report, Feature Reports, is also available, and allows to set
internal properties of a device.

#### Report descriptors

The power of USB HID resides in report descriptors. Instead of defining a fixed
report format for each possible device, the USB HID specification provides a
way for devices to describe what their reports will look like.

A report descriptor is a list of *items*. They are short series of bytes (2 or
3), containing each item type and its value. For instance, an "Usage Page" item
of size 1 has type 0x05, and is followed by a bytes describing the HID Usage
page ([USBHID UsageTables]) in which the device lies.

Here are the items of our keyboard:

- *Usage Page: Generic Desktop Control (0x1)*
  Refers to the Usage Pages table (Section 3), in [USBHID UsageTables]
- *Usage: keyboard (0x06)*
  This is only the high-level description of the device. Its capabilities will
  be described in the following collections. A keyboard might well contain a
  pointing device in addition to its keys. In this case, each input report will
  need to be prefixed with a report ID
- *Collection: application (0x01)*.
    - *Usage Page: Keyboard/Keypad (0x07)* The table in section 10 shows the
      meaning of each key
    - **Modifier declaration:**
    - *Usage minimum: LeftControl (0xe0)*
    - *Usage maximum: RightGUI (0xe7)*
    - *Logical minimum: 0*
    - *Logical maximum: 1*
    - *Report size: 1* size in bits of the report fields
    - *Report count: 8* number of data fields
    - *Input: Dynamic Flag (0x2)* does the actual declaration.
    - **Reserved field:**
    - *Report count: 1* 
    - *Report size: 1*
    - *Input: Static Value (0x1)* This next byte is reserved (constant)
    - **Key code array:**
    - *Report count: 6*
    - *Report size: 8*
    - *Usage minimum: 0*
    - *Usage maximum: 101*
    - *Logical minimum: 0*
    - *Logical maximum: 101*
    - *Input: Selector (0x0)* this means that all items in this array will
      always have one value from the Keyboard/Keypad page, comprised between 0
      and 101, with value 0 meaning "no event".
    - **LED output report (5 bits for LEDs, and 3bit padding)**
    - *Usage page: LEDs (0x8)*
    - *Report count: 5*
    - *Report size: 1*
    - *Usage minimum: Num Lock (1)*
    - *Usage maximum: Kana (5)*
    - *Output: Dynamic Flag (0x2)*
    - *Report count: 3*
    - *Report size: 1*
    - *Output: Static Value (0x1)*

- *End Collection*

### Mouse

The Mouse report format is simpler:

    [Buttons, X-translation, Y-translation]

And the report descriptor is now pretty easy to understand:

- *Usage Page: Generic Desktop Control*
- *Usage: Mouse*
- *Collection: application*
    - *Usage: Pointer*
    - *Collection: physical*
        - **First byte is a bitmap of three buttons:**
        - *Report Count: 3*
        - *Report Size: 1*
        - *Usage Page: Buttons*
        - *Usage Minimum: 1*
        - *Usage Maximum: 3*
        - *Logical Minimum: 0*
        - *Logical Maximum: 1*
        - *Input: DynamicFlags*
        - *Report Count: 1*
        - *Report Size: 5* bits padding
        - *Input: Static Value*
        - **Then two speed reports:**
        - *Report Size: 8*
        - *Report Count: 1*
        - *Usage Page: Generic Desktop*
        - *Usage: X*
        - *Usage: Y*
        - *Logical Minimum: -127*
        - *Logical Maximum: 127*
        - *Input: Dynamic value*
    - *End Collection*
- *End Collection*

## Useful documentation

The USB specification is quite bulky, and navigation is not always easy. Here
are some pointers to documentation sections that are of interest to BLE HID:

I don't have a proper index in my PDF version of [USBHID], so here is the one I
use to navigate:

- 5.2, 5.3, 5.4 Report Descriptor (page *14*)
- 6.2.2 Report Descriptor (page *23*)
- 6.2.2.4 Main Items (page *28*)
- 6.2.2.5 Input/Output/Feature Items (page *30*)
- 6.2.2.6 Collections (page *33*)
- 6.2.2.7 Global Items (page *35*)
- 6.2.2.8 Local Items (page *40*)

- Appendix B: Keyboard and Mouse boot descriptors (page *59*).
  Standardized report descriptors that are implemented by BIOS without having
  to parse a descriptor. You'll note that this descriptor follows closely the
  above examples.
- Appendix C: Implementation requirements for an USB keyboard (page *62*)
- Appendix D: Other examples of report descriptors (page *64*)
- Appendix H: Glossary (page *79*)

The [USBHID UsageTables] is annexe A of the previous document, and contains all
standardized usage:

- Table 1 in section 3 is a summary of the available Usage Pages.
- 3.4 Usage types shows how to use Input/Output/Feature items


[USBHID]: http://www.usb.org/developers/hidpage/HID1_11.pdf "USB HID 1.11 specification"
[USBHID UsageTables]: http://www.usb.org/developers/hidpage/Hut1_12v2.pdf "USB HID Usage Tables"

Next: [HID-over-GATT](HIDService.md)
