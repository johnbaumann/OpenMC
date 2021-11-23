#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include <esp_attr.h>

namespace openmc
{
    namespace system
    {
        namespace timer
        {
            extern uint64_t DRAM_ATTR timestamp;
            void Init(void);
        } // timer
    } // system
} // openmc

#endif // _TIMER_H