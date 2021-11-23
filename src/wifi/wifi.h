#ifndef _WIFI_H
#define _WIFI_H

#include <stdbool.h>
#include <stdint.h>

namespace openmc
{
    namespace wifi
    {
        enum Mode : uint8_t
        {
            kAcessPoint = 0,
            kClient = 1,
            kNone = 2
        };
        extern bool ready;
        extern char ip_address[16];
        extern uint8_t ssid[32];
    } // wifi
} // openmc

#endif // _WIFI_H