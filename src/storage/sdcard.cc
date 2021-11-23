#include "storage/sdcard.h"

#include "logging.h"
#include "pins.h"
#include "playstation/sio.h"
#include "playstation/sio_memory_card.h"
#include "storage/storage.h"
#include "web/file_server.h"

#include <dirent.h>
#include <esp_err.h>
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>


#define FILE_PATH_MAX CONFIG_FATFS_MAX_LFN

#define SPI_DMA_CHAN (spi_dma_chan_t)1

namespace openmc
{
    namespace storage
    {
        namespace sdcard
        {

            bool mounted;
            const char mount_point[] = "/sdcard";
            sdmmc_card_t *card;
            sdmmc_host_t host = {
                .flags = SDMMC_HOST_FLAG_SPI | SDMMC_HOST_FLAG_DEINIT_ARG,
                .slot = VSPI_HOST,
                .max_freq_khz = SDMMC_FREQ_DEFAULT,
                .io_voltage = 3.3f,
                .init = &sdspi_host_init,
                .set_bus_width = NULL,
                .get_bus_width = NULL,
                .set_bus_ddr_mode = NULL,
                .set_card_clk = &sdspi_host_set_card_clk,
                .do_transaction = &sdspi_host_do_transaction,
                .deinit_p = &sdspi_host_remove_device,
                .io_int_enable = &sdspi_host_io_int_enable,
                .io_int_wait = &sdspi_host_io_int_wait,
                .command_timeout_ms = 0,
            };

            esp_err_t Init(void)
            {
                esp_err_t ret;

                // Options for mounting the filesystem.
                esp_vfs_fat_sdmmc_mount_config_t mount_config = {
                    .format_if_mount_failed = false,
                    .max_files = 5,
                    .allocation_unit_size = 16 * 1024};

                host.slot = VSPI_HOST;

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
                    return ESP_FAIL;
                }

                // This initializes the slot without card detect (CD) and write protect (WP) signals.
                // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
                sdspi_device_config_t slot_config = {
                    .host_id = (spi_host_device_t)host.slot,
                    .gpio_cs = kSDPin_CS,
                    .gpio_cd = SDSPI_SLOT_NO_CD,
                    .gpio_wp = SDSPI_SLOT_NO_WP,
                    .gpio_int = GPIO_NUM_NC,
                };

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
                    return ESP_FAIL;
                }

                return ESP_OK;
            }

            void unmount_sdcard(void)
            {
                // All done, unmount partition and disable SPI peripheral
                esp_vfs_fat_sdcard_unmount(mount_point, card);
                ESP_LOGI(kLogPrefix, "Card unmounted");

                //deinitialize the bus after all devices are removed
                spi_bus_free((spi_host_device_t)host.slot);
            }
        } // sdcard
    } // storage
} // openmc
