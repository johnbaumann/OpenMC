#ifndef _STORAGE_H
#define _STORAGE_H

#include <stdbool.h>
#include <stdio.h>

#define MAX_WRITE_FAILURES 5

namespace openmc
{
  namespace storage
  {
    extern FILE *mc_file;
    extern int rw_fail_count;
    extern bool ready;
    extern char loaded_file_path[];
    extern char base_path[];

    void Init();
    void Task_Write(void *params);
    bool LoadCardFromFile(char *filepath, void *destination);
    int WriteFile();
  } // storage
} // openmc

#endif // _STORAGE_H