#include "wifi/access_point.h"

#include "wifi/wifi.h"
#include "web/file_server.h"
#include "logging.h"

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include "nvs_flash.h"

#include <lwip/err.h>
#include <lwip/sys.h>

#define EXAMPLE_ESP_WIFI_SSID "esp-sio-dev"
#define EXAMPLE_ESP_WIFI_PASS "psxdevrulezd00d!"
#define EXAMPLE_ESP_WIFI_CHANNEL 5
#define EXAMPLE_MAX_STA_CONN 1

namespace esp_sio_dev
{
    namespace wifi
    {
        namespace access_point
        {
            void Task_Start(void *params)
            {
                ESP_LOGI(kLogPrefix, "Wifi AP setup task on core %i\n", xPortGetCoreID());
                Init();
                vTaskDelete(NULL); // NULL means "this task"
            }

            static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
            {
                if (event_id == WIFI_EVENT_AP_STACONNECTED)
                {
                    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                    ESP_LOGI(kLogPrefix, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
                }
                else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
                {
                    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                    ESP_LOGI(kLogPrefix, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
                }
            }

            void Init(void)
            {
                //Initialize NVS
                esp_err_t ret = nvs_flash_init();
                if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
                {
                    ESP_ERROR_CHECK(nvs_flash_erase());
                    ret = nvs_flash_init();
                }
                ESP_ERROR_CHECK(ret);

                ESP_LOGI(kLogPrefix, "ESP_WIFI_MODE_AP");

                ESP_ERROR_CHECK(esp_netif_init());
                ESP_ERROR_CHECK(esp_event_loop_create_default());
                esp_netif_create_default_wifi_ap();

                wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
                ESP_ERROR_CHECK(esp_wifi_init(&cfg));

                ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
                wifi_config_t wifi_config = {EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, strlen(EXAMPLE_ESP_WIFI_SSID), EXAMPLE_ESP_WIFI_CHANNEL, WIFI_AUTH_WPA_WPA2_PSK, 0, EXAMPLE_MAX_STA_CONN, 100, WIFI_CIPHER_TYPE_TKIP_CCMP, false};
                if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
                {
                    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
                }

                ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
                ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
                ESP_ERROR_CHECK(esp_wifi_start());

                ESP_LOGI(kLogPrefix, "Init finished. SSID:%s password:%s channel:%d", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

                wifi::ready = true;
            }
        } // access_point

    } // wifi
} // esp_sio_dev