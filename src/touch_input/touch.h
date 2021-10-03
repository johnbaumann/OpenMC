#ifndef _TOUCH_H
#define _TOUCH_H

#include <stdint.h>

namespace esp_sio_dev
{
    namespace touch_input
    {
        typedef struct TouchCalibration
        {
            uint16_t minimum;
            uint16_t maximum;
            uint16_t threshold;
            uint16_t value;
        } TouchCalibration;

        typedef void (*TouchCallback_t)(void);

        void SetCallback_Left(TouchCallback_t callback);
        void SetCallback_Confirm(TouchCallback_t callback);
        void SetCallback_Right(TouchCallback_t callback);
        void Task_TouchInput(void *pvParameter);

    } // touch_input
} // esp_sio_dev

#endif // _TOUCH_H