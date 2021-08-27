#ifndef _SDCARD_H
#define _SDCARD_H

namespace esp_sio_dev
{
    namespace storage
    {
        extern bool sd_mounted;

        void mount_sdcard(void);
        void Task_MountSDCard(void *params);
    } // storage
} // esp_sio_dev

#endif // _SDCARD_H