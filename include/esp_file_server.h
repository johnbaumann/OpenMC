#ifndef _ESP_FILE_SERVER_H
#define _ESP_FILE_SERVER_H

#include <esp_err.h>


namespace esp_sio_dev
{
    namespace file_server
    {
        extern volatile bool net_interface_ready;
        extern volatile bool sd_filesystem_ready;

        //esp_err_t init_spiffs(void);
        esp_err_t start_file_server(const char *base_path);
        void Task_StartFileServer(void *params);
    } // file_server
} // esp_sio_dev

#endif // _ESP_FILE_SERVER_H