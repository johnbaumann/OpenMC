#ifndef _ESP_FILE_HELPER_H
#define _ESP_FILE_HELPER_H

#include <stdio.h>

namespace esp_sio_dev
{
  extern FILE *mc_file;
  extern int rw_fail_count;

  void SD_Write_Task(void *params);
  void LoadCardFromFile(char *filepath, void *destination);
  int WriteFile();
}

#endif // _ESP_FILE_HELPER_H