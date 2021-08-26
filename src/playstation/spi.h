#ifndef _SPI_H
#define _SPI_H

#include "pins.h"

#include <stdint.h>
#include <driver/gpio.h>
#include <esp32/rom/ets_sys.h>

// Macro borrowed from Arduino wiring.h
#define lowByte(w) ((uint8_t)((w)&0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

#define MAX_TIMEOUT_TICKS 0xFFFF // No idea how long this is, need to time/adjust

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
        void InstallInterrupt();
        void PassiveMode();
        bool IRAM_ATTR SendAck();

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

        // Inlined for speed
        inline uint8_t IRAM_ATTR Transceive(uint8_t data_out)
        {
            // Bit hacky, but, if disabled, abort up the chain
            if (!enabled)
            {
                selected = false;
                return 0xFF;
            }
            uint8_t data_in = 0x00;

            uint32_t timeout_ticks;

            for (int bitPos = 0; bitPos < 8; bitPos++)
            {
                timeout_ticks = 0;

                // Wait for clock to go low
                while (((GPIO_REG_READ(GPIO_IN1_REG) >> (kCLK_Pin - 32)) & 1U) == 1)
                {
                    // Abort if unselected
                    if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                    {
                        selected = false;
                        return 0xFF;
                    }

                    timeout_ticks++;
                    if (timeout_ticks >= MAX_TIMEOUT_TICKS)
                    {
                        selected = false;
                        return 0xFF;
                    }
                }

                // Abort if unselected - redundant check here, we just polled this in the last loop
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

                // Reset timeout counter before entering next loop cycle
                timeout_ticks = 0;

                // Wait for clock to go high
                while (((GPIO_REG_READ(GPIO_IN1_REG) >> (kCLK_Pin - 32)) & 1U) == 0)
                {
                    // Abort if unselected
                    if (((GPIO_REG_READ(GPIO_IN1_REG) >> (kSEL_Pin - 32)) & 1U) == 1)
                    {
                        selected = false;
                        return 0xFF;
                    }

                    timeout_ticks++;
                    if (timeout_ticks >= MAX_TIMEOUT_TICKS)
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