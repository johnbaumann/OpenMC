#ifndef _TIMER_H
#define _TIMER_H

#include "system/timer.h"

#include "logging.h"
#include "playstation/sio.h"
#include "playstation/sio_memory_card.h"

#include <esp_attr.h>
#include <esp_timer.h>
#include <esp_log.h>

namespace esp_sio_dev
{
    namespace system
    {
        namespace timer
        {
            uint64_t DRAM_ATTR timestamp;

            static void timer_callback(void *arg)
            {
                timestamp++;
            }

            void Init(void)
            {
                const esp_timer_create_args_t periodic_timer_args = {
                    .callback = &timer_callback,
                    .arg = NULL,
                    .dispatch_method = ESP_TIMER_TASK,
                    /* name is optional, but may help identify the timer when debugging */
                    .name = "periodic",
                    .skip_unhandled_events = false};

                esp_timer_handle_t periodic_timer;
                ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

                // Tick timer every millisecond
                ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));
            }
        } // timer
    } // system
} // esp_sio_dev

#endif // _TIMER_H
