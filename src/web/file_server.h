#ifndef _FILE_SERVER_H
#define _FILE_SERVER_H

#include <esp_err.h>

namespace openmc
{
    namespace web
    {
        namespace file_server
        {
            esp_err_t start_file_server(const char *base_path);
        } // file_server

    } // web
} // openmc

#endif // _FILE_SERVER_H