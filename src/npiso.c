/*
 * Copyright (c) 2019-2020, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <xtensa/hal.h>
#include <esp_intr_alloc.h>
#include <esp_task_wdt.h>
#include "driver/gpio.h"
#include <esp32/dport_access.h>
#include "npiso.h"

#define NPISO_PORT_MAX 2
#define NPISO_LATCH_PIN 32
#define NPISO_LATCH_MASK (1U << 0) /* First of 2nd bank of GPIO */
#define NPISO_TIMEOUT 4096

#define P1_CLK_PIN 5
#define P1_SEL_PIN 23
#define P1_D0_PIN 19
#define P1_D1_PIN 21
#define P2_CLK_PIN 18
#define P2_SEL_PIN 26
#define P2_D0_PIN 22
#define P2_D1_PIN 25

#define P1_CLK_MASK (1U << P1_CLK_PIN)
#define P1_SEL_MASK (1U << P1_SEL_PIN)
#define P1_D0_MASK (1U << P1_D0_PIN)
#define P1_D1_MASK (1U << P1_D1_PIN)
#define P2_CLK_MASK (1U << P2_CLK_PIN)
#define P2_SEL_MASK (1U << P2_SEL_PIN)
#define P2_D0_MASK (1U << P2_D0_PIN)
#define P2_D1_MASK (1U << P2_D1_PIN)

enum {
    NPISO_CLK = 0,
    NPISO_SEL,
    NPISO_D0,
    NPISO_D1,
    NPISO_PIN_MAX,
};

enum {
    DEV_NONE = 0,
    DEV_FC_NES_PAD,
    DEV_FC_NES_MULTITAP,
    DEV_FC_MULTITAP_ALT,
    DEV_FC_TRACKBALL,
    DEV_FC_KB,
    DEV_SFC_SNES_PAD,
    DEV_SFC_SNES_MULTITAP,
    DEV_SFC_SNES_MOUSE,
    DEV_SNES_XBAND_KB,
};

extern volatile void * _xt_intexc_hooks[];

static DRAM_ATTR const uint8_t gpio_pins[NPISO_PORT_MAX][NPISO_PIN_MAX] = {
    {P1_CLK_PIN, P1_SEL_PIN, P1_D0_PIN, P1_D1_PIN},
    {P2_CLK_PIN, P2_SEL_PIN, P2_D0_PIN, P2_D1_PIN},
};

static DRAM_ATTR const uint32_t gpio_mask[NPISO_PORT_MAX][NPISO_PIN_MAX] = {
    {P1_CLK_MASK, P1_SEL_MASK, P1_D0_MASK, P1_D1_MASK},
    {P2_CLK_MASK, P2_SEL_MASK, P2_D0_MASK, P2_D1_MASK},
};

DRAM_ATTR uint8_t output[4] = {0xFF, 0xFF, 0xFF, 0xFF};
static DRAM_ATTR uint8_t dev_type[NPISO_PORT_MAX] = {0};
static DRAM_ATTR uint32_t cnt[NPISO_PORT_MAX] = {0};
static DRAM_ATTR uint8_t mask[NPISO_PORT_MAX] = {0x80, 0x80};
static DRAM_ATTR uint8_t fs_id[NPISO_PORT_MAX] = {0xEF, 0xDF};
static DRAM_ATTR uint32_t idx[NPISO_PORT_MAX];
static DRAM_ATTR uint32_t toggle = 0;

static void IRAM_ATTR set_data(uint8_t port, uint8_t data_id, uint8_t value) {
    uint8_t pin = gpio_pins[port][NPISO_D0 + data_id];

    if (value) {
        GPIO.out_w1ts = BIT(pin);
    }
    else {
        GPIO.out_w1tc = BIT(pin);
    }
}

uint32_t IRAM_ATTR npiso_isr(uint32_t cause) {
    uint32_t low_io = GPIO.acpu_int;
    uint32_t high_io = GPIO.acpu_int1.intr;
    uint32_t cur_in0, cur_in1;

    cur_in0 = GPIO.in;
    cur_in1 = GPIO.in1.val;

    /* reset bit counter, set first bit */
    if (high_io & NPISO_LATCH_MASK) {
        if (cur_in1 & NPISO_LATCH_MASK) {
            for (uint32_t i = 0; i < NPISO_PORT_MAX; i++) {
                switch (dev_type[i]) {
                    case DEV_FC_MULTITAP_ALT:
                        set_data(i, 1, output[0] & 0x80);
                    __attribute__ ((fallthrough));
                    case DEV_FC_NES_PAD:
                    case DEV_FC_NES_MULTITAP:
                    case DEV_SFC_SNES_PAD:
                        set_data(i, 0, output[0] & 0x80);
                        cnt[i] = 1;
                        mask[i] = 0x40;
                        break;
                }
            }
        }
    }

    /* data idx */
    idx[0] = cnt[0] >> 3;
    idx[1] = cnt[1] >> 3;

    /* Update data lines on rising clock edge */
    for (uint32_t i = 0; i < NPISO_PORT_MAX; i++) {
        if (low_io & gpio_mask[i][NPISO_CLK]) {
            if (cur_in0 & gpio_mask[i][NPISO_CLK]) {
                switch (dev_type[i]) {
                    case DEV_FC_MULTITAP_ALT:
                        if (idx[i]) {
                            set_data(i, 1, 0);
                        }
                        else {
                            set_data(i, 1, output[0] & mask[i]);
                        }
                    __attribute__ ((fallthrough));
                    case DEV_FC_NES_PAD:
                        if (idx[i]) {
                            set_data(i, 0, 0);
                        }
                        else {
                            set_data(i, 0, output[0] & mask[i]);
                        }
                        break;
                    case DEV_FC_NES_MULTITAP:
                        switch (idx[i]) {
                            case 0:
                                set_data(i, 0, output[0] & mask[i]);
                                break;
                            case 1:
                                set_data(i, 0, output[0] & mask[i]);
                                break;
                            case 2:
                                set_data(i, 0, fs_id[i] & mask[i]);
                                break;
                            default:
                                set_data(i, 0, 0);
                                break;
                        }
                        break;
                    case DEV_SFC_SNES_PAD:
                        if (idx[i] > 1) {
                            set_data(i, 0, 0);
                        }
                        else {
                            set_data(i, 0, output[idx[i]] & mask[i]);
                        }
                        break;
                }
                cnt[i]++;
                mask[i] >>= 1;
                if (!mask[i]) {
                    mask[i] = 0x80;
                }
                set_data(0, 1, toggle ^= 0x01);
            }
            else {
                set_data(0, 1, toggle ^= 0x01);
            }
        }
    }

    if (high_io) GPIO.status1_w1tc.intr_st = high_io;
    if (low_io) GPIO.status_w1tc = low_io;
    return 0;
}

void IRAM_ATTR npiso_task(void *arg) {
    uint32_t cur_in0, prev_in0, change0 = 0, cur_in1, prev_in1, change1 = 0;
    uint8_t toggle = 0;

    cur_in0 = GPIO.in;
    cur_in1 = GPIO.in1.val;
    while (1) {
        prev_in0 = cur_in0;
        cur_in0 = GPIO.in;
        change0 = cur_in0 ^ prev_in0;
        prev_in1 = cur_in1;
        cur_in1 = GPIO.in1.val;
        change1 = cur_in1 ^ prev_in1;

        /* reset bit counter, set first bit */
        if (change1 & NPISO_LATCH_MASK) {
            if (cur_in1 & NPISO_LATCH_MASK) {
                for (uint32_t i = 0; i < NPISO_PORT_MAX; i++) {
                    switch (dev_type[i]) {
                        case DEV_FC_MULTITAP_ALT:
                            set_data(i, 1, output[0] & 0x80);
                        __attribute__ ((fallthrough));
                        case DEV_FC_NES_PAD:
                        case DEV_FC_NES_MULTITAP:
                        case DEV_SFC_SNES_PAD:
                            set_data(i, 0, output[0] & 0x80);
                            cnt[i] = 1;
                            mask[i] = 0x40;
                            break;
                    }
                }
            }
        }

        /* data idx */
        idx[0] = cnt[0] >> 3;
        idx[1] = cnt[1] >> 3;

        /* Update data lines on rising clock edge */
        for (uint32_t i = 0; i < NPISO_PORT_MAX; i++) {
            if (change0 & gpio_mask[i][NPISO_CLK]) {
                if (cur_in0 & gpio_mask[i][NPISO_CLK]) {
                    switch (dev_type[i]) {
                        case DEV_FC_MULTITAP_ALT:
                            if (idx[i]) {
                                set_data(i, 1, 0);
                            }
                            else {
                                set_data(i, 1, output[0] & mask[i]);
                            }
                        __attribute__ ((fallthrough));
                        case DEV_FC_NES_PAD:
                            if (idx[i]) {
                                set_data(i, 0, 0);
                            }
                            else {
                                set_data(i, 0, output[0] & mask[i]);
                            }
                            break;
                        case DEV_FC_NES_MULTITAP:
                            switch (idx[i]) {
                                case 0:
                                    set_data(i, 0, output[0] & mask[i]);
                                    break;
                                case 1:
                                    set_data(i, 0, output[0] & mask[i]);
                                    break;
                                case 2:
                                    set_data(i, 0, fs_id[i] & mask[i]);
                                    break;
                                default:
                                    set_data(i, 0, 0);
                                    break;
                            }
                            break;
                        case DEV_SFC_SNES_PAD:
                            if (idx[i] > 1) {
                                set_data(i, 0, 0);
                            }
                            else {
                                set_data(i, 0, output[idx[i]] & mask[i]);
                            }
                            break;
                    }
                    cnt[i]++;
                    mask[i] >>= 1;
                    if (!mask[i]) {
                        mask[i] = 0x80;
                    }
                    set_data(0, 1, toggle ^= 0x01);
                }
                else {
                    set_data(0, 1, toggle ^= 0x01);
                }
            }
        }
    }
}

void npiso_init(void)
{
    gpio_config_t io_conf = {0};

    dev_type[0] = DEV_SFC_SNES_PAD;
    dev_type[1] = DEV_SFC_SNES_PAD;

    /* Latch */
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = 1ULL << NPISO_LATCH_PIN;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    GPIO.pin[NPISO_LATCH_PIN].int_ena = BIT(0);

    /* Clocks */
    for (uint32_t i = 0; i < NPISO_PORT_MAX; i++) {
        io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
        io_conf.pin_bit_mask = 1ULL << gpio_pins[i][NPISO_CLK];
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io_conf);
        GPIO.pin[gpio_pins[i][NPISO_CLK]].int_ena = BIT(0);
    }

    /* Selects */
    for (uint32_t i = 0; i < NPISO_PORT_MAX; i++) {
        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        io_conf.pin_bit_mask = 1ULL << gpio_pins[i][NPISO_SEL];
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io_conf);
    }

    /* D0, D1 */
    for (uint32_t i = 0; i < NPISO_PORT_MAX; i++) {
        for (uint32_t j = NPISO_D0; j <= NPISO_D1; j++) {
            io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
            io_conf.pin_bit_mask = 1ULL << gpio_pins[i][j];
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            gpio_config(&io_conf);
            set_data(i, j &  0x1, 1);
        }
    }

    _xt_intexc_hooks[2] = &npiso_isr;
    intr_matrix_set(1, ETS_GPIO_INTR_SOURCE, 19);
}
