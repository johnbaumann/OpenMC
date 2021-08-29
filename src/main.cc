#include "web/file_server.h"
#include "storage/storage.h"
#include "storage/sdcard.h"
#include "logging.h"
#include "wifi/access_point.h"
#include "wifi/client.h"
#include "wifi/wifi.h"
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
//#define FILE_WRITE_TASK_SIZE 1760
#define FILE_WRITE_TASK_SIZE 3096

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
        storage::Init();
        storage::LoadCardFromFile((char *)"/sdcard/freeboot.mc", sio::memory_card::memory_card_ram);
        xTaskCreatePinnedToCore(storage::Task_Write, "write_task_core_0", FILE_WRITE_TASK_SIZE, NULL, 0, NULL, 0);

        sio::Init();       // Init the SIO state machine to a default state.
        spi::InitPins();   // Setup the pins for bitbanged SPI
        spi::Enable();     // Enable SPI
        SetupInterrupts(); // Create a task to install our interrupt handler on Core 1, ESP32 likes Core 0 for WiFi

        core0_stall_init();
        start_app_cpu();

        wifi::client::Init();

        while (wifi::ready == false || storage::ready == false)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        ESP_ERROR_CHECK(web::file_server::start_file_server("/sdcard"));

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