#include "mbed.h"

#include "ble/BLE.h"
#include "ble/services/BatteryService.h"

#include "HIDDeviceInformationService.h"
#include "KeyboardService.h"

/**
 * This program implements a complete HID-over-Gatt Profile:
 *  - HID is provided by KeyboardService
 *  - Battery Service
 *  - Device Information Service
 *
 * Complete strings can be sent over BLE using printf. Please note, however, than a 12char string
 * will take about 500ms to transmit, principally because of the limited notification rate in BLE.
 * KeyboardService uses a circular buffer to store the strings to send, and calls to putc will fail
 * once this buffer is full. This will result in partial strings being sent to the client.
 */

volatile bool connected = false;

DigitalOut waiting_led(LED1);
DigitalOut connected_led(LED2);

InterruptIn button1(BUTTON1);
InterruptIn button2(BUTTON2);

BLE ble;
KeyboardService *kbdServicePtr;

static const char DEVICE_NAME[] = "µKbd";
static const char SHORT_DEVICE_NAME[] = "kbd1";
static const uint16_t uuid16_list[] =  {GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE,
                                        GattService::UUID_DEVICE_INFORMATION_SERVICE,
                                        GattService::UUID_BATTERY_SERVICE};

void onDisconnect(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    connected_led = 0;
    connected = false;
    printf("disconnected\r\n");

    ble.gap().startAdvertising(); // restart advertising
}

void onConnect(const Gap::ConnectionCallbackParams_t *params) {
    waiting_led = 0;
    connected = true;
    printf("connected\r\n");
}

void waiting() {
    if (!connected)
        waiting_led = !waiting_led;
    else
        connected_led = !connected_led;
}

void send_string(const char * c) {
    if (!connected) {
        printf("we haven't connected yet...");
    } else {
        int len = strlen(c);
        kbdServicePtr->printf(c);
        printf("sending %d chars\r\n", len);
    }
}

void send_stuff() {
    send_string("hello world!");
}

void send_more_stuff() {
    send_string("All work and no play makes Jack a dull boy\n");
}

void passkeyDisplayCallback(Gap::Handle_t handle, const SecurityManager::Passkey_t passkey)
{
    printf("Input passKey: ");
    for (unsigned i = 0; i < Gap::ADDR_LEN; i++) {
        printf("%c", passkey[i]);
    }
    printf("\r\n");
}

void securitySetupCompletedCallback(Gap::Handle_t handle, SecurityManager::SecurityCompletionStatus_t status)
{
    if (status == SecurityManager::SEC_STATUS_SUCCESS) {
        printf("Security success %d\r\n", status);
    } else {
        printf("Security failed %d\r\n", status);
    }
}

void securitySetupInitiatedCallback(Gap::Handle_t, bool allowBonding, bool requireMITM, SecurityManager::SecurityIOCapabilities_t iocaps)
{
    printf("Security setup initiated\r\n");
}

int main()
{
    button1.rise(send_stuff);
    button2.rise(send_more_stuff);

    printf("initialising ticker\r\n");

    Ticker heartbeat;
    heartbeat.attach(waiting, 1);

    printf("initialising ble\r\n");

    ble.init();
    ble.securityManager().onSecuritySetupInitiated(securitySetupInitiatedCallback);
    ble.securityManager().onPasskeyDisplay(passkeyDisplayCallback);
    ble.securityManager().onSecuritySetupCompleted(securitySetupCompletedCallback);

    ble.gap().onDisconnection(onDisconnect);
    ble.gap().onConnection(onConnect);

    bool enableBonding = true;
    bool requireMITM = true;
    ble.securityManager().init(enableBonding, requireMITM, SecurityManager::IO_CAPS_DISPLAY_ONLY);

    printf("adding device info and battery service\r\n");

    PnPID_t pnpID;
    pnpID.vendorID_source = 0x2; // from the USB Implementer's Forum
    pnpID.vendorID = 0x0D28; // NXP
    pnpID.productID = 0x0204; // CMSIS-DAP (well, it's a keyboard but oh well)
    pnpID.productVersion = 0x0100; // v1.0
    HIDDeviceInformationService deviceInfo(ble, "ARM", "m1", "abc", "def", "ghi", "jkl", &pnpID);

    BatteryService batteryInfo(ble, 80);

    printf("adding hid service\r\n");

    KeyboardService kbdService(ble);
    kbdServicePtr = &kbdService;

    printf("setting up gap\r\n");
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED |
                                           GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS,
                                           (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::KEYBOARD);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME,
                                           (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                           (uint8_t *)SHORT_DEVICE_NAME, sizeof(SHORT_DEVICE_NAME));

    // see 5.1.2: HID over GATT Specification (pg. 25)
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // 30ms to 50ms is recommended (5.1.2)
    ble.gap().setAdvertisingInterval(50);

    printf("advertising\r\n");
    ble.gap().startAdvertising();

    while (true) {
        ble.waitForEvent();
    }
}
