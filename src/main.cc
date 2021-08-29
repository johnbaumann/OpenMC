#include "storage/file_helper.h"
#include "web/file_server.h"
#include "storage/sdcard.h"
#include "logging.h"
#include "wifi/access_point.h"
#include "wifi/client.h"
#include "oled/oled.h"
#include "pins.h"
#include "playstation/sio.h"
#include "playstation/sio_memory_card.h"
#include "playstation/spi.h"
#include "storage/spiffs.h"

#include "baremetal_core1/core0_stall.h"
#include "baremetal_core1/bare_metal_app_cpu.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <sdkconfig.h>
#include <driver/uart.h>
#include <esp_intr_alloc.h>

// Task sizes - there should be approximately 5% free space to allow for growth
#define SD_CARD_TASK_SIZE 2272
//#define FILE_WRITE_TASK_SIZE 1760
#define FILE_WRITE_TASK_SIZE 3096
#define FILE_SERVER_TASK_SIZE 1877
#define WIFI_CLIENT_TASK_SIZE 2348

// rama
// sickle <3
// Nicolas Noble
// danhans42

extern "C"
{
    void app_main(void);
}

extern DRAM_ATTR uint8_t output[4];

namespace esp_sio_dev
{
    void main(void);
    void SetupInterrupts();

    void SetupInterrupts()
    {
        spi::InstallInterrupt();
    }

    void main(void)
    {
        sio::memory_card_enabled = true;
        sio::pad_enabled = false;
        sio::net_yaroze_enabled = false;

        // Turn off LED
        gpio_set_direction(kPin_LED, GPIO_MODE_OUTPUT);
        gpio_set_level(kPin_LED, 0);

        oled::Init();

        // Create a task to mount the SD Card
        xTaskCreatePinnedToCore(storage::Task_MountSDCard, "sd_card_task_core_0", SD_CARD_TASK_SIZE, NULL, 0, NULL, 0);
        //storage::init_spiffs();

        core0_stall_init();

        sio::Init();       // Init the SIO state machine to a default state.
        spi::InitPins();   // Setup the pins for bitbanged SPI
        spi::Enable();     // Enable SPI
        SetupInterrupts(); // Create a task to install our interrupt handler on Core 1, ESP32 likes Core 0 for WiFi

        start_app_cpu();

        xTaskCreatePinnedToCore(storage::Task_Write, "write_task_core_0", FILE_WRITE_TASK_SIZE, NULL, 0, NULL, 0);

        //xTaskCreatePinnedToCore(wifi::access_point::Task_Start, "wifi_ap_task_core_0", 1024 * 3, NULL, 0, NULL, 0);
        xTaskCreatePinnedToCore(wifi::client::Task_Start, "wifi_client_task_core_0", WIFI_CLIENT_TASK_SIZE, NULL, 0, NULL, 0);

        while (web::file_server::net_interface_ready == false && web::file_server::sd_filesystem_ready == false)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        xTaskCreatePinnedToCore(web::file_server::Task_StartFileServer, "file_server_task_core_0", FILE_SERVER_TASK_SIZE, NULL, 0, NULL, 0);

        //xTaskCreate(tcp_server_task, "tcp_server", 4096, (void *)AF_INET, 5, NULL);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
        printf("Free Heap = %i\n", esp_get_free_heap_size());
    }
}

// Things.
void app_main(void)
{
    esp_sio_dev::main();
}