#include "flashdata.h"
#include "hardware.h"
#include "spi.h"
#include "sio.h"
#include "sio_memory_card.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <sdkconfig.h>
#include <driver/uart.h>
#include <esp_intr_alloc.h>

// Refs
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

void IRAM_ATTR InterruptHandler_SPI(void *args)
{
    // To-do: Use these for SPI timeouts
    //int clockLowWait = 0;
    //int clockHighWait = 0;
    uint8_t lastByte = 0xFF;

    // Interrupt triggered on falling edge, assume we're going in hot
    spi_selected = true;

    // Do SPI loop
    while (spi_selected)
    {
        SPDR = SPI_Transceive(lastByte);
        esp_sio_dev::sio::SIO_ProcessEvents();
        lastByte = SPDR;
    }

    // Status not idle, reset SIO/SPI state
    if (esp_sio_dev::sio::CurrentSIOCommand != esp_sio_dev::sio::PS1_SIOCommands::Idle)
    {
        // Clear last command
        esp_sio_dev::sio::CurrentSIOCommand = esp_sio_dev::sio::PS1_SIOCommands::Idle;

        // Reset emulated device commands/variables
        esp_sio_dev::sio::SIO_GoIdle();

        // Quietly listen on SPI
        SPI_PassiveMode();
        SPI_Enable();

        /*if(esp_sio_dev::sio::memory_card::GameID_Length > 0)
        {
            // Received game id, do a thing
        }*/
    }
}

static void CopyCardToRAM()
{
    for (int i = 0; i < 131072; i++)
    {
        esp_sio_dev::sio::memory_card::MemCardRAM[i] = FlashData[i];
    }
    //ets_printf("Memory card image loaded to RAM\n");
}

// task runs on core 1, so the ints happen on core 1
// so as long as we're in InterruptHandler_SPI() the system won't
// bother us with interrupts on that core.
// sickle the man!
void Task_InstallSPIInterrupt(void *params)
{
    //printf("int handler setup task on core %i\n", xPortGetCoreID());
    //printf("Free Heap = %i\n", esp_get_free_heap_size());

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
    ESP_ERROR_CHECK(gpio_set_intr_type(kSEL_Pin, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(kSEL_Pin, InterruptHandler_SPI, NULL));

    vTaskDelete(NULL); // NULL means "this task"
}

void SetupInterrupts()
{
    // Create a task to install our interrupt handler on Core 1
    xTaskCreatePinnedToCore(Task_InstallSPIInterrupt, "spi_task_core_1", 1024 * 10, NULL, 1, NULL, 1);
}

// Things.
void app_main(void)
{
    esp_sio_dev::sio::SIO_Init(); // Init the SIO state machine to a default state.
    CopyCardToRAM();            // Hacky memcpy from flash storage to ram To-Do: Load from persistant storage
    SPI_InitPins();             // Setup the pins for SPI
    SPI_Enable();
    SetupInterrupts(); // Create a task to install our interrupt handler on Core 1
    // ESP32 likes Core 0 for WiFi
}