#include "spiffs.h"

#include "logging.h"

#include <esp_spiffs.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs.h>

namespace esp_sio_dev
{
    namespace storage
    {
        esp_err_t init_spiffs()
        {
            ESP_LOGI(kLogPrefix, "Initializing SPIFFS");

            esp_vfs_spiffs_conf_t conf = {
                .base_path = "/spiffs",
                .partition_label = NULL,
                .max_files = 5, // This decides the maximum number of files that can be created on the storage
                .format_if_mount_failed = true};

            esp_err_t ret = esp_vfs_spiffs_register(&conf);
            if (ret != ESP_OK)
            {
                if (ret == ESP_FAIL)
                {
                    ESP_LOGE(kLogPrefix, "Failed to mount or format filesystem");
                }
                else if (ret == ESP_ERR_NOT_FOUND)
                {
                    ESP_LOGE(kLogPrefix, "Failed to find SPIFFS partition");
                }
                else
                {
                    ESP_LOGE(kLogPrefix, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
                }
                return ESP_FAIL;
            }

            size_t total = 0, used = 0;
            ret = esp_spiffs_info(NULL, &total, &used);
            if (ret != ESP_OK)
            {
                ESP_LOGE(kLogPrefix, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
                return ESP_FAIL;
            }

            ESP_LOGI(kLogPrefix, "Partition size: total: %d, used: %d", total, used);
            return ESP_OK;
        }
    } // storage
} // esp_sio_dev