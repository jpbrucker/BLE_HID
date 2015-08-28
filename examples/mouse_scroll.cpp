#include "mbed.h"

#include "ble/BLE.h"
#include "ble/services/BatteryService.h"
#include "ble.h"

#include "HIDDeviceInformationService.h"
#include "MouseService.h"

#define printf(...)

/*
 * Simplest use of MouseService: scroll up and down when buttons are pressed
 * To do that, we change wheel speed in HID reports when a button is pushed,
 * and reset it to 0 when it is released.
 */

BLE ble;

MouseService *mouseServicePtr;
static const char DEVICE_NAME[] = "TrivialMouse";
static const char SHORT_DEVICE_NAME[] = "mouse0";
static const uint16_t uuid16_list[] =  {GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE,
                                        GattService::UUID_DEVICE_INFORMATION_SERVICE,
                                        GattService::UUID_BATTERY_SERVICE};

volatile bool connected = false;

DigitalOut waiting_led(LED1);
DigitalOut connected_led(LED2);

InterruptIn button1(BUTTON1);
InterruptIn button2(BUTTON2);

void button1_down() {
    mouseServicePtr->setSpeed(0, 0, 10);
}

void button1_up() {
    mouseServicePtr->setSpeed(0, 0, 0);
}

void button2_down() {
    mouseServicePtr->setSpeed(0, 0, -10);
}

void button2_up() {
    mouseServicePtr->setSpeed(0, 0, 0);
}


/* BLE-related callbacks */

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

void onDisconnect(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    connected_led = 0;
    connected = false;
    printf("disconnected\r\n");

    ble.gap().startAdvertising(); // restart advertising

    if (mouseServicePtr)
        mouseServicePtr->connected = false;
}

void onConnect(const Gap::ConnectionCallbackParams_t *params) {
    waiting_led = 0;
    connected = true;
    printf("connected\r\n");

    if (mouseServicePtr) {
        mouseServicePtr->connected = true;
    }
}

void waiting() {
    if (!connected)
        waiting_led = !waiting_led;
    else
        connected_led = !connected_led;
}

int main()
{
    Ticker heartbeat;

    button1.rise(button1_up);
    button1.fall(button1_down);
    button2.rise(button2_up);
    button2.fall(button2_down);

    printf("initialising ticker\r\n");

    heartbeat.attach(waiting, 1);

    printf("initialising ble\r\n");

    ble.init();

    ble.securityManager().onSecuritySetupInitiated(securitySetupInitiatedCallback);
    ble.securityManager().onPasskeyDisplay(passkeyDisplayCallback);
    ble.securityManager().onSecuritySetupCompleted(securitySetupCompletedCallback);

    ble.gap().onDisconnection(onDisconnect);
    ble.gap().onConnection(onConnect);

    bool enableBonding = true;
    bool requireMITM = false;
    ble.securityManager().init(enableBonding, requireMITM, SecurityManager::IO_CAPS_NONE);

    printf("adding dev info and battery service\r\n");

    PnPID_t pnpID;
    pnpID.vendorID_source = 0x2; // from the USB Implementer's Forum
    pnpID.vendorID = 0x0D28; // NXP
    pnpID.productID = 0x0204; // CMSIS-DAP
    pnpID.productVersion = 0x0100; // v1.0
    HIDDeviceInformationService deviceInfo(ble, "ARM", "m1", "abc", "def", "ghi", "jkl", &pnpID);
    BatteryService batteryInfo(ble, 80);

    printf("adding hid service\r\n");

    MouseService mouseService(ble);
    mouseServicePtr = &mouseService;

    printf("setting up gap\r\n");
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED |
                                           GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS,
                                           (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::MOUSE);
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
