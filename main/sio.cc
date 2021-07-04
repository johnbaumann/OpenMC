#include "sio.h"

#include "flashdata.h"
#include "hardware.h"
#include "sio_memory_card.h"
#include "spi.h"

namespace VirtualMC
{
    namespace sio
    {
        byte CurrentSIOCommand = PS1_SIOCommands::Idle;

        bool bMemCardEnabled = true;

        void SIO_Init()
        {
            // Reset Memory Card commands/variables
            memory_card::GoIdle();
        }

        void IRAM_ATTR SIO_ProcessEvents()
        {
            byte DataIn = 0x00;
            byte DataOut = 0xFF;
            //bool bTempAck = false;

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
                        SendAck();
                    }
                    else
                    {
                        CurrentSIOCommand = PS1_SIOCommands::Ignore;
                        SPI_Disable();
                        //bTempAck = false;
                    }

                    break;

                default: // Bad/Unexpected/Unsupported slave select command
                    CurrentSIOCommand = PS1_SIOCommands::Ignore;
                    SPI_Disable();
                    //bTempAck = false;
                    return;
                }
                // Push outbound data to the SPI Data Register
                // Data will be transferred in the next byte pair
                SPDR = DataOut;

                // Only send ACK if slave still selected
                /*if (bTempAck)
                    SendAck();*/

                // If data is ready for card, store it.
                // This takes a bit so this is done after SPDR + ACK
                memory_card::Commit();
            }
        }
    } // namespace sio
} // namespace VirtualMC