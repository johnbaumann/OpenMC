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

        TouchCallback_t callback_left;
        TouchCallback_t callback_confirm;
        TouchCallback_t callback_right;

        TouchCalibration touch_calibration[3];

        uint8_t input_old;
        uint8_t input_held;
        uint8_t input_trig;

        // borrowed some bit operations from https://github.com/fgsfdsfgs/doukutsupsx/blob/master/src/engine/input.c
        // thanks fgsfds

        void SetCallback_Left(TouchCallback_t callback)
        {
            callback_left = callback;
        }

        void SetCallback_Confirm(TouchCallback_t callback)
        {
            callback_confirm = callback;
        }

        void SetCallback_Right(TouchCallback_t callback)
        {
            callback_right = callback;
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
                    if (callback_left)
                        (*callback_left)();
                }

                if (input_trig & kTOUCH_Confirm_Bitmask)
                {
                    if (callback_confirm)
                        (*callback_confirm)();
                }

                if (input_trig & kTOUCH_Right_Bitmask)
                {
                    if (callback_right)
                        (*callback_right)();
                }
                vTaskDelay(16 / portTICK_PERIOD_MS);
            }
        }
    } // touch_input
} // esp_sio_dev