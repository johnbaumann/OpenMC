#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <driver/gpio.h>
#include <esp_log.h>

const gpio_num_t kMISO_Pin = GPIO_NUM_32; // To PS1 Pin 1, DATA - OUT
const gpio_num_t kMOSI_Pin = GPIO_NUM_34; // To PS1 Pin 2, CMND - IN
const gpio_num_t kSEL_Pin = GPIO_NUM_35;  // To PS1 Pin 6, ATT - IN
const gpio_num_t kCLK_Pin = GPIO_NUM_39;  // To PS1 Pin 7, CLK - IN
const gpio_num_t kCLKMIRROR_Pin = GPIO_NUM_23;  // To PS1 Pin 7, CLK - IN
const gpio_num_t kACK_Pin = GPIO_NUM_33;  // To PS1 Pin 8, ACK - OUT


/*
// ADC2 Pins, possible wifi interference? Doesn't seem to be the issue
const gpio_num_t kMISO_Pin = GPIO_NUM_32; // To PS1 Pin 1, DATA - OUT
const gpio_num_t kMOSI_Pin = GPIO_NUM_33; // To PS1 Pin 2, CMND - IN
const gpio_num_t kSEL_Pin = GPIO_NUM_25;  // To PS1 Pin 6, ATT - IN
const gpio_num_t kCLK_Pin = GPIO_NUM_26;  // To PS1 Pin 7, CLK - IN
const gpio_num_t kACK_Pin = GPIO_NUM_27;  // To PS1 Pin 8, ACK - OUT
*/

#define nop() __asm__ __volatile__("nop;")

inline bool SendAck()
{
    // Delay ~10uS from last clock pulse

    //for (int i = 0; i < 150; i++)
    // 30 = 10

    // To-do: Check timing
    for (int i = 0; i < 30; i++)
    {
        //nop();
        if (gpio_get_level(kSEL_Pin) == 1)
        {
            // Return false, ack not sent
            return false;
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

    // Return true, ack sent
    return true;
}

#endif // _HARDWARE_H