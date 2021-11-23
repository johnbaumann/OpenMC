#ifndef SIO_CONTROLLER_H
#define SIO_CONTROLLER_H

#include <stdint.h>

namespace openmc
{
    namespace sio
    {
        namespace controller
        {
            enum Commands : uint8_t
            {
                kAccess = 0x01, // Pad Select
                kRead = 0x42,   // Read Command
                kNone = 0x00,   // No command, idle state
                kError = 0xFF   // Bad command
            };

            // https://problemkaputt.de/psx-spx.htm#controllersandmemorycards
            enum ControllerTypes : uint16_t
            {
                kMouse = 0x5A12,          // (two button mouse)
                kNegCon = 0x5A23,         // (steering twist/wheel/paddle)
                kKonamiLightgun = 0x5A31, // (IRQ10-type)
                kDigitalPad = 0x5A41,     // (or analog pad/stick in digital mode; LED=Off)
                kAnalog_Stick = 0x5A53,   // (or analog pad in "flight mode"; LED=Green)
                kNamco_Lightgun = 0x5A63, // (Cinch-type)
                kAnalog_Pad = 0x5A73,     // (in normal analog mode; LED=Red)
                kMultitap = 0x5A80,       // (multiplayer adaptor) (when activated)
                kJogcon = 0x5AE3,         // (steering dial)
                kConfig_Mode = 0x5AF3,    // (when in config mode; see rumble command 43h)
                kIdleHighZ = 0xFFFF       // (no controller connected, pins floating High-Z)
            };

            extern uint16_t DigitalSwitches;
            extern uint16_t Analog1;
            extern uint16_t Analog2;

            void Enable();
            void Disable();
            void GoIdle();
            uint8_t ProcessEvents(uint8_t);
            uint8_t ReadCmnd_Tick(uint8_t);
        } // namespace controller
    }     // namespace sio
} // namespace openmc

#endif //SIO_CONTROLLER_H