#ifndef SIO_H
#define SIO_H

#include "custom_types.h"

namespace VirtualMC
{
    namespace sio
    {
        enum PS1_SIOCommands : byte
        {
            Idle = 0x00,
            PAD_Access = 0x01,
            TTY_Access = 0x09,
            NY_Access = 0x21,
            MC_Access = 0x81,
            Wait = 0xFE,  // Piggy back SIO command/variable
            Ignore = 0xFF // To ignore or wait for incoming commands
        };

        extern byte CurrentSIOCommand;

        extern bool bMemCardEnabled;

        void SIO_Init();
        void SIO_ProcessEvents();
        byte SIO_ProcessPadEvents(byte);
    } // namespace sio
} // namespace VirtualMC

#endif // SIO_H