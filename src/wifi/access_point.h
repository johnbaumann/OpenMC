#ifndef _ACCESS_POINT_H
#define _ACCESS_POINT_H

namespace esp_sio_dev
{
    namespace wifi
    {
        namespace access_point
        {
            void Init(void);
            void Task_Start(void *params);
        } // access_point

    } // wifi
} // esp_sio_dev

#endif // _ACCESS_POINT_H