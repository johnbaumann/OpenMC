#ifndef _APP_SDCARD_H
#define _APP_SDCARD_H

extern bool sd_mounted;

void mount_sdcard(void);
void Task_SDCardTest(void *params);

#endif // _APP_SDCARD_H