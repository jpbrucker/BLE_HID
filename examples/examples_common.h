#ifndef HID_EXAMPLES_COMMON_H_
#define HID_EXAMPLES_COMMON_H_

/**
 * Functions and configuration common to all HID demos
 */

#include "ble/BLE.h"

#include "HIDServiceBase.h"

/**
 * IO capabilities of the device. During development, you most likely want "JustWorks", which means
 * no IO capabilities.
 * It is also possible to use IO_CAPS_DISPLAY_ONLY to generate and show a pincode on the serial
 * output.
 */
#ifndef HID_SECURITY_IOCAPS
#define HID_SECURITY_IOCAPS (SecurityManager::IO_CAPS_NONE)
#endif

/**
 * Security level. MITM disabled forces "Just Works". If you require MITM, HID_SECURITY_IOCAPS must
 * be at least IO_CAPS_DISPLAY_ONLY.
 */
#ifndef HID_SECURITY_REQUIRE_MITM
#define HID_SECURITY_REQUIRE_MITM false
#endif

/**
 * Disable debug messages by setting NDEBUG
 */
#ifndef NDEBUG
#define HID_DEBUG(...) printf(__VA_ARGS__)
#else
#define HID_DEBUG(...)
#endif

/**
 * Initialize security manager: set callback functions and required security level
 */
void initializeSecurity(BLE &ble);

/**
 * - Initialize auxiliary services required by the HID-over-GATT Profile.
 * - Initialize common Gap advertisement.
 *
 * Demos only have to set a custom device name and appearance, and their HID
 * service.
 */
void initializeHOGP(BLE &ble);

#endif /* !BLE_HID_COMMON_H_ */
