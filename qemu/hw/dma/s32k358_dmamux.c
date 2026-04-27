/*
 * NXP S32K358 DMAMUX model (minimal CHCFG implementation).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/dma/s32k358_dmamux.h"

static uint64_t s32k358_dmamux_read(void *opaque, hwaddr addr, unsigned size)
{
    S32K358DMAMUXState *s = S32K358_DMAMUX(opaque);
    uint64_t value = 0;

    if (addr + size > S32K358_DMAMUX_MMIO_SIZE) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "s32k358-dmamux: invalid read addr=0x%" HWADDR_PRIx
                      " size=%u\n", addr, size);
        return 0;
    }

    for (unsigned i = 0; i < size; i++) {
        hwaddr off = addr + i;
        uint8_t byte = 0;

        if (off < S32K358_DMAMUX_NUM_CHANNELS) {
            byte = s->chcfg[off];
        }
        value |= ((uint64_t)byte << (8U * i));
    }

    return value;
}

static void s32k358_dmamux_write(void *opaque, hwaddr addr, uint64_t value,
                                 unsigned size)
{
    S32K358DMAMUXState *s = S32K358_DMAMUX(opaque);

    if (addr + size > S32K358_DMAMUX_MMIO_SIZE) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "s32k358-dmamux: invalid write addr=0x%" HWADDR_PRIx
                      " size=%u val=0x%" PRIx64 "\n",
                      addr, size, value);
        return;
    }

    for (unsigned i = 0; i < size; i++) {
        hwaddr off = addr + i;
        uint8_t byte = (uint8_t)(value >> (8U * i));

        if (off < S32K358_DMAMUX_NUM_CHANNELS) {
            s->chcfg[off] = byte;
        }
    }
}

static const MemoryRegionOps s32k358_dmamux_ops = {
    .read = s32k358_dmamux_read,
    .write = s32k358_dmamux_write,
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

static void s32k358_dmamux_reset(DeviceState *dev)
{
    S32K358DMAMUXState *s = S32K358_DMAMUX(dev);

    memset(s->chcfg, 0, sizeof(s->chcfg));
}

static void s32k358_dmamux_instance_init(Object *obj)
{
    S32K358DMAMUXState *s = S32K358_DMAMUX(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &s32k358_dmamux_ops, s,
                          TYPE_S32K358_DMAMUX, S32K358_DMAMUX_MMIO_SIZE);
    sysbus_init_mmio(sbd, &s->mmio);
}

static void s32k358_dmamux_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "S32K358 DMAMUX";
    device_class_set_legacy_reset(dc, s32k358_dmamux_reset);
}

static const TypeInfo s32k358_dmamux_info = {
    .name = TYPE_S32K358_DMAMUX,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K358DMAMUXState),
    .instance_init = s32k358_dmamux_instance_init,
    .class_init = s32k358_dmamux_class_init,
};

static void s32k358_dmamux_register_types(void)
{
    type_register_static(&s32k358_dmamux_info);
}

type_init(s32k358_dmamux_register_types)
