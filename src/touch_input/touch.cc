#include "touch_input/touch.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/touch_pad.h>
#include <esp_log.h>

#define TOUCH_PAD_NO_CHANGE   (-1)
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_FILTER_MODE_EN  (1)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

static void tp_example_touch_pad_init(void)
{
    for (int i = 4;i<= 6;i++) {
        touch_pad_config((touch_pad_t)i, TOUCH_THRESH_NO_USE);
    }
}

/*
  Read values sensed at all available touch pads.
 Print out values in a loop on a serial monitor.
 */
void tp_example_read_task(void *pvParameter)
{
    uint16_t touch_value;
    uint16_t touch_filter_value;

    // Initialize touch pad peripheral.
    // The default fsm mode is software trigger mode.
    ESP_ERROR_CHECK(touch_pad_init());

    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    tp_example_touch_pad_init();

    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);

    printf("Touch Sensor filter mode read, the output format is: \nTouchpad num:[raw data, filtered data]\n\n");

    while (1) {
        for (int i = 4; i <= 6; i++) {
            // If open the filter mode, please use this API to get the touch pad count.
            touch_pad_read_raw_data((touch_pad_t)i, &touch_value);
            touch_pad_read_filtered((touch_pad_t)i, &touch_filter_value);
            printf("T%d:[%4d,%4d] ", i, touch_value, touch_filter_value);
        }
        printf("\n");
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}