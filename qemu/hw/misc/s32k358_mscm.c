/*
 * NXP S32K358 MSCM model (minimal implementation).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/misc/s32k358_mscm.h"
#include "hw/qdev-properties.h"

static uint64_t s32k358_mscm_read(void *opaque, hwaddr addr, unsigned size)
{
    S32K358MSCMState *s = S32K358_MSCM(opaque);
    uint64_t value = 0;

    if (addr + size > S32K358_MSCM_MMIO_SIZE) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "s32k358-mscm: invalid read addr=0x%" HWADDR_PRIx
                      " size=%u\n", addr, size);
        return 0;
    }

    for (unsigned i = 0; i < size; i++) {
        value |= ((uint64_t)s->regs[addr + i] << (8U * i));
    }

    return value;
}

static void s32k358_mscm_write(void *opaque, hwaddr addr, uint64_t value,
                               unsigned size)
{
    S32K358MSCMState *s = S32K358_MSCM(opaque);

    if (addr + size > S32K358_MSCM_MMIO_SIZE) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "s32k358-mscm: invalid write addr=0x%" HWADDR_PRIx
                      " size=%u val=0x%" PRIx64 "\n",
                      addr, size, value);
        return;
    }

    for (unsigned i = 0; i < size; i++) {
        s->regs[addr + i] = (uint8_t)(value >> (8U * i));
    }
}

static const MemoryRegionOps s32k358_mscm_ops = {
    .read = s32k358_mscm_read,
    .write = s32k358_mscm_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
        .unaligned = true,
    },
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
        .unaligned = true,
    },
};

static void s32k358_mscm_reset(DeviceState *dev)
{
    S32K358MSCMState *s = S32K358_MSCM(dev);
    uint32_t cpxnum = s->core_id & S32K358_MSCM_CPXNUM_CPN_MASK;

    memset(s->regs, 0, sizeof(s->regs));
    s->regs[S32K358_MSCM_CPXNUM_OFF + 0] = (uint8_t)(cpxnum & 0xFFU);
    s->regs[S32K358_MSCM_CPXNUM_OFF + 1] = (uint8_t)((cpxnum >> 8) & 0xFFU);
    s->regs[S32K358_MSCM_CPXNUM_OFF + 2] = (uint8_t)((cpxnum >> 16) & 0xFFU);
    s->regs[S32K358_MSCM_CPXNUM_OFF + 3] = (uint8_t)((cpxnum >> 24) & 0xFFU);
}

static void s32k358_mscm_init(Object *obj)
{
    S32K358MSCMState *s = S32K358_MSCM(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &s32k358_mscm_ops, s,
                          TYPE_S32K358_MSCM, S32K358_MSCM_MMIO_SIZE);
    sysbus_init_mmio(sbd, &s->mmio);
}

static Property s32k358_mscm_properties[] = {
    DEFINE_PROP_UINT32("core-id", S32K358MSCMState, core_id, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void s32k358_mscm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "S32K358 MSCM";
    device_class_set_legacy_reset(dc, s32k358_mscm_reset);
    device_class_set_props(dc, s32k358_mscm_properties);
}

static const TypeInfo s32k358_mscm_info = {
    .name = TYPE_S32K358_MSCM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K358MSCMState),
    .instance_init = s32k358_mscm_init,
    .class_init = s32k358_mscm_class_init,
};

static void s32k358_mscm_register_types(void)
{
    type_register_static(&s32k358_mscm_info);
}

type_init(s32k358_mscm_register_types)
