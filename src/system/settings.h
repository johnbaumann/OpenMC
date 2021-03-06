#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "wifi/wifi.h"

#include <stdbool.h>
#include <stdint.h>

namespace openmc
{
    namespace system
    {
        struct Settings
        {
            wifi::Mode wifi_mode;
            uint8_t ssid[33];     // SSID max length = 32 + 1 null terminator
            uint8_t password[64]; // Max passcode length = 63 + 1 null terminator. To-do: PSK 64 digit hex support?
            uint8_t contrast;
            // last file
            // load last file Y/N
            // (default) i.e. freepsxboot
            // auto/mount based on game id??
        };

        extern struct Settings settings;

        void LoadDefaultSettings();
    } // namespace system
} // namespace openmc

#endif // _SETTINGS_H