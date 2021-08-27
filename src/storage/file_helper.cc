#include "storage/file_helper.h"
#include "playstation/sio.h"
#include "playstation/sio_memory_card.h"

#include "logging.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace esp_sio_dev
{
  namespace storage
  {
    char loaded_file_path[CONFIG_FATFS_MAX_LFN + 1];

    void SD_Write_Task(void *params)
    {
      while (1)
      {
        if (sio::memory_card::committed_to_storage == false && sio::memory_card::last_write_tick + 50 <= sio::event_counter)
        {
          if(WriteFile() < 0)
          {
            vTaskDelay(200 / portTICK_PERIOD_MS);
          }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
    }

    void LoadCardFromFile(char *filepath, void *destination)
    {
      FILE *mc_file;
      bool old_mc_status = sio::memory_card_enabled;
      sio::memory_card_enabled = false;

      ESP_LOGI(kLogPrefix, "LoadCardFromFile(%s)\n", filepath);
      mc_file = fopen(filepath, "r+");
      uint8_t *dest = (uint8_t *)destination;

      long file_size;

      if (!mc_file)
      {
        ESP_LOGE(kLogPrefix, "Error opening file!\n");
        sio::memory_card_enabled = old_mc_status;
        return;
      }
      else
      {
        fseek(mc_file, 0, SEEK_END);
        file_size = ftell(mc_file);
        if (file_size <= 0)
        {
          ESP_LOGE(kLogPrefix, "File is empty\n");
          fclose(mc_file);
          sio::memory_card_enabled = old_mc_status;
          return;
        }

        rewind(mc_file);

        if (file_size != (128 * 1024))
        {
          ESP_LOGE(kLogPrefix, "Invalid File size!\n");
          fclose(mc_file);
          sio::memory_card_enabled = old_mc_status;
          return;
        }

        fread(dest, 1, file_size, mc_file);
        fclose(mc_file);

        strlcpy(loaded_file_path, filepath, sizeof(loaded_file_path));

        sio::memory_card::flag = sio::memory_card::Flags::kDirectoryUnread;
        sio::memory_card::GoIdle();
        sio::memory_card::last_write_tick = sio::event_counter;
        sio::memory_card::committed_to_storage = true;
      }

      // Make sure MC stays inactive long enough for BIOS to detect change
      // To-do: Adjust lower and test
      // Bios takes roughly 1051ms between MC status polls.
      // Can probably get away with less, but go a bit longer.
      // We need to miss at least one status check to initiate a proper refresh in bios.
      vTaskDelay(1337 / portTICK_PERIOD_MS);
      sio::memory_card_enabled = old_mc_status;
    }

    int WriteFile()
    {
      FILE *mc_file;
      uint32_t write_end_time;
      uint32_t write_start_time = esp_log_timestamp();

      ESP_LOGI(kLogPrefix, "WriteFile(%s)\n", loaded_file_path);
      mc_file = fopen(loaded_file_path, "r+");

      if (!mc_file)
      {
        ESP_LOGE(kLogPrefix, "Error opening file!\n");
        return -1;
      }

      if (fwrite(sio::memory_card::memory_card_ram, 1, (128 * 1024), mc_file) != (128 * 1024))
      {
        ESP_LOGE(kLogPrefix, "Error writing file!\n");
        return -3;
      }
      else if (ferror(mc_file))
      {
        ESP_LOGE(kLogPrefix, "Unknown error writing file!\n");
      }
      else
      {
        ESP_LOGI(kLogPrefix, "File written!\n");
      }

      fclose(mc_file);

      write_end_time = esp_log_timestamp();
      sio::memory_card::committed_to_storage = true;

      printf("Took %i ms to write file.\n", write_end_time - write_start_time);

      return 0;
    }
  } // storage
} // esp_sio_dev