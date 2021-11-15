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
#include <string.h>

//#define SHOW_FPS

namespace esp_sio_dev
{
    namespace gui
    {
        DisplayState display_state = kBootStatus;
        static uint32_t last_input_timestamp;

        char item_list[14][10];
        static int menu_index = 0;
        static int menu_start_index = 0;

        void Callback_Left(void)
        {
            last_input_timestamp = system::timer::timestamp;

            switch (display_state)
            {
            case DisplayState::kMainStatus:
                break;

            case DisplayState::kMainMenu:
                if (menu_index > 0)
                {
                    menu_index--;
                    menu_start_index = 7 * (menu_index / 7);
                }
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

            default:
                break;
            }
        }

        void Callback_Confirm(void)
        {
            last_input_timestamp = system::timer::timestamp;

            switch (display_state)
            {
            case DisplayState::kMainStatus:
                display_state = kMainMenu;
                menu_index = 0;
                menu_start_index = 0;
                break;

            case DisplayState::kMainMenu:
                display_state = kMainStatus;
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

            default:
                break;
            }
        }

        void Callback_Right(void)
        {
            last_input_timestamp = system::timer::timestamp;

            switch (display_state)
            {
            case DisplayState::kMainStatus:
                break;

            case DisplayState::kMainMenu:
                if (menu_index < (13))
                {
                    menu_index++;
                    menu_start_index = 7 * (menu_index / 7);
                }
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

            default:
                break;
            }
        }

        static void DoDisplay(void)
        {
            char text_buffer[256];
            int32_t msg_y_offset = 1;

            switch (display_state)
            {

            case DisplayState::kBootStatus:
                sprintf(text_buffer, "Attempting to\nload config\nfrom SD...");
                oled::DrawMessage(text_buffer, 0, msg_y_offset, false, true, false);
                msg_y_offset += 7;
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
                oled::DrawMessage(text_buffer, 0, msg_y_offset, false, true, true);
                msg_y_offset += 7;

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
                    oled::DrawMessage(text_buffer, 0, msg_y_offset, false, true);
                    msg_y_offset += 7;
                }

                sprintf(text_buffer, "%s", storage::loaded_file_path + strlen(storage::base_path));
                oled::DrawMessage(text_buffer, 0, msg_y_offset, true, true);
                //msg_y_offset += 7;
                break;

            case DisplayState::kMainMenu:
                for (int i = menu_start_index; i < menu_start_index + 7; i++)
                {
                    sprintf(text_buffer, "%s", item_list[i]);
                    if (menu_index == i)
                    {
                        oled::DrawBox(0, msg_y_offset - 1, oled::GetMessageRenderedWidth(item_list[i]) + 2, 8, true);
                    }
                    oled::DrawMessage(text_buffer, 1, msg_y_offset, false, true, (menu_index == i));
                    msg_y_offset += 9;
                }
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

            for (int i = 0; i < 14; i++)
            {
                sprintf(item_list[i], "Test %i", i);
            }

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