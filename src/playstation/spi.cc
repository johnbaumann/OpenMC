#include "spi.h"

#include "esp_logging.h"
#include "playstation/sio.h"

#include "baremetal_core1/core0_stall.h"

#include <stdint.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern volatile void *_xt_intexc_hooks[];

namespace esp_sio_dev
{
    namespace spi
    {
        uint8_t DRAM_ATTR SPDR = 0xFF;
        bool DRAM_ATTR selected = false;
        bool DRAM_ATTR enabled = false; // AVR artifact(SPSR), contemplate the existance of this

        void IRAM_ATTR ActiveMode()
        {
            // Enable MISO and ACK pins
            REG_WRITE(GPIO_ENABLE1_W1TS_REG, (kMISO_Bitmask | kACK_Bitmask));
            // Set MISO and ACK pins HIGH
            REG_WRITE(GPIO_OUT1_W1TS_REG, (kMISO_Bitmask | kACK_Bitmask));
        }

        void IRAM_ATTR Disable()
        {
            // Set both variables to false so code aborts in whichever order happens to be reached first
            enabled = false;
            selected = false;
            PassiveMode();
        }

        void IRAM_ATTR Enable()
        {
            enabled = true;
        }

        void IRAM_ATTR InitPins()
        {
            gpio_config_t io_conf;

            // To-do: Change over to REG_WRITE ?
            // Speed not really a concern here, but inconsistent usage of GPIO is
            int allpins[] = {kACK_Pin, kCLK_Pin, kSEL_Pin, kMOSI_Pin, kMISO_Pin};

            for (int i = 0; i < 5; i++)
            {
                io_conf.intr_type = GPIO_INTR_DISABLE;
                io_conf.pin_bit_mask = 1ULL << allpins[i];
                io_conf.mode = GPIO_MODE_INPUT;
                io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
                io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
                gpio_config(&io_conf);
            }
        }

        void IRAM_ATTR PassiveMode()
        {
            // Disable MISO and ACK pins
            REG_WRITE(GPIO_ENABLE1_W1TC_REG, (kMISO_Bitmask | kACK_Bitmask));
        }

        uint32_t IRAM_ATTR InterruptHandler(uint32_t cause)
        {
            // Stall core0 to prevent interruptions
            core0_stall_start();

            // Storage for outgoing byte, start hi-Z
            uint8_t last_byte = 0xFF;

            // Interrupt triggered on falling edge, assume we're going in hot
            selected = true;

            // Do SPI loop
            while (selected)
            {
                // Send a byte, receive a byte
                SPDR = Transceive(last_byte);
                if (!selected)
                {
                    // Transceive function detected select signal HIGH, abort
                    break;
                }
                sio::ProcessEvents(); // Received byte is processed by state machine
                // SPDR is accessed directly by previous function, relic of AVR approach.
                // To-do: ? Change to last_byte = esp_sio_dev::sio::ProcessEvents();
                last_byte = SPDR;
            }

            // Clear last command
            sio::current_command = esp_sio_dev::sio::PS1_SIOCommands::Idle;
            sio::TickEventCounter();

            // Reset emulated device commands/variables
            sio::GoIdle();

            // Quietly listen on SPI
            PassiveMode();

            // Re-Enable SPI if previously set to Disable(ignore mode)
            Enable();

            // Clear interrupt status
            REG_WRITE(GPIO_STATUS1_W1TC_REG, REG_READ(GPIO_ACPU_INT1_REG));

            // Resume core0
            core0_stall_end();
            return 0;
        }

        // task runs on core 1, so the ints happen on core 1
        // so as long as we're in InterruptHandler() the system won't
        // bother us with interrupts on that core.
        // sickle the man!
        void InstallInterrupt()
        { 
            gpio_config_t io_conf;

            io_conf.intr_type = GPIO_INTR_NEGEDGE;
            io_conf.pin_bit_mask = 1ULL << kSEL_Pin;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            gpio_config(&io_conf);
            GPIO.pin[kSEL_Pin].int_ena = BIT(0);

            _xt_intexc_hooks[2] = (void *)&InterruptHandler;
            intr_matrix_set(1, ETS_GPIO_INTR_SOURCE, 19);
        }
    } // spi
} // esp_sio_dev