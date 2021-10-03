#include "gui/gui.h"
#include "logging.h"
#include "oled/ssd1306.h"
#include "pins.h"
#include "playstation/sio.h"
#include "playstation/sio_memory_card.h"
#include "playstation/spi.h"
#include "storage/storage.h"
#include "touch_input/touch.h"
#include "web/file_server.h"
#include "wifi/access_point.h"
#include "wifi/client.h"
#include "wifi/wifi.h"

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

    void main(void)
    {
        sio::memory_card_enabled = true;
        sio::pad_enabled = false;
        sio::net_yaroze_enabled = false;

        // Turn off LED
        gpio_set_direction(kPin_LED, GPIO_MODE_OUTPUT);
        gpio_set_level(kPin_LED, 0);
        
        // Turn screen backlight off and delay init at power on
        // To allow voltage levels to stabilize
        gpio_set_direction(kOLEDPin_Backlight, GPIO_MODE_OUTPUT);
        gpio_set_level(kOLEDPin_Backlight, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(kOLEDPin_Backlight, 0);

        oled::Init(); // Init oled screen

        xTaskCreatePinnedToCore(gui::Task_UpdateScreen, "screen_update_task_core_0", 2048, NULL, 0, NULL, 0);

        touch_input::SetCallback_Left(gui::Callback_Left);
        touch_input::SetCallback_Confirm(gui::Callback_Confirm);
        touch_input::SetCallback_Right(gui::Callback_Right);
        xTaskCreatePinnedToCore(touch_input::Task_TouchInput, "touch_pad_input_task_core_0", 2048, NULL, 0, NULL, 0);

        storage::Init();

        // To-do: Check settings - if no default file specified, load last file. If no previous file, do nothing
        //storage::LoadCardFromFile((char *)"/sdcard/freeboot.mc", sio::memory_card::memory_card_ram);

        // Auto-write task
        xTaskCreatePinnedToCore(storage::Task_Write, "write_task_core_0", FILE_WRITE_TASK_SIZE, NULL, 0, NULL, 0);

        sio::Init();       // Init the SIO state machine to a default state.
        spi::Init();       // Setup the pins for bitbanged SPI
        spi::Enable();     // Enable SPI
        SetupInterrupts(); // Create a task to install our interrupt handler on Core 1

        core0_stall_init();
        start_app_cpu();

        // if client settings found do this
        wifi::client::Init();

        // if no settings found, start access point mode
        // Enable wifi scan mode, add option for wifi config from web server
        //wifi::access_point::Init();

        while (wifi::ready == false || storage::ready == false)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        ESP_ERROR_CHECK(web::file_server::start_file_server(storage::base_path));
       
        //xTaskCreate(tcp_server_task, "tcp_server", 4096, (void *)AF_INET, 5, NULL);

        //ESP_LOGI(kLogPrefix,"Free Heap = %i\n", esp_get_free_heap_size());
        //ESP_LOGI(kLogPrefix, "Free Internal Heap = %i\n", esp_get_free_internal_heap_size());
        ESP_LOGI(kLogPrefix, "Minimum Free Heap = %i\n", esp_get_minimum_free_heap_size());
        
    }
}

// Things.
void app_main(void)
{
    esp_sio_dev::main();
}