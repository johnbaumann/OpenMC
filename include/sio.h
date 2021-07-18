#ifndef SIO_H
#define SIO_H

#include <stdint.h>

namespace esp_sio_dev
{
    namespace sio
    {
        enum PS1_SIOCommands : uint8_t
        {
            Idle = 0x00,
            PAD_Access = 0x01,
            TTY_Access = 0x09,
            NY_Access = 0x21,
            MC_Access = 0x81,
            Wait = 0xFE,  // Piggy back SIO command/variable
            Ignore = 0xFF // To ignore or wait for incoming commands
        };

        extern uint8_t CurrentSIOCommand;

        extern bool bMemCardEnabled;
        extern bool bPadEnabled;
        extern bool bNYEnabled;

        void SIO_GoIdle();
        void SIO_Init();
        void SIO_ProcessEvents();
        uint8_t SIO_ProcessPadEvents(uint8_t);
    } // namespace sio
} // namespace esp_sio_dev

#endif // SIO_H