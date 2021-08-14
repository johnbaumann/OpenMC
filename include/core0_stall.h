/*
 * Copyright (c) 2021, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

// Notice: This file modified for use in this project, esp-sio-dev

#ifndef _CORE0_STALL_H_
#define _CORE0_STALL_H_

#ifdef __cplusplus
extern "C"
{
#endif

    void IRAM_ATTR core0_stall_start(void);
    void IRAM_ATTR core0_stall_end(void);
    void IRAM_ATTR core0_stall_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _CORE0_STALL_H_ */
