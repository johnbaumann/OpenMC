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

        extern uint8_t current_command;

        extern bool memory_card_enabled;
        extern bool pad_enabled;
        extern bool net_yaroze_enabled;

        void GoIdle();
        void Init();
        void ProcessEvents();
    } // namespace sio
} // namespace esp_sio_dev

#endif // SIO_H