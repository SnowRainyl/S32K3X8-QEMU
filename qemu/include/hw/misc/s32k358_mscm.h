/*
 * NXP S32K358 MSCM model (minimal implementation).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef S32K358_MSCM_H
#define S32K358_MSCM_H

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "exec/memory.h"
#include "qom/object.h"

#define TYPE_S32K358_MSCM "s32k358-mscm"

#define S32K358_MSCM_MMIO_SIZE 0x4000U
#define S32K358_MSCM_CPXNUM_OFF 0x4U
#define S32K358_MSCM_CPXNUM_CPN_MASK 0x7U

typedef struct S32K358MSCMState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    uint32_t core_id;
    uint8_t regs[S32K358_MSCM_MMIO_SIZE];
} S32K358MSCMState;

OBJECT_DECLARE_SIMPLE_TYPE(S32K358MSCMState, S32K358_MSCM)

#endif /* S32K358_MSCM_H */
