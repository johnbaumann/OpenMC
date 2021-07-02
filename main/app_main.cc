
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "esp_intr_alloc.h"

#include "custom_types.h"
#include "flashdata.h"
#include "hardware.h"
#include "spi.h"

#include "sio.h"
#include "sio_memory_card.h"

// Refs
// https://github.com/nkolban/esp32-snippets/blob/master/gpio/interrupts/test_intr.c
// https://esp32.com/viewtopic.php?t=13432
// https://github.com/nkolban/esp32-snippets/blob/master/gpio/interrupts/test_intr.c
// not using pins >= 34 as pull ups
// https://www.esp32.com/viewtopic.php?t=1183
// rama
//

extern "C"
{

    static void InitPins();

    // TODO: use them for timeouts
    //int clockLowWait = 0;
    //int clockHighWait = 0;

    inline byte Trancieve(char data_out)
    {
        byte data_in = 0x00;

        for (int bitPos = 0; bitPos < 8; bitPos++)
        {

            // Wait for clock to go low
            while (gpio_get_level(D1_CLK) > 0)
            {
                if (gpio_get_level(D2_SEL) == 1)
                    return 0xFF;
            }

            // Write bit out while clock is low
            gpio_set_level(D4_MISO, data_out & (1 << bitPos));

            // Wait for clock to go high
            while (gpio_get_level(D1_CLK) < 1)
            {
                if (gpio_get_level(D2_SEL) == 1)
                    return 0xFF;
            }

            // Store current bit state
            data_in |= gpio_get_level(D3_MOSI) << bitPos;
        }

        //SendAck();

        return data_in;
    }

    void IRAM_ATTR intHandler(void *args)
    {
        char lastByte = 0xFF;

        if (gpio_get_level(D2_SEL) == 1)
            return;

        SPI_ActiveMode();
        while (gpio_get_level(D2_SEL) == 0)
        {
            SPDR = Trancieve(lastByte);
            VirtualMC::sio::SIO_ProcessEvents();
            lastByte = SPDR;
        }

        if (VirtualMC::sio::CurrentSIOCommand != VirtualMC::sio::PS1_SIOCommands::Idle)
        {
            // Clear last command
            VirtualMC::sio::CurrentSIOCommand = VirtualMC::sio::PS1_SIOCommands::Idle;

            // Reset Memory Card commands/variables
            VirtualMC::sio::memory_card::GoIdle();
            SPI_PassiveMode();
        }
    }

    static void InitPins()
    {

        int allpins[] = {D0_ACK, D1_CLK, D2_SEL, D3_MOSI, D4_MISO};

        for (int i = 0; i < 5; i++)
        {
            gpio_reset_pin((gpio_num_t)allpins[i]);
            gpio_set_direction((gpio_num_t)allpins[i], GPIO_MODE_INPUT);
        }

        gpio_set_pull_mode(D1_CLK, GPIO_PULLUP_ONLY);
        gpio_set_pull_mode(D2_SEL, GPIO_PULLUP_ONLY);
        gpio_set_pull_mode(D3_MOSI, GPIO_PULLUP_ONLY);

        gpio_set_pull_mode(D0_ACK, GPIO_FLOATING);
        gpio_set_direction(D0_ACK, GPIO_MODE_OUTPUT);
        gpio_set_pull_mode(D4_MISO, GPIO_FLOATING);
        gpio_set_direction(D4_MISO, GPIO_MODE_OUTPUT);
    }

    // task runs on core 0, so the ints happen on core 0
    // so as long as we're in intHandler() the system won't
    // bother us with interrupts on that core.
    void IRAM_ATTR myTask(void *params)
    {

        ets_printf("int handler setup task on core...\n", xPortGetCoreID());

        ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
        ESP_ERROR_CHECK(gpio_set_intr_type(D2_SEL, GPIO_INTR_NEGEDGE));
        ESP_ERROR_CHECK(gpio_isr_handler_add(D2_SEL, intHandler, NULL));

        vTaskDelete(NULL); // NULL means "this task"
    }

    // Do the thing on core 0
    void IRAM_ATTR SetupInterrupts()
    {

        xTaskCreatePinnedToCore(myTask, "spi_task_core_0", 1024 * 10, NULL, 1, NULL, 0);
    }

    // Things.
    void IRAM_ATTR app_main(void)
    {

        InitPins();

        SetupInterrupts();

        while (1)
        {
            // make the watchdog happy. the fussy prick
            vTaskDelay(1);
        }
    }
}