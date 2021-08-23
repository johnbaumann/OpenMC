#include "sio.h"

#include "pins.h"
#include "sio_controller.h"
#include "sio_memory_card.h"
#include "sio_net_yaroze.h"
#include "spi.h"

#include <esp32/rom/ets_sys.h>

namespace esp_sio_dev
{
    namespace sio
    {
        uint8_t DRAM_ATTR current_command = PS1_SIOCommands::Idle;

        bool DRAM_ATTR memory_card_enabled = true;
        bool DRAM_ATTR pad_enabled = false;
        bool DRAM_ATTR net_yaroze_enabled = false;

        uint64_t DRAM_ATTR event_counter;

        void IRAM_ATTR GoIdle()
        {
            // Reset emulated device commands/variables
            controller::GoIdle();
            memory_card::GoIdle();
            net_yaroze::GoIdle();
        }

        void IRAM_ATTR Init()
        {
            event_counter = 0;
            memory_card::last_write_tick = 0;
            GoIdle();
        }

        void IRAM_ATTR ProcessEvents()
        {
            uint8_t data_in = 0x00;
            uint8_t data_out = 0xFF;

            // If ignore, do nothing
            if (current_command == PS1_SIOCommands::Ignore)
            {
                return;
            }
            else // Enter SIO Loop
            {

                // Nothing done yet, prepare for SIO/SPI
                if (current_command == PS1_SIOCommands::Idle)
                {
                    // Re-enable SPI output
                    spi::ActiveMode();

                    // Wait for SPI transmission
                    current_command = PS1_SIOCommands::Wait;
                }

                // Check SPI status register
                // Store incoming data to variable
                data_in = spi::SPDR;

                // Waiting for command, store incoming byte as command
                if (current_command == PS1_SIOCommands::Wait)
                {
                    current_command = data_in;
                }

                // Interpret incoming command
                switch (current_command)
                {

                // Console requests memory card, continue interpreting command
                case PS1_SIOCommands::MC_Access:
                    if (memory_card_enabled)
                    {
                        data_out = memory_card::ProcessEvents(data_in);
                    }
                    else
                    {
                        current_command = PS1_SIOCommands::Ignore;
                        spi::Disable();
                        return;
                    }

                    break;

                case PS1_SIOCommands::PAD_Access:
                    if (pad_enabled)
                    {
                        data_out = controller::ProcessEvents(data_in);
                    }
                    else
                    {
                        current_command = PS1_SIOCommands::Ignore;
                        spi::Disable();
                        return;
                    }
                    break;

                case PS1_SIOCommands::NY_Access:
                    if (net_yaroze_enabled)
                    {
                        data_out = net_yaroze::ProcessEvents(data_in);
                    }
                    else
                    {
                        current_command = PS1_SIOCommands::Ignore;
                        spi::Disable();
                        return;
                    }
                    break;

                default: // Bad/Unexpected/Unsupported slave select command
                    current_command = PS1_SIOCommands::Ignore;
                    spi::Disable();
                    return;
                }

                // Push outbound data to the SPI Data Register
                // Data will be transferred in the next byte pair
                spi::SPDR = data_out;
                if (spi::enabled && current_command != PS1_SIOCommands::Ignore)
                {
                    if (spi::SendAck() == false) // SendAck returns false if ack not sent, i.e. slave no longer selected
                    {
                        spi::selected = false;
                    }
                }

                // If data is ready for card, store it.
                // This takes a bit so this is done after SPDR + ACK
                memory_card::Commit();
            }
        }

        void IRAM_ATTR TickEventCounter()
        {
            event_counter++;
            // Roll-over detection
            if (event_counter == 0)
            {
                memory_card::last_write_tick = 0;
            }
        }
    } // namespace sio
} // namespace esp_sio_dev