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

#include "HIDServiceBase.h"

enum ButtonState
{
    BUTTON_UP,
    BUTTON_DOWN
};

enum MouseButton
{
    MOUSE_BUTTON_LEFT    = 0x1,
    MOUSE_BUTTON_RIGHT   = 0x2,
    MOUSE_BUTTON_MIDDLE  = 0x4,
};

/**
 * Report descriptor for a standard 3 buttons + wheel mouse with relative X/Y
 * moves
 */
report_map_t MOUSE_REPORT_MAP = {
    USAGE_PAGE(1),      0x01,         // Generic Desktop
    USAGE(1),           0x02,         // Mouse
    COLLECTION(1),      0x01,         // Application
    USAGE(1),           0x01,         //  Pointer
    COLLECTION(1),      0x00,         //  Physical
    USAGE_PAGE(1),      0x09,         //   Buttons
    USAGE_MINIMUM(1),   0x01,
    USAGE_MAXIMUM(1),   0x03,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_COUNT(1),    0x03,         //   3 bits (Buttons)
    REPORT_SIZE(1),     0x01,
    INPUT(1),           0x02,         //   Data, Variable, Absolute
    REPORT_COUNT(1),    0x01,         //   5 bits (Padding)
    REPORT_SIZE(1),     0x05,
    INPUT(1),           0x01,         //   Constant
    USAGE_PAGE(1),      0x01,         //   Generic Desktop
    USAGE(1),           0x30,         //   X
    USAGE(1),           0x31,         //   Y
    USAGE(1),           0x38,         //   Wheel
    LOGICAL_MINIMUM(1), 0x81,         //   -127
    LOGICAL_MAXIMUM(1), 0x7f,         //   127
    REPORT_SIZE(1),     0x08,         //   Three bytes
    REPORT_COUNT(1),    0x03,
    INPUT(1),           0x06,         //   Data, Variable, Relative
    END_COLLECTION(0),
    END_COLLECTION(0),
};

uint8_t report[] = { 0, 0, 0, 0 };

/**
 * @class MouseService
 * @brief HID-over-Gatt mouse service.
 *
 * Send mouse moves and button informations over BLE.
 *
 * @code
 * BLE ble;
 * MouseService mouse(ble);
 *
 * Timeout timeout;
 *
 * void stop_mouse_move(void)
 * {
 *      // Set mouse state to immobile
 *      mouse.setButton(MOUSE_BUTTON_LEFT, MOUSE_UP);
 *      mouse.setSpeed(0, 0, 0);
 * }
 *
 * void start_mouse_move(void)
 * {
 *      // Move left with a left button down. If the focus is on a drawing
 *      // software, for instance, this should draw a line.
 *      mouse.setButton(MOUSE_BUTTON_LEFT, MOUSE_DOWN);
 *      mouse.setSpeed(1, 0, 0);
 *
 *      timeout.attach(stop_mouse_move, 0.2);
 * }
 * @endcode
 */
class MouseService: public HIDServiceBase
{
public:
    MouseService(BLE &_ble) :
        HIDServiceBase(_ble,
                       MOUSE_REPORT_MAP, sizeof(MOUSE_REPORT_MAP),
                       inputReport          = report,
                       outputReport         = NULL,
                       featureReport        = NULL,
                       inputReportLength    = sizeof(inputReport),
                       outputReportLength   = 0,
                       featureReportLength  = 0,
                       reportTickerDelay    = 20),
        buttonsState (0),
        failedReports (0)
    {
        speed[0] = 0;
        speed[1] = 0;
        speed[2] = 0;

        startReportTicker();
    }

    virtual void onConnection(const Gap::ConnectionCallbackParams_t *params)
    {
        HIDServiceBase::onConnection(params);
        startReportTicker();
    }

    virtual void onDisconnection(const Gap::DisconnectionCallbackParams_t *params)
    {
        stopReportTicker();
        HIDServiceBase::onDisconnection(params);
    }

    /**
     * Set X, Y, Z speed of the mouse. Parameters are sticky and will be
     * transmitted on every tick. Users should therefore reset them to 0 when
     * the device is immobile.
     *
     * @param x     Speed on hoizontal axis
     * @param y     Speed on vertical axis
     * @param wheel Scroll speed
     *
     * @returns A status code
     *
     * @note Directions depend on the operating system's configuration. It is
     * customary to increase values on the X axis from left to right, and on the
     * Y axis from top to bottom.
     * Wheel is less standard, although positive values will usually scroll up.
     */
    int setSpeed(int8_t x, int8_t y, int8_t wheel)
    {
        speed[0] = x;
        speed[1] = y;
        speed[2] = wheel;

        startReportTicker();

        return 0;
    }

    /**
     * Toggle the state of one button
     *
     * @returns A status code
     */
    int setButton(MouseButton button, ButtonState state)
    {
        if (state == BUTTON_UP)
            buttonsState &= ~(button);
        else
            buttonsState |= button;

        startReportTicker();

        return 0;
    }

    /**
     * Called by the report ticker
     */
    virtual void sendCallback(void) {
        uint8_t buttons = buttonsState & 0x7;

        if (!connected)
            return;

        bool can_sleep = (report[0] == 0
                       && report[1] == 0
                       && report[2] == 0
                       && report[3] == 0
                       && report[0] == buttons
                       && report[1] == speed[0]
                       && report[2] == speed[1]
                       && report[3] == speed[2]);

        if (can_sleep) {
            /* TODO: find out why there always is two more calls to sendCallback after this
             * stopReportTicker(). */
            stopReportTicker();
            return;
        }

        report[0] = buttons;
        report[1] = speed[0];
        report[2] = speed[1];
        report[3] = speed[2];

        if (send(report))
            failedReports++;
    }

protected:
    uint8_t buttonsState;
    uint8_t speed[3];

public:
    uint32_t failedReports;
};
