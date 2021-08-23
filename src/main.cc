#include "esp_file_helper.h"
#include "esp_file_server.h"
#include "esp_sdcard.h"
#include "esp_logging.h"
#include "esp_wifi_ap.h"
#include "esp_wifi_client.h"
#include "oled.h"
#include "pins.h"
#include "sio.h"
#include "sio_memory_card.h"
#include "spi.h"

#include "core0_stall.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <sdkconfig.h>
#include <driver/uart.h>
#include <esp_intr_alloc.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define SD_TASK_CORE 0
#define WIFI_TASK_CORE 0
#define SIO_TASK_CORE 1

#define MAX_TIMEOUT_SECONDS 10

#define PORT 6699
#define KEEPALIVE_IDLE 5
#define KEEPALIVE_INTERVAL 5
#define KEEPALIVE_COUNT 3

// rama
// sickle <3
// Nicolas Noble
// danhans42

extern "C"
{
    void app_main(void);
    void start_app_cpu(void);
}

extern DRAM_ATTR uint8_t output[4];

namespace esp_sio_dev
{
    void main(void);
    void SetupInterrupts();

    static void do_retransmit(const int sock)
    {
        int len;
        char rx_buffer[128];

        char test[15] = "Hello world\r\n\0";

        do
        {
            len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0)
            {
                ESP_LOGE(kLogPrefix, "Error occurred during receiving: errno %d", errno);
            }
            else if (len == 0)
            {
                ESP_LOGW(kLogPrefix, "Connection closed");
            }
            else
            {
                rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
                ESP_LOGI(kLogPrefix, "Received %d bytes: %s", len, rx_buffer);

                // send() can return less bytes than supplied length.
                // Walk-around for robust implementation.
                //int to_write = len;
                int to_write = 15;
                while (to_write > 0)
                {
                    int written = send(sock, test + (15 - to_write), to_write, 0);
                    if (written < 0)
                    {
                        ESP_LOGE(kLogPrefix, "Error occurred during sending: errno %d", errno);
                    }
                    to_write -= written;
                }
            }
        } while (len > 0);
    }

    static void tcp_server_task(void *pvParameters)
    {
        char addr_str[128];
        int addr_family = (int)pvParameters;
        int ip_protocol = 0;
        int keepAlive = 1;
        int keepIdle = KEEPALIVE_IDLE;
        int keepInterval = KEEPALIVE_INTERVAL;
        int keepCount = KEEPALIVE_COUNT;
        struct sockaddr_storage dest_addr;

        if (addr_family == AF_INET)
        {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(PORT);
            ip_protocol = IPPROTO_IP;
        }

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0)
        {
            ESP_LOGE(kLogPrefix, "Unable to create socket: errno %d", errno);
            vTaskDelete(NULL);
            return;
        }
        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        ESP_LOGI(kLogPrefix, "Socket created");

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0)
        {
            ESP_LOGE(kLogPrefix, "Socket unable to bind: errno %d", errno);
            ESP_LOGE(kLogPrefix, "IPPROTO: %d", addr_family);
            goto CLEAN_UP;
        }
        ESP_LOGI(kLogPrefix, "Socket bound, port %d", PORT);

        err = listen(listen_sock, 1);
        if (err != 0)
        {
            ESP_LOGE(kLogPrefix, "Error occurred during listen: errno %d", errno);
            goto CLEAN_UP;
        }

        while (1)
        {

            ESP_LOGI(kLogPrefix, "Socket listening");

            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);
            int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock < 0)
            {
                ESP_LOGE(kLogPrefix, "Unable to accept connection: errno %d", errno);
                break;
            }

            // Set tcp keepalive option
            setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
            // Convert ip address to string
            if (source_addr.ss_family == PF_INET)
            {
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            }

            ESP_LOGI(kLogPrefix, "Socket accepted ip address: %s", addr_str);

            do_retransmit(sock);

            shutdown(sock, 0);
            close(sock);
        }

    CLEAN_UP:
        close(listen_sock);
        vTaskDelete(NULL);
    }

    void SetupInterrupts()
    {
        spi::InstallInterrupt();
    }

    void main(void)
    {
        sio::memory_card_enabled = true;
        sio::pad_enabled = false;
        sio::net_yaroze_enabled = false;

        // Turn off LED
        gpio_set_direction(kPin_LED, GPIO_MODE_OUTPUT);
        gpio_set_level(kPin_LED, 0);

        oled::init_oled();

        // Create a task to mount the SD Card
        xTaskCreatePinnedToCore(Task_MountSDCard, "sd_card_task_core_0", 1024 * 3, NULL, 0, NULL, SD_TASK_CORE);
        
        // 
        core0_stall_init();

        sio::Init();       // Init the SIO state machine to a default state.
        spi::InitPins();   // Setup the pins for bitbanged SPI
        spi::Enable();     // Enable SPI
        SetupInterrupts(); // Create a task to install our interrupt handler on Core 1, ESP32 likes Core 0 for WiFi
        
        //
        start_app_cpu();

        xTaskCreatePinnedToCore(SD_Write_Task, "sd_write_task_core_0", 1024 * 3, NULL, 0, NULL, WIFI_TASK_CORE);

        //xTaskCreatePinnedToCore(wifi_ap::Task_StartWifiAP, "wifi_ap_task_core_0", 1024 * 3, NULL, 0, NULL, WIFI_TASK_CORE);
        xTaskCreatePinnedToCore(wifi_client::Task_StartWifiClient, "wifi_client_task_core_0", 1024 * 3, NULL, 0, NULL, WIFI_TASK_CORE);

        while (file_server::net_interface_ready == false && file_server::sd_filesystem_ready == false)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        xTaskCreatePinnedToCore(file_server::Task_StartFileServer, "file_server_task_core_0", 1024 * 3, NULL, 0, NULL, WIFI_TASK_CORE);

        xTaskCreate(tcp_server_task, "tcp_server", 4096, (void *)AF_INET, 5, NULL);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
        printf("Free Heap = %i\n", esp_get_free_heap_size());
    }
}

// Things.
void app_main(void)
{
    esp_sio_dev::main();
}