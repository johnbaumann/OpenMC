#ifndef OLED_H_
#define OLED_H_

#include <esp_err.h>
#include <stdint.h>

namespace esp_sio_dev
{
    namespace oled
    {
        typedef enum : uint8_t
        {
            k2Frames = 7,
            k3Frames = 4,
            k4Frames = 5,
            k5Frames = 0,
            k25Frames = 6,
            k64Frames = 1,
            k128Frames = 2,
            k256Frames = 3,
        } Interval;

        typedef enum : uint8_t
        {
            kPAGE0 = 0,
            kPAGE1 = 1,
            kPAGE2 = 2,
            kPAGE3 = 3,
            kPAGE4 = 4,
            kPAGE5 = 5,
            kPAGE6 = 6,
            kPAGE7 = 7
        } Page;
        
        void ClearScreen();
        void DrawBuffer();
        void Init(void);
        void ResetScreen();

        esp_err_t SetContrast(uint8_t contrast);
        esp_err_t DisplayFromRAM();
        esp_err_t EntireDisplayOn();
        esp_err_t NormalDisplayMode();
        esp_err_t InverseDisplayMode();
        esp_err_t DisplayOff();
        esp_err_t DisplayOn();
        esp_err_t ScrollRight(Page start_page, Interval interval, Page end_page);
        esp_err_t ScrollLeft(Page start_page, Interval interval, Page end_page);
        esp_err_t ScrollDownAndRight(Page start_page, Interval interval, Page end_page, uint8_t offset);
        esp_err_t ScrollDownAndLeft(Page start_page, Interval interval, Page end_page, uint8_t offset);
        esp_err_t DeactivateScroll();
        esp_err_t ActivateScroll();
        esp_err_t SetVerticalScrollArea(uint8_t num_top_fix_rows, uint8_t num_scroll_rows);
    } // oled
} // esp_sio_dev

#endif // OLED_H_