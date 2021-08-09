#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <driver/gpio.h>
#include <esp_log.h>

const gpio_num_t kMISO_Pin = GPIO_NUM_32; // To PS1 Pin 1, DATA - OUT
const gpio_num_t kMOSI_Pin = GPIO_NUM_34; // To PS1 Pin 2, CMND - IN
const gpio_num_t kSEL_Pin = GPIO_NUM_35;  // To PS1 Pin 6, ATT - IN
const gpio_num_t kCLK_Pin = GPIO_NUM_39;  // To PS1 Pin 7, CLK - IN
const gpio_num_t kACK_Pin = GPIO_NUM_33;  // To PS1 Pin 8, ACK - OUT
//const gpio_num_t kCLKMIRROR_Pin = GPIO_NUM_23;  // To Logic Analyzer, debugging only

// Bitmasks assume all above GPIO are 32~39, hence subtracting 32
const uint32_t kMISO_Bitmask = (1UL << (kMISO_Pin - 32));
const uint32_t kMOSI_Bitmask = (1UL << (kMOSI_Pin - 32));
const uint32_t kSEL_Bitmask = (1UL << (kSEL_Pin - 32));
const uint32_t kCLK_Bitmask = (1UL << (kCLK_Pin - 32));
const uint32_t kACK_Bitmask = (1UL << (kACK_Pin - 32));

#define nop() __asm__ __volatile__("nop;")

inline bool IRAM_ATTR SendAck()
{
    // Delay ~10uS from last clock pulse
    for (int i = 0; i < 84; i++)
    {
        if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
        {
            // Return false, ack not sent
            return false;
        }
    }

    // Drop ACK LO
    REG_WRITE(GPIO_OUT1_W1TC_REG, kACK_Bitmask);

    // Delay ~2uS
    for (int i = 0; i < 20; i++)
    {
        if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
        {
            // Device not selected anymore, break early
            // To-do: Maybe call PassiveMode here?
            break;
        }
    }

    // Put ACK back high
    REG_WRITE(GPIO_OUT1_W1TS_REG, kACK_Bitmask);

    // Return true, ack sent
    return true;
}

#endif // _HARDWARE_H