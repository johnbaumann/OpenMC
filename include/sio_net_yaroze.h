#ifndef SIO_NETYAROZE_H
#define SIO_NETYAROZE_H

#include <stdint.h>

namespace esp_sio_dev
{
    namespace sio
    {
        namespace net_yaroze
        {
            void Disable();
            void Enable();
            void GoIdle();
            uint8_t ProcessEvents(uint8_t DataIn);

            enum Commands : uint8_t
            {
                kAccess = 0x21, // Net Yaroze Select
                kGetID = 0x53,  // Get ID Command
                kNone = 0x00,   // No command, idle state
                kError = 0xFF   // Bad command
            };

            enum Responses : uint8_t
            {
                kIdleHighZ = 0xFF, // High default state
                kID = 0x00         // NY seems happy with anything not HI-Z
                // Additionally, NY doesn't actually care about ACK
                // NY checks for the access card until it is found
                // Shorting DATA to GND is enough to satisfy the acceess card check
            };

            extern bool SendAck;
        } // namespace net_yaroze
    }     // namespace sio
} // namespace esp_sio_dev

#endif //SIO_NETYAROZE_H