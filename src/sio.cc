#include "sio.h"

#include "hardware.h"
#include "sio_controller.h"
#include "sio_memory_card.h"
#include "sio_net_yaroze.h"
#include "spi.h"

namespace esp_sio_dev
{
    namespace sio
    {
        uint8_t CurrentSIOCommand = PS1_SIOCommands::Idle;

        bool bMemCardEnabled = true;
        bool bPadEnabled = false;
        bool bNYEnabled = false;

        void SIO_GoIdle()
        {
            // Reset emulated device commands/variables
            controller::GoIdle();
            memory_card::GoIdle();
            net_yaroze::GoIdle();
        }

        void SIO_Init()
        {
            SIO_GoIdle();
        }

        void IRAM_ATTR SIO_ProcessEvents()
        {
            uint8_t DataIn = 0x00;
            uint8_t DataOut = 0xFF;

            // If ignore, do nothing
            if (CurrentSIOCommand == PS1_SIOCommands::Ignore)
            {
                return;
            }
            else // Enter SIO Loop
            {
                // Nothing done yet, prepare for SIO/SPI
                if (CurrentSIOCommand == PS1_SIOCommands::Idle)
                {
                    // Re-enable SPI output
                    SPI_ActiveMode();

                    // Wait for SPI transmission
                    CurrentSIOCommand = PS1_SIOCommands::Wait;
                }

                // Check SPI status register
                // Store incoming data to variable
                DataIn = SPDR;

                // Waiting for command, store incoming byte as command
                if (CurrentSIOCommand == PS1_SIOCommands::Wait)
                    CurrentSIOCommand = DataIn;

                // Interpret incoming command
                switch (CurrentSIOCommand)
                {

                // Console requests memory card, continue interpreting command
                case PS1_SIOCommands::MC_Access:
                    if (bMemCardEnabled)
                    {
                        // Byte exchange is offset by one
                        // This offsets the ACK signal accordingly
                        DataOut = memory_card::ProcessEvents(DataIn);
                    }
                    else
                    {
                        CurrentSIOCommand = PS1_SIOCommands::Ignore;
                        SPI_Disable();
                        return;
                    }

                    break;

                case PS1_SIOCommands::PAD_Access:
                    if (bPadEnabled)
                    {
                        DataOut = controller::ProcessEvents(DataIn);
                    }
                    else
                    {
                        CurrentSIOCommand = PS1_SIOCommands::Ignore;
                        SPI_Disable();
                        return;
                    }
                    break;

                case PS1_SIOCommands::NY_Access:
                    if (bNYEnabled)
                    {
                        DataOut = net_yaroze::ProcessEvents(DataIn);
                    }
                    else
                    {
                        CurrentSIOCommand = PS1_SIOCommands::Ignore;
                        SPI_Disable();
                        return;
                    }
                    break;

                default: // Bad/Unexpected/Unsupported slave select command
                    CurrentSIOCommand = PS1_SIOCommands::Ignore;
                    SPI_Disable();
                    return;
                }
                // Push outbound data to the SPI Data Register
                // Data will be transferred in the next byte pair
                SPDR = DataOut;
                if(SendAck() == false) // SendAck returns false if ack not sent, i.e. slave no longer selected
                    spi_selected = false;

                // If data is ready for card, store it.
                // This takes a bit so this is done after SPDR + ACK
                memory_card::Commit();
            }
        }
    } // namespace sio
} // namespace esp_sio_dev