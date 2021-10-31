#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_SSID_LENGTH (32)
#define MIN_PASS_LENGTH (8)
#define MAX_PASS_LENGTH (63)
#define MIN_LINE_LENGTH (3)
#define MAX_LINE_LENGTH (256) // Longest setting name(password) w/ = sign(9) + longest value length(password)(63) + null terminator(1) = 73

namespace esp_sio_dev
{
    namespace storage
    {
        namespace config
        {
            struct Settings settings;
            // config setting defaults
            bool LoadConfig()
            {
                char file_path[] = "/sdcard/config.txt";
                FILE *config_file;

                char current_line[MAX_LINE_LENGTH]; // To-do: Determine maximum size required
                char current_setting_name[MAX_LINE_LENGTH];
                char current_setting_value[MAX_LINE_LENGTH];

                Settings temp_settings;

                char *delimiter_location;
                char *line_ending_location;

                bool found_wifi_mode = false;
                bool found_ssid = false;
                bool found_password = false;

                config_file = fopen(file_path, "r");

                if (config_file == 0)
                {
                    return false;
                }

                //while (feof(config_file) != EOF && !ferror(config_file))
                while (fgets(current_line, sizeof(current_line), config_file))
                {
                    if (strlen(current_line) > MIN_LINE_LENGTH && strlen(current_line) < MAX_LINE_LENGTH)
                    {
                        // Check if comment line, ignore if so
                        if (current_line[0] == '#')
                            continue;

                        // Look for = character
                        delimiter_location = strchr(current_line, '=');
                        if (delimiter_location == 0)
                        {
                            // Delimiter not found on this line, continue searching
                            continue;
                        }
                        // Trim string to obtain config var name
                        strncpy(current_setting_name, current_line, delimiter_location - current_line);
                        current_setting_name[delimiter_location - current_line] = '\0';

                        // Trim string to obtain config setting
                        strncpy(current_setting_value, delimiter_location + 1, strlen(current_line) - (delimiter_location - current_line));
                        current_setting_value[strlen(current_line) - (delimiter_location - current_line)] = '\0';

                        // Null terminate any line endings
                        line_ending_location = strchr(current_setting_value, '\r');
                        if (line_ending_location != 0)
                        {
                            *line_ending_location = '\0';
                        }
                        line_ending_location = strchr(current_setting_value, '\n');
                        if (line_ending_location != 0)
                        {
                            *line_ending_location = '\0';
                        }

                        if (strcmp(current_setting_name, "mode") == 0)
                        {
                            if (strcmp(current_setting_value, "client") == 0)
                            {
                                temp_settings.wifi_mode = kClient;
                                found_wifi_mode = true;
                            }
                            else if (strcmp(current_setting_value, "ap") == 0)
                            {
                                temp_settings.wifi_mode = kAcessPoint;
                                found_wifi_mode = true;
                            }
                        }
                        else if (strcmp(current_setting_name, "ssid") == 0)
                        {
                            if (strlen(current_setting_value) > 0)
                            {
                                memcpy(temp_settings.ssid, current_setting_value, strlen(current_setting_value) + 1);
                                found_ssid = true;
                            }
                        }
                        else if (strcmp(current_setting_name, "password") == 0)
                        {
                            // Check if password length is between MIN and MAX, but also allow 0 length password for open authentication
                            if ((strlen(current_setting_value) >= MIN_PASS_LENGTH && strlen(current_setting_value) <= MAX_PASS_LENGTH) || strlen(current_setting_value) == 0)
                            {
                                memcpy(temp_settings.password, current_setting_value, strlen(current_setting_value) + 1);
                                found_password = true;
                            }
                        }
                    }
                }

                if (found_wifi_mode && found_ssid && found_password)
                {
                    // Valid configuration file, copy settings
                    settings = temp_settings;
                    return true;
                }
                else
                {
                    return false;
                }
            }

            void LoadDefaultSettings()
            {
                static const uint8_t default_ssid[] = "esp-sio-client";
                static const uint8_t default_password[] = "espdevrulezdude!";

                settings.wifi_mode = kAcessPoint;
                memcpy(settings.ssid, default_ssid, sizeof(default_ssid));
                memcpy(settings.password, default_password, sizeof(default_password));
            }
        }
    }
}