#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>

namespace esp_sio_dev
{
    namespace oled
    {
        void Clear();
        void Init(void);
        void Reset();
        void SetContrast(uint8_t contrast);
    } // oled
} // esp_sio_dev

#endif // OLED_H_