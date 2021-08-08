#ifndef _ESP_WIFI_AP_H
#define _ESP_WIFI_AP_H

namespace esp_sio_dev
{
    namespace wifi_ap
    {
        void Wifi_Init_SoftAP(void);
        void Task_StartWifiAP(void *params);
        
    } // wifi_ap
} // esp_sio_dev

#endif // _ESP_WIFI_AP_H