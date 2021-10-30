#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdbool.h>
#include <stdint.h>

namespace esp_sio_dev
{
    namespace storage
    {
        namespace config
        {

            enum WifiMode : uint8_t
            {
                kAcessPoint = 0,
                kClient = 1
            };

            struct Settings
            {
                WifiMode wifi_mode;
                uint8_t ssid[33];     // SSID max length = 32 + 1 null terminator
                uint8_t password[64]; // Max passcode length = 63 + 1 null terminator. To-do: PSK 64 digit hex support?
                // last file
                // load last file Y/N
                // (default) i.e. freepsxboot
                // auto/mount based on game id??
            };

            extern struct Settings settings;

            // Try loading settings file from sdcard
            bool LoadConfig();
            void LoadDefaultSettings();
        }
    }
}

#endif // _CONFIG_H
