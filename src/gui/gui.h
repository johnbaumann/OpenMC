#ifndef _GUI_H
#define _GUI_H

#include <stdint.h>

namespace esp_sio_dev
{
    namespace gui
    {

        enum DisplayState : uint8_t
        {
            kIdle = 0x00,
            kBootStatus = 0x10,
            kMainStatus = 0x11,
            kMainMenu = 0x20,
            kFileMenu = 0x21,
            kWifiMenu = 0x22
        };

        void Callback_Left(void);
        void Callback_Confirm(void);
        void Callback_Right(void);
        void Task_UpdateScreen(void *params);

        extern DisplayState display_state;
    } // gui
} // esp_sio_dev

#endif // _GUI_H