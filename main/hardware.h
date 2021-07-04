#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "driver/gpio.h"

#include <esp_log.h>

#define D0_ACK GPIO_NUM_27
#define D4_MISO GPIO_NUM_12
#define D3_MOSI GPIO_NUM_13
#define D2_SEL GPIO_NUM_14
#define D1_CLK GPIO_NUM_26

inline void SendAck()
{
    // Delay ~10uS from last clock pulse
    
    //for (int i = 0; i < 150; i++)
    // 30 = 10

    for (int i = 0; i < 25; i++)
    {
        //nop();
        if(gpio_get_level(D2_SEL) == 1)
        {
            return;
        }
    }

    //if (gpio_get_level(D2_SEL) == 1)
        //return;

    // Drop ACK LO
    gpio_set_level(D0_ACK, 0);

    // Delay ~2uS
    for (int i = 0; i < 39; i++)
    {
        nop();
    }

    // Put ACK back high
    gpio_set_level(D0_ACK, 1);
}

inline void SPI_ActiveMode()
{
    gpio_set_level(D4_MISO, 1);
    gpio_set_direction(D4_MISO, GPIO_MODE_OUTPUT);

    gpio_set_level(D0_ACK, 1);
    gpio_set_direction(D0_ACK, GPIO_MODE_OUTPUT);
}

inline void SPI_PassiveMode()
{

    //gpio_set_level(D4_MISO, 1);
    //gpio_set_level(D0_ACK, 1);

    gpio_set_pull_mode(D4_MISO, GPIO_FLOATING);
    gpio_set_direction(D4_MISO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(D0_ACK, GPIO_FLOATING);
    gpio_set_direction(D0_ACK, GPIO_MODE_INPUT);
}

inline void SPI_Disable()
{
    SPI_PassiveMode();
    // Add a status variable = disabled
}

#endif // _HARDWARE_H