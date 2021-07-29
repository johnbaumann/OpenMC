#ifndef _APP_SDCARD_H
#define _APP_SDCARD_H


namespace esp_sio_dev
{
    extern bool sd_mounted;

    void mount_sdcard(void);
    void Task_MountSDCard(void *params);
}

#endif // _APP_SDCARD_H