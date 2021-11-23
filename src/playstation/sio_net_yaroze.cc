#include "sio_net_yaroze.h"

#include "playstation/sio.h"
#include "playstation/spi.h"

uint8_t current_command;

namespace openmc
{
    namespace sio
    {
        namespace net_yaroze
        {
            void IRAM_ATTR GoIdle()
            {
                current_command = Commands::kNone;
            }

            uint8_t IRAM_ATTR ProcessEvents(uint8_t DataIn)
            {
                uint8_t DataOut;
                bool cmdRouted = false;

                //Loop until command is properly routed
                while (!cmdRouted)
                {
                    switch (current_command)
                    {
                        // No command yet
                    case Commands::kNone:
                        // Store incoming byte as command
                        current_command = DataIn;
                        // Store flag byte in outbound variable
                        DataOut = Responses::kID;
                        // Safe to exit interpret loop
                        cmdRouted = true;
                        break;

                    case Commands::kAccess:
                        //Code not actually reached, value for debug
                        DataOut = 0xFC;
                        cmdRouted = true;
                        break;

                    case Commands::kError: // Unexpected/Unsupported command
                    default:
                        DataOut = Responses::kIdleHighZ;
                        GoIdle();
                        cmdRouted = true;
                        break;
                    }
                }

                return DataOut;
            }
        } // namespace net_yaroze
    } // namespace sio
} // namespace openmc