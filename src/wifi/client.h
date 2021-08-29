#ifndef _CLIENT_H
#define _CLIENT_H

namespace esp_sio_dev
{
    namespace wifi
    {
        namespace client
        {
            void Task_Start(void *params);
            void Init(void);
        } // client
    } // wifi
} // esp_sio_dev

#endif // _CLIENT_H