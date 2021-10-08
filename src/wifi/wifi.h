#ifndef _WIFI_H
#define _WIFI_H

#include <stdbool.h>
#include <stdint.h>

namespace esp_sio_dev
{
    namespace wifi
    {
        extern bool ready;
        extern bool is_client_mode;
        extern char ip_address[16];
        extern uint8_t ssid[32];
    } // wifi
} // esp_sio_dev

#endif // _WIFI_H