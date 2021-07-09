#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "driver/gpio.h"

#include <esp_log.h>

const gpio_num_t kMISO_Pin = GPIO_NUM_12; // To PS1 Pin 1, DATA
const gpio_num_t kMOSI_Pin = GPIO_NUM_13; // To PS1 Pin 2, CMND
const gpio_num_t kSEL_Pin = GPIO_NUM_14; // To PS1 Pin 6, ATT
const gpio_num_t kCLK_Pin = GPIO_NUM_26; // To PS1 Pin 7, CLK
const gpio_num_t kACK_Pin = GPIO_NUM_27; // To PS1 Pin 8, ACK

#define nop() __asm__ __volatile__("nop;")

inline void SendAck()
{
    // Delay ~10uS from last clock pulse

    //for (int i = 0; i < 150; i++)
    // 30 = 10

    // To-do: Check timing
    for (int i = 0; i < 25; i++)
    {
        //nop();
        if (gpio_get_level(kSEL_Pin) == 1)
        {
            return;
        }
    }

    // Drop ACK LO
    gpio_set_level(kACK_Pin, 0);

    // Delay ~2uS
    for (int i = 0; i < 39; i++)
    {
        nop();

        // To-do: Switch to this, check timing
        //if(gpio_get_level(kSEL_Pin) == 1)
        //{
        //    break;
        //}
    }

    // Put ACK back high
    gpio_set_level(kACK_Pin, 1);
}

#endif // _HARDWARE_H