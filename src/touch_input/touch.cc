#include "touch_input/touch.h"

#include "pins.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/touch_pad.h>
#include <esp_log.h>

#define TOUCH_PAD_NO_CHANGE (-1)
#define TOUCH_THRESH_NO_USE (0)
#define TOUCH_FILTER_MODE_EN (1)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

// Initial values, consider making these adjustable through config
#define TOUCH_DEFAULT_MIN 300
#define TOUCH_DEFAULT_MAX 400

namespace esp_sio_dev
{
    namespace touch_input
    {
        const touch_pad_t touch_pads[3] = {kTOUCH_Right, kTOUCH_Confirm, kTOUCH_Left};

        // Left/Up
        TouchCallback_t LeftHeld;
        TouchCallback_t LeftPressed;

        // Confirm/OK
        TouchCallback_t ConfirmHeld;
        TouchCallback_t ConfirmPressed;      

        // Right/Down
        TouchCallback_t RightHeld;
        TouchCallback_t RightPressed;

        // Holds min/max/trigger values for the touch input
        TouchCalibration touch_calibration[3];

        // Variables for tracking button bitmasks
        uint8_t input_old;
        uint8_t input_held;
        uint8_t input_trig;

        uint64_t left_pressed_timestamp = 0;
        uint64_t confirm_pressed_timestamp = 0;
        uint64_t right_pressed_timestamp = 0;

        // borrowed some bit operations from https://github.com/fgsfdsfgs/doukutsupsx/blob/master/src/engine/input.c
        // thanks fgsfds

        void SetCallback_LeftHeld(TouchCallback_t callback)
        {
            LeftHeld = callback;
        }

        void SetCallback_LeftPressed(TouchCallback_t callback)
        {
            LeftPressed = callback;
        }

        void SetCallback_ConfirmHeld(TouchCallback_t callback)
        {
            ConfirmHeld = callback;
        }

        void SetCallback_ConfirmPressed(TouchCallback_t callback)
        {
            ConfirmPressed = callback;
        }

        void SetCallback_RightHeld(TouchCallback_t callback)
        {
            RightHeld = callback;
        }

        void SetCallback_RightPressed(TouchCallback_t callback)
        {
            RightPressed = callback;
        }

        static void InitTouch(void)
        {
            // Initialize touch pad peripheral.
            // The default fsm mode is software trigger mode.
            ESP_ERROR_CHECK(touch_pad_init());

            touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
            for (int i = 0; i < 3; i++)
            {
                touch_pad_config(touch_pads[i], TOUCH_THRESH_NO_USE);
                touch_calibration[i].minimum = TOUCH_DEFAULT_MIN;
                touch_calibration[i].maximum = TOUCH_DEFAULT_MAX;
                touch_calibration[i].value = TOUCH_DEFAULT_MAX;
                touch_calibration[i].threshold = ((touch_calibration[i].maximum - touch_calibration[i].minimum) / 2) + touch_calibration[i].minimum;
            }
        }

        static void ReadTouchValues(void)
        {
            for (int i = 0; i < 3; i++)
            {
                touch_pad_read(touch_pads[i], &touch_calibration[i].value);

                if (touch_calibration[i].value < touch_calibration[i].minimum)
                {
                    touch_calibration[i].minimum = touch_calibration[i].value;
                    touch_calibration[i].threshold = ((touch_calibration[i].maximum - touch_calibration[i].minimum) / 2) + touch_calibration[i].minimum;
                }
                else if (touch_calibration[i].value > touch_calibration[i].maximum)
                {
                    touch_calibration[i].maximum = touch_calibration[i].value;
                    touch_calibration[i].threshold = ((touch_calibration[i].maximum - touch_calibration[i].minimum) / 2) + touch_calibration[i].minimum;
                }
            }
        }

        static void UpdateButtonMask(void)
        {
            input_old = input_held;

            input_held = 0;

            for (int i = 0; i < 3; i++)
            {
                if (touch_calibration[i].value >= touch_calibration[i].threshold)
                {
                    input_held |= (1 << i);
                }
            }

            input_trig = input_old & ~input_held;
            // thanks fgsfds
        }

        void Task_TouchInput(void *pvParameter)
        {
            InitTouch();

            while (1)
            {
                ReadTouchValues();
                UpdateButtonMask();

                if (input_trig & kTOUCH_Left_Bitmask)
                {
                    if (LeftPressed)
                        (*LeftPressed)();
                }

                if (input_trig & kTOUCH_Confirm_Bitmask)
                {
                    if (ConfirmPressed)
                        (*ConfirmPressed)();
                }

                if (input_trig & kTOUCH_Right_Bitmask)
                {
                    if (RightPressed)
                        (*RightPressed)();
                }
                vTaskDelay(16 / portTICK_PERIOD_MS);
            }
        }
    } // touch_input
} // esp_sio_dev