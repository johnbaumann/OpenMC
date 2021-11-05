#include "wifi/wifi.h"

#include <stdbool.h>
#include <stdint.h>

namespace esp_sio_dev
{
    namespace wifi
    {
        bool ready = false;
        char ip_address[16] = "192.168.4.1";
        uint8_t ssid[32];
    } // wifi
} // esp_sio_dev
