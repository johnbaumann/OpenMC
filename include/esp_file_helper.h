#ifndef _ESP_FILE_HELPER_H
#define _ESP_FILE_HELPER_H

#include <stdio.h>

namespace esp_sio_dev
{
  typedef struct
  {
    uint8_t *dest_mc_ram;
    uint16_t dest_sector;
  } param_mc_write;

  extern param_mc_write myparam;

  extern FILE *mc_file;
  extern bool mc_sector_uncommitted[1024];
  extern bool mc_hard_committed;
  extern int rw_fail_count;

  void Create_Write_Task(void *params);
  void LoadCardFromFile(char *filepath, void *destination);
  int WriteFrame(FILE *dest_file, void *mc_ram, uint16_t sector);
}

#endif // _ESP_FILE_HELPER_H