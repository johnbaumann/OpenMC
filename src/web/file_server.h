#ifndef _FILE_SERVER_H
#define _FILE_SERVER_H

#include <esp_err.h>

namespace esp_sio_dev
{
    namespace web
    {
        namespace file_server
        {
            esp_err_t start_file_server(const char *base_path);
            void Task_StartFileServer(void *params);
        } // file_server

    } // web
} // esp_sio_dev

#endif // _FILE_SERVER_H