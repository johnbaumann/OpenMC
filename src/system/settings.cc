#include "system/settings.h"

#include <string.h>

namespace openmc
{
    namespace system
    {
        struct Settings settings;

        void LoadDefaultSettings()
        {
            static const uint8_t default_ssid[] = "openmc";
            static const uint8_t default_password[] = "ps1devrulezdude!";

            settings.wifi_mode = wifi::Mode::kAcessPoint;
            memcpy(settings.ssid, default_ssid, sizeof(default_ssid));
            memcpy(settings.password, default_password, sizeof(default_password));
            settings.contrast = 0xFF;
        }
    } // namespace system
} // namespace openmc
