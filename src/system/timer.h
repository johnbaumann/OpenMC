#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include <esp_attr.h>

namespace esp_sio_dev
{
    namespace system
    {
        namespace timer
        {
            extern uint64_t DRAM_ATTR timestamp;
            void Init(void);
        } // timer
    } // system
} // esp_sio_dev

#endif // _TIMER_H