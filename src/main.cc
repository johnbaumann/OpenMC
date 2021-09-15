#include "web/file_server.h"
#include "storage/storage.h"
#include "storage/sdcard.h"
#include "logging.h"
#include "wifi/access_point.h"
#include "wifi/client.h"
#include "wifi/wifi.h"
#include "oled/ssd1306.h"
#include "pins.h"
#include "playstation/sio.h"
#include "playstation/sio_memory_card.h"
#include "playstation/spi.h"
#include "storage/spiffs.h"
#include "touch_input/touch.h"

#include "baremetal_core1/core0_stall.h"
#include "baremetal_core1/bare_metal_app_cpu.h"

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_attr.h>
#include <esp_intr_alloc.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <sdkconfig.h>
#include <stdio.h>

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

    void Task_UpdateScreen(void *params)
    {
        // Currently maxes out at 21fps
        uint8_t target_fps = 10;
        uint8_t delay_per_frame = 1000 / target_fps;
        int32_t time_to_delay = 0;
        uint32_t render_start_time = 0;
        uint32_t render_end_time = 0;

        //uint8_t fps_count = 0;
        //uint32_t fps_counter_start = 0;
        //char fps_display[16];

        char text_buffer[256];

        while (1)
        {
            render_start_time = esp_log_timestamp();

            // if(render_start_time - fps_counter_start >= 1000)
            // {
            //     sprintf(fps_display, "%i fps", fps_count);
            //     fps_count = 0;
            //     fps_counter_start = render_end_time;
            // }

            oled::ClearBuffer();

            //oled::DrawMessage(fps_display, 0, 0);

            if (wifi::ready)
            {
                sprintf(text_buffer, "Wi-fi connected\nIP:%s", wifi::ip_address);
            }
            else
            {
                sprintf(text_buffer, "Wi-fi not ready");
            }
            oled::DrawMessage(text_buffer, 0, 0, false, true);

            sprintf(text_buffer, "Current file:\n%s", storage::loaded_file_path);
            oled::DrawMessage(text_buffer, 0, 14, true, true);

            oled::DrawBuffer();

            // fps_count++;

            render_end_time = esp_log_timestamp();

            time_to_delay = delay_per_frame - (render_end_time - render_start_time);
            if (time_to_delay > 0)
            {
                vTaskDelay(time_to_delay / portTICK_PERIOD_MS);
            }
        }

        vTaskDelete(NULL);
    }

    void main(void)
    {
        sio::memory_card_enabled = true;
        sio::pad_enabled = false;
        sio::net_yaroze_enabled = false;

        // Turn off LED
        gpio_set_direction(kPin_LED, GPIO_MODE_OUTPUT);
        gpio_set_level(kPin_LED, 0);

        oled::Init(); // Init oled screen
        xTaskCreatePinnedToCore(Task_UpdateScreen, "screen_update_task_core_0", 2048, NULL, 0, NULL, 0);
        //xTaskCreatePinnedToCore(tp_example_read_task, "touch_pad_Read_task_core_0", 2048, NULL, 0, NULL, 0);

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
        printf("Minimum Free Heap = %i\n", esp_get_minimum_free_heap_size());
        printf("Free Internal Heap = %i\n", esp_get_free_internal_heap_size());
    }
}

// Things.
void app_main(void)
{
    esp_sio_dev::main();
}