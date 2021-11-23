#ifndef _TOUCH_H
#define _TOUCH_H

#include <stdint.h>

namespace openmc
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

        void SetCallback_LeftPressed(TouchCallback_t callback);
        void SetCallback_ConfirmPressed(TouchCallback_t callback);
        void SetCallback_RightPressed(TouchCallback_t callback);
        void Task_TouchInput(void *pvParameter);

    } // touch_input
} // openmc

#endif // _TOUCH_H