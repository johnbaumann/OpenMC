#ifndef _SSD1306_H_
#define _SSD1306_H_

// Datasheet - https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

namespace esp_sio_dev
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
            // A4/A5 - Entire Display ON
            // No arguments, Page 28
            kDisplayFromRAM = 0xA4,
            kEntireDisplayOn = 0xA5,
            // A6/A7 - Set Normal/Inverse Display
            // No arguments, Page 28
            kNormalDisplayMode = 0xA6,
            kInverseDisplayMode = 0xA7,

            // AE/AF - Set Display ON/OFF
            // No arguments, Page 28
            kDisplayOff = 0xAE,
            kDisplayOn = 0xAF,
            // ----------------------------

            // 2. Scrolling Command Table
            // ----------------------------
            // 26/27 - Continuous Horizontal Scroll Setup
            // 6 arguments, Page 28
            kScrollRight = 0x26,
            kScrollLeft = 0x27,
            // 29/2A - Continuous Vertical and Horizontal Scroll Setup
            // 5 arguments Page 29
            kScrollDownAndRight = 0x29,
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
            // ----------------------------

            // 2.1 Charge Pump Command Table
            // ----------------------------
            // 8D - Charge Pump Setting
            kSetChargePump = 0x8D,

            // ----------------------------
            // 3. Addressing Setting Command Table
            // ----------------------------
            //
            // 00~0F
            //
            //
            // 10~1F
            //
            // 20 - Set Memory Addressing Mode
            // 1 argument, Page 30
            kSetMemoryAddressingMode = 0x20,
            // 21 - Set Column Address
            // 2 arguments, Page 30
            kSetColumnAddress = 0x21,
            // 22 - Set Page Address
            // 2 arguments, Page 30
            //
            // B0~B7
            //
            // ----------------------------

            // 4. Hardware Configuration (Panel resolution & layout related) Command Table
            // ----------------------------
            //
            // 40~7F
            //
            // A0/A1 - Set Segment Re-map
            // No arguments, Page 31
            kSegmentReset = 0xA0,
            kSegmentRemap = 0xA1,
            // A8 - Set Multiplex Ratio
            // 1 argument, Page 31
            kSetMultiplexRatio = 0xA8,
            // C0/C8 - Set COM Output Scan Direction
            // No arguments, Page 31
            kCOMReset = 0xC0,
            kCOMRemap = 0xC8,
            // D3 - Set Display Offset
            // 1 argument, Page 31
            kSetDisplayOffset = 0xD3,
            // DA - Set COM Pins Hardware Configuration
            // 1 argument, Page 31
            kSetCOMPins = 0xDA,
            // ----------------------------

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
            // ----------------------------
        };

        enum ControlBytes : uint8_t
        {
            kCommandStream = 0x00,
            kDataStream = 0x40,
            kSingleCommandByte = 0x80,
            kSingleDataByte = 0xC0
        };
    }
}

#endif /* _SSD1306_H_ */