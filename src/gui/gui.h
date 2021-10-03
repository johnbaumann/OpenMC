#ifndef _GUI_H
#define _GUI_H

namespace esp_sio_dev
{
    namespace gui
    {
        void Callback_Left(void);
        void Callback_Confirm(void);
        void Callback_Right(void);
        void Task_UpdateScreen(void *params);
    } // gui
} // esp_sio_dev

#endif // _GUI_H