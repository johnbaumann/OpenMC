#include "esp_file_helper.h"
#include "esp_sdcard.h"
#include "esp_logging.h"
#include "esp_wifi_ap.h"
#include "hardware.h"
#include "sio.h"
#include "sio_memory_card.h"
#include "spi.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <sdkconfig.h>
#include <driver/uart.h>
#include <esp_intr_alloc.h>

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
}

namespace esp_sio_dev
{
    void main(void);
    void SetupInterrupts(); 

    void SetupInterrupts()
    {
        // Create a task to install our interrupt handler on Core 1
        xTaskCreatePinnedToCore(spi::InstallInterrupt, "spi_task_core_1", 1024 * 10, NULL, 1, NULL, 1);
    }

    void main(void)
    {
        xTaskCreatePinnedToCore(Task_MountSDCard, "sd_card_task_core_0", 1024 * 10, NULL, 1, NULL, 0);
        xTaskCreatePinnedToCore(wifi_ap::Task_StartWifiAP, "wifi_ap_task_core_0", 1024 * 40, NULL, 1, NULL, 0);
        sio::Init(); // Init the SIO state machine to a default state.
        spi::InitPins();  // Setup the pins for SPI
        spi::Enable();    // Enable SPI
        ESP_LOGI(kLogPrefix, "Free Heap = %i\n", esp_get_free_heap_size());
        SetupInterrupts(); // Create a task to install our interrupt handler on Core 1, ESP32 likes Core 0 for WiFi
    }
}

// Things.
void app_main(void)
{
    esp_sio_dev::main();
}