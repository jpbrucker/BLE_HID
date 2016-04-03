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

#ifdef USE_JOYSTICK
#include "JoystickService.h"
#define HID_BUTTON_1 JOYSTICK_BUTTON_1
#define HID_BUTTON_2 JOYSTICK_BUTTON_2
#else
#include "MouseService.h"
#define HID_BUTTON_1 MOUSE_BUTTON_LEFT
#define HID_BUTTON_2 MOUSE_BUTTON_RIGHT
#endif

#include "examples_common.h"

/*
 * This demo drives the joystick/mouse HID service with the micro:bit's accelerometer, an MMA8653.
 * The accelerometer is polled every 40ms or so, and the HIDService sends speed reports.
 *
 * How it works: when immobile, the accelerometer reports an acceleration of 1g = 9.8m/s^2.
 * When horizontal, ax = ay = 0, and az = g. Otherwise, g will be projected on each axis. This demo
 * uses that projection on ax and ay, to control the speed of the joystick.
 *
 * Linear moves will be negligible compared to g reports, and are almost impossible to detect
 * without adding at least a gyro in the mix.
 */

#define MMA8653_ADDR            0x3a
#define MMA8653_OUT_X_MSB       0x01
#define MMA8653_WHOAMI          0x0d
#define MMA8653_XYZ_DATA_CFG    0x0e
#define MMA8653_CTRL_REG1       0x2a
#define MMA8653_CTRL_REG4       0x2d
#define MMA8653_CTRL_REG5       0x2e

I2C i2c(p30, p0);

BLE ble;

#ifdef USE_JOYSTICK
JoystickService *hidServicePtr;
#else
MouseService *hidServicePtr;
#endif

static const char DEVICE_NAME[] = "uJoy";
static const char SHORT_DEVICE_NAME[] = "joystick0";

DigitalOut waiting_led(LED1);
DigitalOut connected_led(LED2);

InterruptIn button1(BUTTON_A);
InterruptIn button2(BUTTON_B);

void button1_down()
{
    if (hidServicePtr)
        hidServicePtr->setButton(HID_BUTTON_1, BUTTON_DOWN);
}

void button1_up()
{
    if (hidServicePtr)
        hidServicePtr->setButton(HID_BUTTON_1, BUTTON_UP);
}

void button2_down()
{
    if (hidServicePtr)
        hidServicePtr->setButton(HID_BUTTON_2, BUTTON_DOWN);
}

void button2_up()
{
    if (hidServicePtr)
        hidServicePtr->setButton(HID_BUTTON_2, BUTTON_UP);
}


/* ---- MMA8653 handling ---- */
int write_accel(uint8_t reg, uint8_t data)
{
    uint8_t command[2];

    command[0] = reg;
    command[1] = data;

    return i2c.write(MMA8653_ADDR, (const char*)command, 2, true);
}

int read_accel(uint8_t reg, uint8_t *buffer, int length)
{
    int err = i2c.write(MMA8653_ADDR, (const char *)&reg, 1, true);

    if (err) {
        HID_DEBUG("init write failed\r\n");
        return err;
    }
    return i2c.read(MMA8653_ADDR, (char *)buffer, length);
}

void init_accel(void)
{
    uint8_t whoami;
    int err;

    /* Put device in standby mode */
    err = write_accel(MMA8653_CTRL_REG1, 0x00);
    if (err)
        HID_DEBUG("CTRL_REG1 1 failed\r\n");

    /* Interrupt DRDY */
    err = write_accel(MMA8653_CTRL_REG4, 0x00);
    if (err)
        HID_DEBUG("CTRL_REG4 failed\r\n");

    /* same, CFG */
    err = write_accel(MMA8653_CTRL_REG5, 0x00);
    if (err)
        HID_DEBUG("CTRL_REG5 failed\r\n");

    /* +/- 2g */
    err = write_accel(MMA8653_XYZ_DATA_CFG, 0x00);
    if (err)
        HID_DEBUG("CTRL_DATA_CFG failed\r\n");

    /*
     * Data rate = 50Hz
     * 10 bits of data
     */
    err = write_accel(MMA8653_CTRL_REG1, 0x21);
    if (err)
        HID_DEBUG("CTRL_REG1 2 failed\r\n");

    read_accel(MMA8653_WHOAMI, &whoami, 1);
    HID_DEBUG("Accel is %x\r\n", whoami);
    MBED_ASSERT(whoami == 0x5a);
}

/** Integer square root of an uint16 */
static uint8_t sqrti(uint16_t v)
{
    static const uint16_t sqr_table[] = {
        1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144, 169, 196, 225, 256, 289,
        324, 361, 400, 441, 484, 529, 576, 625, 676, 729, 784, 841, 900, 961
    };
    static const unsigned sqr_table_size = sizeof(sqr_table) / sizeof(sqr_table[0]);
    uint8_t i;

    for (i = 0; i < sqr_table_size; i++)
    {
        if (sqr_table[i] > v)
            return i;
    }

    return sqr_table[sqr_table_size - 1];
}

/**
 * Invert sign and attempt to smoothen the acceleration value.
 * Disclaimer: the process used to write the following functions was exteremly chaotic.
 * No calculation whatsoever was involved.
 */
static int8_t soften_accel(int16_t v)
{
    int sign = -1;

    if (v < 0) {
        sign = 1;
        v = -v;
    }

    return sign * sqrti(v / 5);
}

void poll_accel(void)
{
    static const int maxv = 128;
    int8_t data[6];

    static int vx = 0;
    static int vy = 0;

    int ax = 0;
    int ay = 0;

    read_accel(MMA8653_OUT_X_MSB, (uint8_t *)data, 6);

    ax = data[0];
    ay = data[2];

    ax = soften_accel(ax);
    ay = soften_accel(ay);

    vx = ax + vx;
    vy = ay + vy;

    /* Clamp down on speed */
    if (vx > maxv)
        vx = maxv;
    if (vx < -maxv)
        vx = -maxv;
    if (vy > maxv)
        vy = maxv;
    if (vy < -maxv)
        vy = -maxv;

    if (hidServicePtr) {
        /*
         * Reduce speed a little bit. HID report values must be in [-127; 127], but above 32 is
         * generally too high anyway.
         */
        hidServicePtr->setSpeed(vx / 4, vy / 4, 0);
    }

    /**
     * Decrease over time. We need this to remove integration drifts due to noise, but it could well
     * be improved, as this prevents from having steady low speeds.
     */
    vx *= 0.999;
    vy *= 0.999;
}

void onDisconnect(const Gap::DisconnectionCallbackParams_t *params)
{
    HID_DEBUG("disconnected\r\n");
    connected_led = 0;

    ble.gap().startAdvertising(); // restart advertising
}

void onConnect(const Gap::ConnectionCallbackParams_t *params)
{
    HID_DEBUG("connected\r\n");
    waiting_led = 0;
}

void waiting() {
    if (!hidServicePtr->isConnected())
        waiting_led = !waiting_led;
    else
        connected_led = !connected_led;
}

int main()
{
    Ticker accel_poll_ticker;
    Ticker heartbeat;

    init_accel();

    accel_poll_ticker.attach(poll_accel, 0.02);

    button1.rise(button1_up);
    button1.fall(button1_down);
    button2.rise(button2_up);
    button2.fall(button2_down);

    HID_DEBUG("initialising ticker\r\n");

    heartbeat.attach(waiting, 1);

    HID_DEBUG("initialising ble\r\n");

    ble.init();

    initializeSecurity(ble);

    ble.gap().onDisconnection(onDisconnect);
    ble.gap().onConnection(onConnect);

    HID_DEBUG("adding hid service\r\n");

#ifdef USE_JOYSTICK
    JoystickService joystickService(ble);
    hidServicePtr = &joystickService;

    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::JOYSTICK);
#else
    MouseService mouseService(ble);
    hidServicePtr = &mouseService;

    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::MOUSE);
#endif

    HID_DEBUG("setting up gap\r\n");
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME,
                                           (const uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                           (const uint8_t *)SHORT_DEVICE_NAME, sizeof(SHORT_DEVICE_NAME));

    ble.gap().setDeviceName((const uint8_t *)DEVICE_NAME);

    HID_DEBUG("adding dev info and battery service\r\n");
    initializeHOGP(ble);

    HID_DEBUG("advertising\r\n");
    ble.gap().startAdvertising();

    while (true) {
        ble.waitForEvent();
    }
}
