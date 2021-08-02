#include "spi.h"

#include "esp_logging.h"
#include "sio.h"

#include <stdint.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace esp_sio_dev
{
    namespace spi
    {
        uint8_t SPDR = 0xFF;
        bool selected = false;
        bool enabled = false;

        void ActiveMode()
        {
            gpio_set_level(kMISO_Pin, 1);
            gpio_set_direction(kMISO_Pin, GPIO_MODE_OUTPUT);

            gpio_set_level(kACK_Pin, 1);
            gpio_set_direction(kACK_Pin, GPIO_MODE_OUTPUT);
        }

        void Disable()
        {
            enabled = false;
            selected = false;
            PassiveMode();
        }

        void Enable()
        {
            enabled = true;
        }

        void InitPins()
        {

            int allpins[] = {kACK_Pin, kCLK_Pin, kSEL_Pin, kMOSI_Pin, kMISO_Pin};

            for (int i = 0; i < 5; i++)
            {
                gpio_reset_pin((gpio_num_t)allpins[i]);
                gpio_set_direction((gpio_num_t)allpins[i], GPIO_MODE_INPUT);
            }

            gpio_set_pull_mode(kCLK_Pin, GPIO_PULLUP_ONLY);
            gpio_set_pull_mode(kSEL_Pin, GPIO_PULLUP_ONLY);
            gpio_set_pull_mode(kMOSI_Pin, GPIO_PULLUP_ONLY);

            gpio_set_pull_mode(kACK_Pin, GPIO_FLOATING);
            gpio_set_pull_mode(kMISO_Pin, GPIO_FLOATING);
            
            gpio_set_direction(kCLKMIRROR_Pin, GPIO_MODE_OUTPUT);
        }

        void PassiveMode()
        {
            gpio_set_direction(kACK_Pin, GPIO_MODE_INPUT);
            gpio_set_direction(kMISO_Pin, GPIO_MODE_INPUT);
        }

        void IRAM_ATTR InterruptHandler(void *args)
        {
            // To-do: Use these for SPI timeouts
            //int clockLowWait = 0;
            //int clockHighWait = 0;

            uint8_t lastByte = 0xFF;

            // Interrupt triggered on falling edge, assume we're going in hot
            selected = true;

            // Do SPI loop
            while (selected)
            {
                SPDR = SPI_Transceive(lastByte);
                if (!selected)
                {
                    break;
                }
                esp_sio_dev::sio::ProcessEvents();
                lastByte = SPDR;
            }

            // Status not idle, reset SIO/SPI state
            //if (esp_sio_dev::sio::current_command != esp_sio_dev::sio::PS1_SIOCommands::Idle)
            //{
            // Clear last command
            esp_sio_dev::sio::current_command = esp_sio_dev::sio::PS1_SIOCommands::Idle;

            // Reset emulated device commands/variables
            esp_sio_dev::sio::GoIdle();

            // Quietly listen on SPI
            PassiveMode();
            Enable();

            /*if(esp_sio_dev::sio::memory_card::game_id_length > 0)
        {
            // Received game id, do a thing
        }*/
            //}
        }

        // task runs on core 1, so the ints happen on core 1
        // so as long as we're in InterruptHandler() the system won't
        // bother us with interrupts on that core.
        // sickle the man!
        void InstallInterrupt(void *params)
        {
            ESP_LOGI(kLogPrefix, "int handler setup task on core %i\n", xPortGetCoreID());

            ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
            ESP_ERROR_CHECK(gpio_set_intr_type(kSEL_Pin, GPIO_INTR_NEGEDGE));
            ESP_ERROR_CHECK(gpio_isr_handler_add(kSEL_Pin, InterruptHandler, NULL));

            vTaskDelete(NULL); // NULL means "this task"
        }
    } // spi
} // esp_sio_dev