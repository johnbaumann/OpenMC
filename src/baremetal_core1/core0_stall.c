// Copyright 2010-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdint.h>
#include <string.h>
#include "esp_intr_alloc.h"
#include "esp32/rom/ets_sys.h"
#include "soc/cpu.h"
#include "soc/dport_reg.h"
#include "xtensa/core-macros.h"
#include "sdkconfig.h"

/* these global variables are accessed from interrupt vector, hence not declared as static */
uint32_t volatile DRAM_ATTR core0_stall_start_var[1]; //dport register could be accessed
uint32_t volatile DRAM_ATTR core0_stall_end_var[1];   //dport register is accessed over
bool volatile DRAM_ATTR core0_stall_enabled = true;

void IRAM_ATTR core0_stall_disable(void)
{
    core0_stall_enabled = false;
}

// Experimental, allows blocking core0 stalls.
// This breaks memory card traffic, so not sure this has any use.
void IRAM_ATTR core0_stall_enable(void)
{
    core0_stall_enabled = true;
}

void IRAM_ATTR core0_stall_start(void)
{
    if (core0_stall_enabled)
    {
        core0_stall_start_var[0] = 0;
        core0_stall_end_var[0] = 0;
    }

    _DPORT_REG_WRITE(DPORT_CPU_INTR_FROM_CPU_2_REG, DPORT_CPU_INTR_FROM_CPU_2); //interrupt on cpu0
}

void IRAM_ATTR core0_stall_end(void)
{
    while (!core0_stall_start_var[0])
        ;
    core0_stall_end_var[0] = 1;
}

void IRAM_ATTR core0_stall_init(void)
{
    core0_stall_start_var[0] = 0;
    core0_stall_end_var[0] = 0;
    core0_stall_enabled = true;

    ESP_INTR_DISABLE(31);
    intr_matrix_set(0, ETS_FROM_CPU_INTR2_SOURCE, 31);
    ESP_INTR_ENABLE(31);
}
