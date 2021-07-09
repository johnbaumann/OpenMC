#ifndef _SPI_H
#define _SPI_H

#include "hardware.h"

#include <stdint.h>
#include "driver/gpio.h"

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

extern uint8_t SPDR;
extern bool spi_selected;
extern bool spi_enabled;

void SPI_ActiveMode();
void SPI_Disable();
void SPI_Enable();
void SPI_InitPins();
void SPI_PassiveMode();

// Inlined for speed
inline uint8_t SPI_Transceive(uint8_t data_out)
{
    uint8_t data_in = 0x00;

    for (int bitPos = 0; bitPos < 8; bitPos++)
    {

        // Wait for clock to go low
        while (gpio_get_level(kCLK_Pin) > 0)
        {
            if (gpio_get_level(kSEL_Pin) == 1)
            {
                spi_selected = false;
                return 0xFF;
            }
            else
            {
                spi_selected = true;
            }
        }

        // Write bit out while clock is low
        gpio_set_level(kMISO_Pin, data_out & (1 << bitPos));

        // Wait for clock to go high
        while (gpio_get_level(kCLK_Pin) < 1)
        {
            if (gpio_get_level(kSEL_Pin) == 1)
            {
                spi_selected = false;
                return 0xFF;
            }
            else
            {
                spi_selected = true;
            }
        }

        // Store current bit state
        data_in |= gpio_get_level(kMOSI_Pin) << bitPos;
    }

    return data_in;
}

#endif // _SPI_H