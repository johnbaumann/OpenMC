#include "spi.h"

#include "esp_logging.h"
#include "sio.h"

#include "core0_stall.h"

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
        bool DRAM_ATTR enabled = false;
        gpio_config_t DRAM_ATTR io_conf;

        void IRAM_ATTR ActiveMode()
        {
            /*io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.pin_bit_mask = 1ULL << kMISO_Pin;
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            gpio_config(&io_conf);
            REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (kMISO_Pin - 32));

            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.pin_bit_mask = 1ULL << kACK_Pin;
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            gpio_config(&io_conf);
            REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (kACK_Pin - 32));*/

            GPIO.enable1_w1ts.data = (0x1 << (kMISO_Pin - 32));
            REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (kMISO_Pin - 32));
            GPIO.enable1_w1ts.data = (0x1 << (kACK_Pin - 32));
            REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (kACK_Pin - 32));

            /*gpio_set_level(kMISO_Pin, 1);
            gpio_set_direction(kMISO_Pin, GPIO_MODE_OUTPUT);

            gpio_set_level(kACK_Pin, 1);
            gpio_set_direction(kACK_Pin, GPIO_MODE_OUTPUT);*/
        }

        void IRAM_ATTR Disable()
        {
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

            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.pin_bit_mask = 1ULL << kCLKMIRROR_Pin;
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            gpio_config(&io_conf);

            //gpio_set_pull_mode(kCLK_Pin, GPIO_PULLUP_ONLY);
            //gpio_set_pull_mode(kSEL_Pin, GPIO_PULLUP_ONLY);
            //gpio_set_pull_mode(kMOSI_Pin, GPIO_PULLUP_ONLY);

            //gpio_set_pull_mode(kACK_Pin, GPIO_FLOATING);
            //gpio_set_pull_mode(kMISO_Pin, GPIO_FLOATING);

            //gpio_set_direction(kCLKMIRROR_Pin, GPIO_MODE_OUTPUT);
        }

        void IRAM_ATTR PassiveMode()
        {
            /*io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.pin_bit_mask = 1ULL << kMISO_Pin;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            gpio_config(&io_conf);

            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.pin_bit_mask = 1ULL << kACK_Pin;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            gpio_config(&io_conf);*/
            //gpio_set_direction(kACK_Pin, GPIO_MODE_INPUT);
            //gpio_set_direction(kMISO_Pin, GPIO_MODE_INPUT);

            GPIO.enable1_w1tc.data = (0x1 << (kMISO_Pin - 32));
            GPIO.enable1_w1tc.data = (0x1 << (kACK_Pin - 32));
        }

        uint32_t IRAM_ATTR InterruptHandler(uint32_t cause)
        {
            core0_stall_start();
            uint32_t low_io = GPIO.acpu_int;        // GPIO0~31 APP CPU interrupt status
            uint32_t high_io = GPIO.acpu_int1.intr; //GPIO32~39 APP CPU interrupt status

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

            // Clear last command
            esp_sio_dev::sio::current_command = esp_sio_dev::sio::PS1_SIOCommands::Idle;

            // Reset emulated device commands/variables
            esp_sio_dev::sio::GoIdle();

            // Quietly listen on SPI
            PassiveMode();
            Enable();

            //GPIO.status1_w1tc.intr_st = GPIO.acpu_int1.intr;
            if (high_io)
                GPIO.status1_w1tc.intr_st = high_io;
            if (low_io)
                GPIO.status_w1tc = low_io;

            core0_stall_end();    
            return 0;
        }

        // task runs on core 1, so the ints happen on core 1
        // so as long as we're in InterruptHandler() the system won't
        // bother us with interrupts on that core.
        // sickle the man!
        void InstallInterrupt()
        {
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