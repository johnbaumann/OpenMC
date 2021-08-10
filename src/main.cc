#include "esp_file_helper.h"
#include "esp_file_server.h"
#include "esp_sdcard.h"
#include "esp_logging.h"
#include "esp_wifi_ap.h"
#include "esp_wifi_client.h"
#include "hardware.h"
#include "sio.h"
#include "sio_memory_card.h"
#include "spi.h"

#include "core0_stall.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <sdkconfig.h>
#include <driver/uart.h>
#include <esp_intr_alloc.h>

#define SD_TASK_CORE 0
#define WIFI_TASK_CORE 0
#define SIO_TASK_CORE 1

// Refs
// https://psx-spx.consoledev.net/
// https://github.com/nkolban/esp32-snippets/blob/master/gpio/interrupts/test_intr.c
// https://esp32.com/viewtopic.php?t=13432
// https://github.com/nkolban/esp32-snippets/blob/master/gpio/interrupts/test_intr.c
// not using pins >= 34 as pull ups
// https://www.esp32.com/viewtopic.php?t=1183
// rama
// sickle <3
// Nicolas Noble
// danhans42

extern "C"
{
    void app_main(void);
    void start_app_cpu(void);
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

        xTaskCreatePinnedToCore(Task_MountSDCard, "sd_card_task_core_0", 1024 * 3, NULL, 0, NULL, SD_TASK_CORE);

        core0_stall_init();
        sio::Init();       // Init the SIO state machine to a default state.
        spi::InitPins();   // Setup the pins for SPI
        spi::Enable();     // Enable SPI
        SetupInterrupts(); // Create a task to install our interrupt handler on Core 1, ESP32 likes Core 0 for WiFi
        start_app_cpu();

        //xTaskCreatePinnedToCore(wifi_ap::Task_StartWifiAP, "wifi_ap_task_core_0", 1024 * 40, NULL, 0, NULL, WIFI_TASK_CORE);
        xTaskCreatePinnedToCore(wifi_client::Task_StartWifiClient, "wifi_client_task_core_0", 1024 * 3, NULL, 0, NULL, WIFI_TASK_CORE);
        
        printf("Free Heap = %i\n", esp_get_free_heap_size());

        while(file_server::net_interface_ready == false && file_server::sd_filesystem_ready == false)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        xTaskCreatePinnedToCore(file_server::Task_StartFileServer, "file_server_task_core_0", 1024*3, NULL, 0, NULL, WIFI_TASK_CORE);

        printf("Free Heap = %i\n", esp_get_free_heap_size());

    }
}

// Things.
void app_main(void)
{
    esp_sio_dev::main();
}