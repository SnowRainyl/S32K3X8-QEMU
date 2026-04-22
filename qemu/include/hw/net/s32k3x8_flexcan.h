#ifndef HW_NET_S32K3X8_FLEXCAN_H
#define HW_NET_S32K3X8_FLEXCAN_H

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "net/can_emu.h"

#define TYPE_S32K3X8_FLEXCAN "s32k3x8-flexcan"
OBJECT_DECLARE_SIMPLE_TYPE(S32K3X8FlexCANState, S32K3X8_FLEXCAN)

#define S32K3X8_FLEXCAN_MMIO_SIZE 0x4000U
#define S32K3X8_FLEXCAN_LEGACY_FIFO_CAPACITY 6U
#define S32K3X8_FLEXCAN_ENHANCED_FIFO_CAPACITY 63U

typedef struct S32K3X8FlexCANRxEntry {
    qemu_can_frame frame;
    uint16_t idhit;
} S32K3X8FlexCANRxEntry;

struct S32K3X8FlexCANState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    qemu_irq irq;

    CanBusClientState bus_client;
    CanBusState *canbus;

    uint32_t instance_id;
    uint16_t timestamp;
    uint32_t regs[S32K3X8_FLEXCAN_MMIO_SIZE / sizeof(uint32_t)];

    S32K3X8FlexCANRxEntry legacy_fifo[S32K3X8_FLEXCAN_LEGACY_FIFO_CAPACITY];
    uint8_t legacy_fifo_head;
    uint8_t legacy_fifo_tail;
    uint8_t legacy_fifo_count;

    S32K3X8FlexCANRxEntry enhanced_fifo[S32K3X8_FLEXCAN_ENHANCED_FIFO_CAPACITY];
    uint8_t enhanced_fifo_head;
    uint8_t enhanced_fifo_tail;
    uint8_t enhanced_fifo_count;
};

#endif /* HW_NET_S32K3X8_FLEXCAN_H */
