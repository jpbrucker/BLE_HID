# The USB HID protocol

## HID reports

This document covers:

* [Keyboard reports](#keyboard).
* [Mouse reports](#mouse).
* [Further reading](#further-reading).

### Keyboard

The keyboard is still the most important interface device (because the current
version of HID doesn't support brain-computer interface).

#### Input reports

A keyboard will send "press key" and "release key" information to the
operating system, which will interpret it differently depending on the
current application. For example, many computer games accept single letters as commands, whereas a text application will simply add the letter to the text.

When the OS receives a "press key" report, it writes the letter or uses it as a command as explained above. It will keep performing the same action, again and again, until it receives a
"release key" report.

The usual format for keyboard reports is the following byte array:

    [modifier, reserved, Key1, Key2, Key3, Key4, Key6, Key7]

Note that this report can send six simultaneous key events. Their order in
the report doesn't matter, since the OS must keep track of the timing of each
key press.

The reserved byte might be used by vendors, but is mostly useless.

When you press the letter 'a' on a USB keyboard, the following report will be
sent over the USB interrupt pipe:

    'a' report:     [0, 0, 4, 0, 0, 0, 0, 0]

This '4' value is the Keycode for the letter 'a', as described in [USB HID
Usage Tables][USBHID UsageTables] (Section 10: Keyboard/Keypad Page). That
document defines the report formats for all standardized HIDs.

After releasing the key, the following report will be sent:

    null report:    [0, 0, 0, 0, 0, 0, 0, 0]

'4' is replaced with '0'; an array of zeros means nothing is being pressed.

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

#### Output reports

We want the OS to be able to control our keyboard's LEDs, for
instance when another keyboard enables Caps Lock. To do this, the OS writes
a separate one-byte output report that defines the state of each LED.

#### Feature reports

There is another type of report, Feature Reports, which allows to set the
internal properties of a device.

#### Report descriptors

The power of USB HID resides in report descriptors. Instead of defining a fixed
report format for each possible device, the USB HID specification provides a
way for devices to describe what their reports will look like.

A report descriptor is a list of *items*. They are a short series of bytes (two or
three), containing each item type and its value. For instance, a "Usage Page" item
of size 1 has type 0x05, and is followed by a byte describing the HID Usage
Page ([USBHID UsageTables]) in which the device lies.

Here are the items of our keyboard:

- *Usage Page: Generic Desktop Control (0x1)*:
  Refers to the Usage Pages table (Section 3), in [USBHID UsageTables].

- *Usage: keyboard (0x06)*:
  This is only the high-level description of the device. Its capabilities will
  be described in the following collections. A keyboard might contain a
  pointing device in addition to its keys. In that case, each input report will
  need to be prefixed with a report ID.

- *Collection: application (0x01)*.
    - *Usage Page: Keyboard or keypad (0x07)*. The table in section 10 shows the
      meaning of each key.
    - **Modifier declaration:**
    - *Usage minimum: Left Control (0xe0)*.
    - *Usage maximum: Right GUI (0xe7)*.
    - *Logical minimum: 0*.
    - *Logical maximum: 1*.
    - *Report size: 1* - size in bits of the report fields.
    - *Report count: 8* - number of data fields.
    - *Input: Dynamic Flag (0x2)* - does the actual declaration.
    - **Reserved field:**
    - *Report count: 1*.
    - *Report size: 1*.
    - *Input: Static Value (0x1)*. This byte is reserved (constant).
    - **Key code array:**
    - *Report count: 6*.
    - *Report size: 8*.
    - *Usage minimum: 0*.
    - *Usage maximum: 101*.
    - *Logical minimum: 0*.
    - *Logical maximum: 101*.
    - *Input: Selector (0x0)*. This means that all items in this array will
      always have one value from the Keyboard or Keypad page, ranging from 0
      to 101, with 0 meaning "no event".
    - **LED output report (five bits for LEDs, and three bits for padding)**:
    - *Usage page: LEDs (0x8)*.
    - *Report count: 5*.
    - *Report size: 1*.
    - *Usage minimum: Num Lock (1)*.
    - *Usage maximum: Kana (5)*.
    - *Output: Dynamic Flag (0x2)*.
    - *Report count: 3*.
    - *Report size: 1*.
    - *Output: Static Value (0x1)*.

- *End Collection*.

### Mouse

The Mouse report format is simpler:

    [Buttons, X-translation, Y-translation]

And the report descriptor is now pretty easy to understand:

- *Usage Page: Generic Desktop Control*.

- *Usage: Mouse*.

- *Collection: application*.

    - *Usage: Pointer*.
    - *Collection: physical*.
        - **First byte is a bitmap of three buttons:**
        - *Report Count: 3*.
        - *Report Size: 1*.
        - *Usage Page: Buttons*.
        - *Usage Minimum: 1*.
        - *Usage Maximum: 3*.
        - *Logical Minimum: 0*.
        - *Logical Maximum: 1*.
        - *Input: DynamicFlags*.
        - *Report Count: 1*.
        - *Report Size: 5* bits padding.
        - *Input: Static Value*.
        - **Then two speed reports:**
        - *Report Size: 8*.
        - *Report Count: 1*.
        - *Usage Page: Generic Desktop*.
        - *Usage: X*.
        - *Usage: Y*.
        - *Logical Minimum: -127*.
        - *Logical Maximum: 127*.
        - *Input: Dynamic value*.
    - *End Collection*.
- *End Collection*.

## Further reading

The USB specification is quite bulky, and navigation is not always easy. Here
are some pointers to documentation sections that are of interest to BLE HID:

I don't have a proper index in my PDF version of [USBHID], so here is the one I
use to navigate:

- 5.2, 5.3, 5.4 Report Descriptor (page *14*).

- 6.2.2 Report Descriptor (page *23*).

- 6.2.2.4 Main Items (page *28*).

- 6.2.2.5 Input/Output/Feature Items (page *30*).

- 6.2.2.6 Collections (page *33*).

- 6.2.2.7 Global Items (page *35*).

- 6.2.2.8 Local Items (page *40*).

- Appendix B: Keyboard and Mouse boot descriptors (page *59*).
  Standardized report descriptors that are implemented by the BIOS without having
  to parse a descriptor. You'll note that this descriptor closely follows the examples above.

- Appendix C: Implementation requirements for a USB keyboard (page *62*).

- Appendix D: Other examples of report descriptors (page *64*).

- Appendix H: Glossary (page *79*).

The [USBHID UsageTables] is annex A of the previous document, and contains all
standardized usage:

- Table 1 in section 3 is a summary of the available Usage Pages.

- 3.4 Usage Types shows how to use Input/Output/Feature items.


[USBHID]: http://www.usb.org/developers/hidpage/HID1_11.pdf "USB HID 1.11 specification"
[USBHID UsageTables]: http://www.usb.org/developers/hidpage/Hut1_12v2.pdf "USB HID Usage Tables"

Next: [HID-over-GATT](HIDService.md)
