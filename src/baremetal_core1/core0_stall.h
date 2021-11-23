/*
 * Copyright (c) 2021, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

// Notice: This file modified for use in this project, openmc

#ifndef _CORE0_STALL_H_
#define _CORE0_STALL_H_

#ifdef __cplusplus
extern "C"
{
#endif

    void core0_stall_disable(void);
    void core0_stall_enable(void);
    void core0_stall_end(void);
    void core0_stall_init(void);
    void core0_stall_start(void);

#ifdef __cplusplus
}
#endif

#endif /* _CORE0_STALL_H_ */
