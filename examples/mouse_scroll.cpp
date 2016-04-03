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
#include "MouseService.h"

#include "examples_common.h"
/*
 * Simplest use of MouseService: scroll up and down when buttons are pressed
 * To do that, we change wheel speed in HID reports when a button is pushed,
 * and reset it to 0 when it is released.
 */

BLE ble;

MouseService *mouseServicePtr;
static const char DEVICE_NAME[] = "TrivialMouse";
static const char SHORT_DEVICE_NAME[] = "mouse0";

DigitalOut waiting_led(LED1);
DigitalOut connected_led(LED2);

InterruptIn button1(BUTTON1);
InterruptIn button2(BUTTON2);

void button1_down() {
    mouseServicePtr->setSpeed(0, 0, 1);
}

void button1_up() {
    mouseServicePtr->setSpeed(0, 0, 0);
}

void button2_down() {
    mouseServicePtr->setSpeed(0, 0, -1);
}

void button2_up() {
    mouseServicePtr->setSpeed(0, 0, 0);
}


static void onDisconnect(const Gap::DisconnectionCallbackParams_t *params)
{
    HID_DEBUG("disconnected\r\n");
    connected_led = 0;

    ble.gap().startAdvertising(); // restart advertising
}

static void onConnect(const Gap::ConnectionCallbackParams_t *params)
{
    HID_DEBUG("connected\r\n");
    waiting_led = 0;
}

static void waiting() {
    if (!mouseServicePtr->isConnected())
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

    HID_DEBUG("initialising ticker\r\n");

    heartbeat.attach(waiting, 1);

    HID_DEBUG("initialising ble\r\n");
    ble.init();

    ble.gap().onDisconnection(onDisconnect);
    ble.gap().onConnection(onConnect);

    initializeSecurity(ble);

    HID_DEBUG("adding hid service\r\n");

    MouseService mouseService(ble);
    mouseServicePtr = &mouseService;

    HID_DEBUG("adding dev info and battery service\r\n");
    initializeHOGP(ble);

    HID_DEBUG("setting up gap\r\n");
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::MOUSE);
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
