/*
 * NXP S32K358 DMAMUX model (minimal CHCFG implementation).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef S32K358_DMAMUX_H
#define S32K358_DMAMUX_H

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "exec/memory.h"
#include "qom/object.h"

#define TYPE_S32K358_DMAMUX "s32k358-dmamux"

#define S32K358_DMAMUX_NUM_CHANNELS 16U
#define S32K358_DMAMUX_MMIO_SIZE    0x1000U

typedef struct S32K358DMAMUXState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    uint8_t chcfg[S32K358_DMAMUX_NUM_CHANNELS];
} S32K358DMAMUXState;

OBJECT_DECLARE_SIMPLE_TYPE(S32K358DMAMUXState, S32K358_DMAMUX)

#endif /* S32K358_DMAMUX_H */
