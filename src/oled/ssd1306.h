#ifndef _SSD1306_H_
#define _SSD1306_H_


#include <stdint.h>
#include <esp_err.h>

// Datasheet - https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

namespace openmc
{
    namespace oled
    {
        enum Commands : uint8_t
        {
            // 1. Fundamental Command Table
            // ----------------------------

            // 81 - Set Contrast Control
            // 1 argument, Page 28
            kSetContrast = 0x81,

            // A4 - Display from RAM(normal display mode)
            // No arguments, Page 28
            kDisplayFromRAM = 0xA4,

            // A5 - Entire Display ON
            // No arguments, Page 28
            kEntireDisplayOn = 0xA5,

            // A6/A7 - Set Normal Display Mode
            // No arguments, Page 28
            kNormalDisplayMode = 0xA6,

            // A7 - Set Inverse Display Mode
            // No arguments, Page 28
            kInverseDisplayMode = 0xA7,

            // AE - Set Display OFF
            // No arguments, Page 28
            kDisplayOff = 0xAE,

            // AF - Set Display ON
            // No arguments, Page 28
            kDisplayOn = 0xAF,


            // 2. Scrolling Command Table
            // ----------------------------

            // 26/27 - Continuous Horizontal Scroll Setup
            // 6 arguments, Page 28
            kScrollRight = 0x26,

            // 26/27 - Continuous Horizontal Scroll Setup
            kScrollLeft = 0x27,

            // 29/2A - Continuous Vertical and Horizontal Scroll Setup
            // 5 arguments Page 29
            kScrollDownAndRight = 0x29,

            // 29/2A - Continuous Vertical and Horizontal Scroll Setup
            // 5 arguments Page 29
            kScrollDownAndLeft = 0x2A,

            // 2E - Deactivate scroll
            // No arguments, Page 30
            kDeactivateScroll = 0x2E,

            // 2F - Activate scroll
            // No arguments, Page 30
            kActivateScroll = 0x2F,

            // A3 - Set Vertical Scroll Area
            // 2 arguments, Page 30
            kSetVerticalScrollArea = 0xA3,


            // 2.1 Charge Pump Command Table
            // ----------------------------

            // 8D - Charge Pump Setting
            // 1 argument, Page 62
            kSetChargePump = 0x8D,


            // ----------------------------
            // 3. Addressing Setting Command Table
            // ----------------------------
            
            // 00~0F - Set Lower Column Start Address for
            // Page Addressing Mode
            // No arguments, Page 30
            // Unimplemented/Unused

            // 10~1F - Set Higher Column Start Address for
            // Page Addressing Mode
            // No arguments, Page 30
            // Unimplemented/Unused

            // 20 - Set Memory Addressing Mode
            // 1 argument, Page 30
            kSetMemoryAddressingMode = 0x20,

            // 21 - Set Column Address
            // 2 arguments, Page 30
            kSetColumnAddress = 0x21,

            // 22 - Set Page Address
            // 2 arguments, Page 30
            kSetPageAddress = 0x22,

            // B0~B7 - Set Page Start Address
            // for Page Addressing Mode
            // Unimplemented/Unused

            // ----------------------------

            // 4. Hardware Configuration (Panel resolution & layout related) Command Table
            // ----------------------------

            // 40~7F - Set Display Start Line
            // No arguments, page 31
            // Unimplemented/Unused

            // A0 - Reset Segment Re-map (column address 0 is mapped to SEG0 (RESET))
            // No arguments, Page 31
            kSegmentReset = 0xA0,

            // A1 - Set Segment Re-map (column address 127 is mapped to SEG0)
            // No arguments, Page 31
            kSegmentRemap = 0xA1,

            // A8 - Set Multiplex Ratio
            // 1 argument, Page 31
            kSetMultiplexRatio = 0xA8,

            // C0 - Reset COM Output Scan Direction
            // No arguments, Page 31
            kCOMReset = 0xC0,

            // C8 - Set COM Output Scan Direction
            // No arguments, Page 31
            kCOMRemap = 0xC8,

            // D3 - Set Display Offset
            // 1 argument, Page 31
            kSetDisplayOffset = 0xD3,

            // DA - Set COM Pins Hardware Configuration
            // 1 argument, Page 31
            kSetCOMPins = 0xDA,


            // 5. Timing & Driving Scheme Setting Command Table
            // ----------------------------

            // D5 - Set Display Clock Divide Ratio/Oscillator Frequency
            // 1 argument, Page 32
            kSetClockFrequency = 0xD5,

            // D9 - Set Pre-charge Period
            // 1 argument, Page 32
            kSetPrechargePeriod = 0xD9,

            // DB - Set VCOMH Deselect Level
            // (VCOMH The pin for COM signal deselected voltage level.)
            // 1 argument, Page 32
            kSetPreselectLevel = 0xDB,

            // E3 NOP - Command for no operation
            // No arguments, Page 32
            kNOP = 0xE3
        };

        enum ControlBytes : uint8_t
        {
            kCommandStream = 0x00,
            kDataStream = 0x40,
            kSingleCommandByte = 0x80,
            kSingleDataByte = 0xC0
        };

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

        typedef enum : uint8_t
        {
            HorizontalMode = 0x00,
            VerticalMode = 0x01,
            PageMode = 0x03
        } MemoryAddressing;
        
        void ClearBuffer();
        void ClearScreen();
        void DrawBuffer();
        void DrawBox(int32_t x, int32_t y, uint8_t w, uint8_t h, bool color_set);
        void DrawChar(uint8_t character, int32_t x_pos, int32_t y_pos, bool clear_bg = false, bool colors_inverted = false);
        void DrawMessage(const char *message, int32_t x_pos, int32_t y_pos, bool auto_wrap, bool clear_bg = false, bool colors_inverted = false);
        int32_t GetMessageRenderedWidth(const char *message);
        void Init(void);
        void ResetScreen();

        // Built in screen functions
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
        esp_err_t SetChargePump(uint8_t setting);
        esp_err_t SetMemoryAddressingMode(MemoryAddressing mode);
        esp_err_t SetColumnAddress(uint8_t column_start, Page column_end);
        esp_err_t SetPageAddress(uint8_t page_start, Page page_end);
        esp_err_t SetSegmentReset();
        esp_err_t SetSegmentRemap();
        esp_err_t SetMultiPlexRatio(uint8_t ratio);
        esp_err_t SetCOMReset();
        esp_err_t SetCOMRemap();
    }
}

#endif // _SSD1306_H_