#include "esp_file_helper.h"
#include "esp_sdcard.h"
#include "esp_logging.h"
#include "esp_wifi_ap.h"
#include "esp_wifi_client.h"
#include "hardware.h"
#include "sio.h"
#include "sio_memory_card.h"
#include "spi.h"

#include "npiso.h"

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
    void npiso_init(void);
}

extern DRAM_ATTR uint8_t output[4];

namespace esp_sio_dev
{
    void main(void);
    void SetupInterrupts();

    void SetupInterrupts()
    {
        // Create a task to install our interrupt handler on Core 1
        //xTaskCreatePinnedToCore(spi::InstallInterrupt, "spi_task_core_1", 1024 * 10, NULL, 1, NULL, SIO_TASK_CORE);
        spi::InstallInterrupt();
    }

    void main(void)
    {
        xTaskCreatePinnedToCore(Task_MountSDCard, "sd_card_task_core_0", 1024 * 10, NULL, 0, NULL, SD_TASK_CORE);        
        xTaskCreatePinnedToCore(wifi_ap::Task_StartWifiAP, "wifi_ap_task_core_0", 1024 * 40, NULL, 0, NULL, WIFI_TASK_CORE);
        //xTaskCreatePinnedToCore(wifi_client::Task_StartWifiClient, "wifi_client_task_core_0", 1024 * 40, NULL, 0, NULL, WIFI_TASK_CORE);


        sio::Init();       // Init the SIO state machine to a default state.
        spi::InitPins();   // Setup the pins for SPI
        spi::Enable();     // Enable SPI
        SetupInterrupts(); // Create a task to install our interrupt handler on Core 1, ESP32 likes Core 0 for WiFi
        start_app_cpu();
    }
}

// Things.
void app_main(void)
{
    esp_sio_dev::main();
}