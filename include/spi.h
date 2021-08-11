#ifndef _SPI_H
#define _SPI_H

#include "hardware.h"

#include <stdint.h>
#include <driver/gpio.h>
#include <esp32/rom/ets_sys.h>

#define lowByte(w) ((uint8_t)((w)&0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

namespace esp_sio_dev
{
    namespace spi
    {
        extern uint8_t SPDR;
        extern bool selected;
        extern bool enabled;

        uint32_t IRAM_ATTR InterruptHandler(uint32_t cause);
        void ActiveMode();
        void Disable();
        void Enable();
        void InitPins();
        void PassiveMode();
        void InstallInterrupt();

        // Inlined for speed
        inline uint8_t IRAM_ATTR Transceive(uint8_t data_out)
        {
            // Bit hacky, but, if disabled, abort up the chain
            if(!enabled)
            {
                selected = false;
                return 0xFF;
            }
            uint8_t data_in = 0x00;

            for (int bitPos = 0; bitPos < 8; bitPos++)
            {
                // Wait for clock to go low
                while (((GPIO_REG_READ(GPIO_IN1_REG) >> (kCLK_Pin - 32)) & 1U) == 1)
                {
                    // Abort if unselected
                    if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                    {
                        selected = false;
                        return 0xFF;
                    }
                }

                // Abort if unselected
                if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                {
                    selected = false;
                    return 0xFF;
                }
                
                // Write bit out while clock is low
                if (data_out & (1 << bitPos))
                {
                    // if bit is set, write to the bit set register
                    REG_WRITE(GPIO_OUT1_W1TS_REG, kMISO_Bitmask);
                }
                else
                {
                    // if bit is clear, write to the bit clear register
                    REG_WRITE(GPIO_OUT1_W1TC_REG, kMISO_Bitmask);
                }

                // Wait for clock to go high
                while (((GPIO_REG_READ(GPIO_IN1_REG) >> (kCLK_Pin - 32)) & 1U) == 0)
                {
                    // Abort if unselected
                    if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                    {
                        selected = false;
                        return 0xFF;
                    }
                }

                // Store current bit state
                data_in |= ((GPIO_REG_READ(GPIO_IN1_REG) >> (kMOSI_Pin - 32)) & 1U) << bitPos;
            }

            return data_in;
        }
    }
}

#endif // _SPI_H