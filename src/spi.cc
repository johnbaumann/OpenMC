#include "spi.h"

#include <stdint.h>

#define _SPIF 7

uint8_t SPDR = 0xFF;
bool spi_selected = false;
bool spi_enabled = false;

void SPI_ActiveMode()
{
    gpio_set_level(kMISO_Pin, 1);
    gpio_set_direction(kMISO_Pin, GPIO_MODE_OUTPUT);

    gpio_set_level(kACK_Pin, 1);
    gpio_set_direction(kACK_Pin, GPIO_MODE_OUTPUT);
}

// To-do: De-duplicate function
// Artifact of AVR implementation
void SPI_Disable()
{
    spi_enabled = false;
    SPI_PassiveMode();
    // To-do: Add a status variable = disabled? SPSR?
}

void SPI_Enable()
{
    spi_enabled = true;
}

void SPI_InitPins()
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
}

void SPI_PassiveMode()
{
    //gpio_set_pull_mode(kACK_Pin, GPIO_FLOATING);
    //gpio_set_pull_mode(kMISO_Pin, GPIO_FLOATING);  
    gpio_set_direction(kACK_Pin, GPIO_MODE_INPUT);
    gpio_set_direction(kMISO_Pin, GPIO_MODE_INPUT);
    
}
