#include "sio_controller.h"

#include "sio.h"
#include "spi.h"

namespace esp_sio_dev
{
    namespace sio
    {
        namespace controller
        {
            uint8_t DRAM_ATTR current_command;
            uint8_t DRAM_ATTR command_ticks;

            uint16_t DRAM_ATTR DigitalSwitches = 0xFFFF;
            uint16_t DRAM_ATTR Analog1 = 0xFFFF;
            uint16_t DRAM_ATTR Analog2 = 0xFFFF;
            uint16_t DRAM_ATTR MOT = 0x0000; //
            //uint8_t Controller_TAP = 0x00;  // Multi-player tap select, un-implemented

            void IRAM_ATTR GoIdle()
            {
                current_command = Commands::kNone;
                command_ticks = 0;
                //uint8_t Controller_TAP = 0x00;
            }

            uint8_t IRAM_ATTR ProcessEvents(uint8_t DataIn)
            {
                uint8_t DataOut = 0xFF;
                bool cmdRouted = false;

                while (!cmdRouted)
                {
                    switch (current_command)
                    {
                    // No command yet
                    case Commands::kNone:
                        // Store incoming byte as command
                        current_command = DataIn;
                        // Store low byte of Pad type
                        DataOut = lowByte(ControllerTypes::kDigitalPad);
                        // Safe to exit interpret loop
                        cmdRouted = true;
                        break;

                    case Commands::kAccess:
                        if (DataIn == Commands::kRead)
                        {
                            current_command = DataIn;
                            // Re-evaluate command
                            // cmdRouted = false;
                        }
                        else
                        {
                            current_command = Commands::kError;
                            // Re-evaluate command
                            // cmdRouted = false;
                        }
                        break;

                    case Commands::kRead:
                        DataOut = ReadCmnd_Tick(DataIn);
                        cmdRouted = true;
                        break;

                    case Commands::kError:
                        // Need data on PAD responses to invalid commands
                        DataOut = 0xFF;
                        GoIdle();
                        cmdRouted = true;
                        break;
                    }
                }

                return DataOut;
            }

            uint8_t IRAM_ATTR ReadCmnd_Tick(uint8_t DataIn)
            {
                uint8_t DataOut;

                switch (command_ticks)
                {
                    //Data is sent and received simultaneously,
                    //so the data we send isn't received by the system
                    //until the next bytes are exchanged. In this way,
                    //you have to basically respond one byte earlier than
                    //actually occurs between Slave and Master.
                    //Offset "Send" bytes noted from nocash's psx specs.

                case 0: //42h
                    //idhi
                    DataOut = highByte(ControllerTypes::kDigitalPad);
                    //DataOut = 0x18;
                    break;

                case 1: //TAP
                    //Controller_TAP = DataIn;
                    DataOut = lowByte(DigitalSwitches);
                    break;

                case 2: //MOT
                    MOT = (DataIn << 8);
                    DataOut = highByte(DigitalSwitches);
                    break;

                case 3: //MOT
                    MOT |= DataIn;
                    DataOut = 0xFC;
                    //PAD_GoIdle();
                    break;

                default:
                    DataOut = 0xFD;
                    GoIdle();
                    break;
                }

                command_ticks++;

                return DataOut;
            }
        } // namespace controller
    } // namespace sio
} // namespace esp_sio_dev