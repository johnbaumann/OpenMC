#ifndef _PINS_H
#define _PINS_H

#include <driver/gpio.h>

// To-do: Assign definitions to respective namespaces

// General
const gpio_num_t kPin_LED = GPIO_NUM_25;
// General

// OLED
const gpio_num_t kOLEDPin_SDA = GPIO_NUM_4;        // Serial Data
const gpio_num_t kOLEDPin_SCL = GPIO_NUM_15;       // Serial Clock
const gpio_num_t kOLEDPin_RST = GPIO_NUM_16;       // Reset
const gpio_num_t kOLEDPin_Backlight = GPIO_NUM_21; // Backlight control
// OLED

// SD Card
const gpio_num_t kSDPin_CS = GPIO_NUM_5;    // Chip Select - IN / CD
const gpio_num_t kSDPin_CLK = GPIO_NUM_18;  // Clock - OUT / SCK
const gpio_num_t kSDPin_MISO = GPIO_NUM_19; // Master In Slave Out - IN / D0
const gpio_num_t kSDPin_MOSI = GPIO_NUM_23; // Master Out Slave In - OUT / CMD
// SD Card

// Playstation
const gpio_num_t kMISO_Pin = GPIO_NUM_32; // To PS1 Pin 1, DATA - OUT
const gpio_num_t kMOSI_Pin = GPIO_NUM_34; // To PS1 Pin 2, CMND - IN
const gpio_num_t kSEL_Pin = GPIO_NUM_35;  // To PS1 Pin 6, ATT - IN
const gpio_num_t kCLK_Pin = GPIO_NUM_39;  // To PS1 Pin 7, CLK - IN
const gpio_num_t kACK_Pin = GPIO_NUM_33;  // To PS1 Pin 8, ACK - OUT
// Playstation

// Bitmasks, for addressing pins via registers
const uint32_t kMISO_Bitmask = (1UL << (kMISO_Pin - 32));
const uint32_t kMOSI_Bitmask = (1UL << (kMOSI_Pin - 32));
const uint32_t kSEL_Bitmask = (1UL << (kSEL_Pin - 32));
const uint32_t kCLK_Bitmask = (1UL << (kCLK_Pin - 32));
const uint32_t kACK_Bitmask = (1UL << (kACK_Pin - 32));
// Bitmasks, for addressing pins via registers

#endif // _PINS_H