#ifndef _SDCARD_H
#define _SDCARD_H

#include <esp_err.h>

namespace esp_sio_dev
{
    namespace storage
    {
        namespace sdcard
        {
            extern bool sd_mounted;

            esp_err_t Init(void);
            void Task_MountSDCard(void *params);
        } // sdcard
    } // storage
} // esp_sio_dev

#endif // _SDCARD_H