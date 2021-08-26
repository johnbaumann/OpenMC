#ifndef _ESP_SDCARD_H
#define _ESP_SDCARD_H


namespace esp_sio_dev
{
    extern bool sd_mounted;

    void mount_sdcard(void);
    void Task_MountSDCard(void *params);
}

#endif // _ESP_SDCARD_H