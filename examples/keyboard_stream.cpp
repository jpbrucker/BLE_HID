/* mbed Microcontroller Library
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"

#include "ble/BLE.h"
#include "KeyboardService.h"

#include "examples_common.h"

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

DigitalOut waiting_led(LED1);
DigitalOut connected_led(LED2);

InterruptIn button1(BUTTON1);
InterruptIn button2(BUTTON2);

BLE ble;
KeyboardService *kbdServicePtr;

static const char DEVICE_NAME[] = "uKbd";
static const char SHORT_DEVICE_NAME[] = "kbd1";

static void onDisconnect(const Gap::DisconnectionCallbackParams_t *params)
{
    HID_DEBUG("disconnected\r\n");
    connected_led = 0;

    ble.gap().startAdvertising(); // restart advertising
}

static void onConnect(const Gap::ConnectionCallbackParams_t *params)
{
    HID_DEBUG("connected\r\n");
    waiting_led = false;
}

static void waiting() {
    if (!kbdServicePtr->isConnected())
        waiting_led = !waiting_led;
    else
        connected_led = !connected_led;
}

void send_string(const char * c) {
    if (!kbdServicePtr)
        return;

    if (!kbdServicePtr->isConnected()) {
        HID_DEBUG("we haven't connected yet...");
    } else {
        int len = strlen(c);
        kbdServicePtr->printf(c);
        HID_DEBUG("sending %d chars\r\n", len);
    }
}

void send_stuff() {
    send_string("hello world!\n");
}

void send_more_stuff() {
    send_string("All work and no play makes Jack a dull boy\n");
}

int main()
{
    Ticker heartbeat;

    button1.rise(send_stuff);
    button2.rise(send_more_stuff);

    HID_DEBUG("initialising ticker\r\n");

    heartbeat.attach(waiting, 1);

    HID_DEBUG("initialising ble\r\n");
    ble.init();

    ble.gap().onDisconnection(onDisconnect);
    ble.gap().onConnection(onConnect);

    initializeSecurity(ble);

    HID_DEBUG("adding hid service\r\n");
    KeyboardService kbdService(ble);
    kbdServicePtr = &kbdService;

    HID_DEBUG("adding device info and battery service\r\n");
    initializeHOGP(ble);

    HID_DEBUG("setting up gap\r\n");
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::KEYBOARD);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME,
                                           (const uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                           (const uint8_t *)SHORT_DEVICE_NAME, sizeof(SHORT_DEVICE_NAME));

    ble.gap().setDeviceName((const uint8_t *)DEVICE_NAME);

    HID_DEBUG("advertising\r\n");
    ble.gap().startAdvertising();

    while (true) {
        ble.waitForEvent();
    }
}
