#ifndef _SPIFFS_H
#define _SPIFFS_H

#include <esp_err.h>

namespace esp_sio_dev
{
    namespace storage
    {
        esp_err_t init_spiffs();
    } // storage
} // esp_sio_dev

#endif // _SPIFFS_H