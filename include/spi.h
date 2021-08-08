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
        inline uint8_t IRAM_ATTR SPI_Transceive(uint8_t data_out)
        {
            uint8_t data_in = 0x00;

            for (int bitPos = 0; bitPos < 8; bitPos++)
            {
                // Mirror clock signal, high
                REG_WRITE(GPIO_OUT_W1TS_REG, 1 << (kCLKMIRROR_Pin));

                // Wait for clock to go low
                while (((GPIO_REG_READ(GPIO_IN1_REG) >> (kCLK_Pin - 32)) & 1U) == 1)
                {
                    // Abort if unselected
                    if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                    {
                        //ets_printf("SPI_Transceive bail 1\n");
                        selected = false;
                        return 0xFF;
                    }
                }

                // Abort if unselected
                if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                {
                    //ets_printf("SPI_Transceive bail 2\n");
                    selected = false;
                    return 0xFF;
                }
                
                // Write bit out while clock is low
                if (data_out & (1 << bitPos))
                {
                    REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << (kMISO_Pin - 32));
                }
                else
                {
                    REG_WRITE(GPIO_OUT1_W1TC_REG, 1 << (kMISO_Pin - 32));
                }

                // Mirror clock signal, low
                REG_WRITE(GPIO_OUT_W1TC_REG, 1 << (kCLKMIRROR_Pin));

                // Wait for clock to go high
                while (((GPIO_REG_READ(GPIO_IN1_REG) >> (kCLK_Pin - 32)) & 1U) == 0)
                {
                    // Abort if unselected
                    if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                    {
                        //ets_printf("SPI_Transceive bail 3\n");
                        selected = false;
                        return 0xFF;
                    }
                }

                REG_WRITE(GPIO_OUT_W1TS_REG, 1 << (kCLKMIRROR_Pin));

                // Store current bit state
                data_in |= ((GPIO_REG_READ(GPIO_IN1_REG) >> (kMOSI_Pin - 32)) & 1U) << bitPos;
            }

            return data_in;
        }
    }
}

#endif // _SPI_H