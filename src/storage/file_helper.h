#ifndef _FILE_HELPER_H
#define _FILE_HELPER_H

#include <stdio.h>

namespace esp_sio_dev
{
  namespace storage
  {
    extern FILE *mc_file;
    extern int rw_fail_count;

    void SD_Write_Task(void *params);
    void LoadCardFromFile(char *filepath, void *destination);
    int WriteFile();
  } // storage
} // esp_sio_dev

#endif // _FILE_HELPER_H