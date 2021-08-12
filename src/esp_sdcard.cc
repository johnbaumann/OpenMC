/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SPI peripheral to communicate with SD card.

#include "esp_sdcard.h"

#include "esp_file_helper.h"
#include "esp_file_server.h"
#include "esp_logging.h"
#include "sio.h"
#include "sio_memory_card.h"

#include <dirent.h>
#include <esp_vfs.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define MOUNT_POINT "/sdcard"
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

// Pin mapping
const gpio_num_t kSDPin_CS = GPIO_NUM_21;   // Green
const gpio_num_t kSDPin_MOSI = GPIO_NUM_22; // Blue
const gpio_num_t kSDPin_CLK = GPIO_NUM_19;  // Yellow
const gpio_num_t kSDPin_MISO = GPIO_NUM_23; // Orange

#define SPI_DMA_CHAN (spi_dma_chan_t)1

namespace esp_sio_dev
{

    FILE *sdcard_file;
    const char mount_point[] = MOUNT_POINT;
    sdmmc_card_t *card;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT(); // Macro, not actually a function call

    void mount_sdcard(void)
    {
        ESP_LOGI(kLogPrefix, "sdcard entry point on core %i\n", xPortGetCoreID());
        esp_err_t ret;

        // Options for mounting the filesystem.
        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024};
        ESP_LOGI(kLogPrefix, "Initializing SD card");

        // Use settings defined above to initialize SD card and mount FAT filesystem.
        // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
        // Please check its source code and implement error recovery when developing
        // production applications.
        ESP_LOGI(kLogPrefix, "Using SPI peripheral");

        spi_bus_config_t bus_cfg = {
            .mosi_io_num = kSDPin_MOSI,
            .miso_io_num = kSDPin_MISO,
            .sclk_io_num = kSDPin_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
            .flags = SPICOMMON_BUSFLAG_MASTER,
            .intr_flags = 0};
        ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SPI_DMA_CHAN);
        if (ret != ESP_OK)
        {
            ESP_LOGE(kLogPrefix, "Failed to initialize bus.");
            return;
        }

        // This initializes the slot without card detect (CD) and write protect (WP) signals.
        // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = kSDPin_CS;
        slot_config.host_id = (spi_host_device_t)host.slot;

        ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

        if (ret != ESP_OK)
        {
            if (ret == ESP_FAIL)
            {
                ESP_LOGE(kLogPrefix, "Failed to mount filesystem. "
                                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
            }
            else
            {
                ESP_LOGE(kLogPrefix, "Failed to initialize the card (%s). "
                                     "Make sure SD card lines have pull-up resistors in place.",
                         esp_err_to_name(ret));
            }
            return;
        }

        // Card has been initialized, print its properties
        //sdmmc_card_print_info(stdout, card);
    }

    void unmount_sdcard(void)
    {
        // All done, unmount partition and disable SPI peripheral
        esp_vfs_fat_sdcard_unmount(mount_point, card);
        ESP_LOGI(kLogPrefix, "Card unmounted");

        //deinitialize the bus after all devices are removed
        spi_bus_free((spi_host_device_t)host.slot);
    }

    void Task_MountSDCard(void *params)
    {
        mount_sdcard();
        LoadCardFromFile((char *)"/sdcard/freeboot.mc", sio::memory_card::memory_card_ram);
        vTaskDelete(NULL); // NULL means "this task"
    }
}