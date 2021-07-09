#include "sio_controller.h"

#include "sio.h"
#include "spi.h"

namespace esp_sio_dev
{
    namespace sio
    {
        namespace controller
        {
            uint8_t Cur_Cmnd;
            uint8_t Cmnd_Ticks;

            uint16_t DigitalSwitches = 0xFFFF;
            uint16_t Analog1 = 0xFFFF;
            uint16_t Analog2 = 0xFFFF;
            uint16_t MOT = 0x0000; //
            //uint8_t Controller_TAP = 0x00;  // Multi-player tap select, un-implemented

            bool SendAck = true;

            void GoIdle()
            {
                Cur_Cmnd = Commands::kNone;
                Cmnd_Ticks = 0;
                SendAck = true;
                //uint8_t Controller_TAP = 0x00;
            }

            uint8_t ProcessEvents(uint8_t DataIn)
            {
                uint8_t DataOut = 0xFF;
                bool cmdRouted = false;

                while (!cmdRouted)
                {
                    switch (Cur_Cmnd)
                    {
                    // No command yet
                    case Commands::kNone:
                        // Store incoming byte as command
                        Cur_Cmnd = DataIn;
                        // Store low byte of Pad type
                        DataOut = lowByte(ControllerTypes::kDigitalPad);
                        // Safe to exit interpret loop
                        cmdRouted = true;
                        break;

                    case Commands::kAccess:
                        if (DataIn == Commands::kRead)
                        {
                            Cur_Cmnd = DataIn;
                            // Re-evaluate command
                            // cmdRouted = false;
                        }
                        else
                        {
                            Cur_Cmnd = Commands::kError;
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

            uint8_t ReadCmnd_Tick(uint8_t DataIn)
            {
                uint8_t DataOut;

                SendAck = true; // Default true;

                switch (Cmnd_Ticks)
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
                    SendAck = false;
                    break;

                default:
                    DataOut = 0xFD;
                    GoIdle();
                    break;
                }

                Cmnd_Ticks++;

                return DataOut;
            }
        } // namespace controller
    } // namespace sio
} // namespace esp_sio_dev