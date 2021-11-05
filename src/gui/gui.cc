#include "gui/gui.h"

#include "oled/ssd1306.h"
#include "storage/config.h"
#include "storage/storage.h"
#include "system/timer.h"
#include "wifi/wifi.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdio.h>

//#define SHOW_FPS

namespace esp_sio_dev
{
    namespace gui
    {
        DisplayState display_state = kBootStatus;
        static uint32_t last_input_timestamp;

        void Callback_Left(void)
        {
            last_input_timestamp = system::timer::timestamp;
            printf("Callback Left\n");
        }

        void Callback_Confirm(void)
        {
            last_input_timestamp = system::timer::timestamp;
            printf("Callback Confirm\n");
        }

        void Callback_Right(void)
        {
            last_input_timestamp = system::timer::timestamp;
            printf("Callback Right\n");
        }

        static void DoDisplay(void)
        {
            char text_buffer[256];
            int32_t msg_y_offset = 0;

            switch (display_state)
            {

            case DisplayState::kBootStatus:
                sprintf(text_buffer, "Attempting to\nload config\nfrom SD...");
                oled::DrawMessage(text_buffer, 0, 0, false, true, false);
                break;

            case DisplayState::kMainStatus:

                if (storage::config::settings.wifi_mode == wifi::Mode::kClient)
                {
                    sprintf(text_buffer, "Wifi-Client");
                }
                else if (storage::config::settings.wifi_mode == wifi::Mode::kAcessPoint)
                {
                    sprintf(text_buffer, "Wifi-AP");
                }
                else
                {
                    sprintf(text_buffer, "Wifi Disabled");
                }

                oled::DrawMessage(text_buffer, 0, msg_y_offset += 7, false, true, true);

                if (storage::config::settings.wifi_mode != wifi::Mode::kNone)
                {
                    if (wifi::ready)
                    {
                        sprintf(text_buffer, "IP %s", wifi::ip_address);
                    }
                    else
                    {
                        // To-do: Grab status from ap/client module, alert to connection failure
                        sprintf(text_buffer, "Connecting...");
                    }
                    oled::DrawMessage(text_buffer, 0, msg_y_offset += 7, false, true);
                }

                sprintf(text_buffer, "Current file:\n%s", storage::loaded_file_path);
                oled::DrawMessage(text_buffer, 0, msg_y_offset += 7, true, true);

                break;

            case DisplayState::kMainMenu:
                //
                break;

            case DisplayState::kFileMenu:
                //
                break;

            case DisplayState::kWifiMenu:
                //
                break;

            case DisplayState::kIdle:
                //
                break;
            }

            msg_y_offset = 0;
        }

        void Task_UpdateScreen(void *params)
        {
            // Currently maxes around 21fps
            uint8_t target_fps = 10;
            uint8_t delay_per_frame = 1000 / target_fps;
            int32_t time_to_delay = 0;
            uint32_t render_start_time = 0;
            uint32_t render_end_time = 0;

#ifdef SHOW_FPS
            uint8_t fps_count = 0;
            uint32_t fps_counter_start = 0;
            char fps_display[16];
#endif

            while (1)
            {
                render_start_time = system::timer::timestamp;

                oled::ClearBuffer();

#ifdef SHOW_FPS
                if (render_start_time - fps_counter_start >= 1000)
                {
                    sprintf(fps_display, "%i fps", fps_count);
                    fps_count = 0;
                    fps_counter_start = render_end_time;
                }
                oled::DrawMessage(fps_display, 0, 0, false, false, false);
                fps_count++;
#endif

                DoDisplay();

                oled::DrawBuffer();

                render_end_time = system::timer::timestamp;
                time_to_delay = delay_per_frame - (render_end_time - render_start_time);
                if (time_to_delay > 0)
                {
                    vTaskDelay(time_to_delay / portTICK_PERIOD_MS);
                }
            }

            vTaskDelete(NULL);
        }
    } // gui
} // esp_sio_dev