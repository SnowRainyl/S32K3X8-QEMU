/*
 * S32K358 UART
 *
 * Copyright (c) 2025 Emiliano Salvetto <em1l14n0s@gmail.com>
 * SPDX-License-Identifier: CC-BY-NC-4.0
 *
 * This work is licensed under the terms of the Creative Commons Attribution Non Commercial 4.0 International
 * See the COPYING file in the top-level directory.
 */
#include "hw/dma/s32k358_dma.h"
#include "exec/memattrs.h"
#define DEBUG_DMA_TCD
#define S32K358_DMA_DEBUG 0
/* Those callbacks are made to set the registers of the eDMA engine */

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (S32K358_DMA_DEBUG >= lvl) { \
        qemu_log("[QEMU/S32K358-DMA] %s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void s32k358_dma_write (void *opaque, hwaddr addr, uint64_t val, unsigned size){
    S32K358DMAState* s = S32K358_DMA(opaque);
    switch(addr){
        case CSR_OFF:
            s->reg_csr=val;
            break;
        case ES_OFF:
            s->reg_es=val;
            break;
        case INT_OFF:
            s->reg_int=val;
            break;
        case HRS_OFF:
            s->reg_hrs=val;
            break;
        case EDMA_REGPROT_GCR_OFF:
            s->regprot_gcr = (uint32_t)val;
            break;
        default:
            long chNum = (addr - CH_GRPRI_OFF) / 4;
            if( chNum >= 0 && chNum < S32K358_NUM_DMA_CH ){
                s->reg_ch_gpri[chNum]=val;
            }
            break;
    }
}

static uint64_t s32k358_dma_read (void *opaque, hwaddr addr, unsigned size){
    S32K358DMAState* s = S32K358_DMA(opaque);
    switch(addr){
        case CSR_OFF:
            return s->reg_csr;
        case ES_OFF:
            return s->reg_es;
        case INT_OFF:
            return s->reg_int;
        case HRS_OFF:
            return s->reg_hrs;
        case EDMA_REGPROT_GCR_OFF:
            return s->regprot_gcr;
        default:
            long chNum = (addr - CH_GRPRI_OFF) / 4;
            if( chNum >= 0 && chNum < S32K358_NUM_DMA_CH ){
                return s->reg_ch_gpri[chNum];
            }
            break;
    }
    return 0;
}

static const MemoryRegionOps s32k358_dma_ops = {
    .read = s32k358_dma_read,
    .write = s32k358_dma_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const char* dma_handle_rw_error(MemTxResult result){
    switch(result){
        case MEMTX_OK:
            return "DMA R/W OK";
        case MEMTX_ERROR:
            return "Device returned error";
        case MEMTX_DECODE_ERROR:
            return "Nothing at that address error";
        case MEMTX_ACCESS_ERROR:
            return "Access denied error";
        default:
            break;
    }
    return "Undefinied error";
}
void s32k358_dma_preemption(S32K358DMAState* s, S32K358DMAChannel* ch){
    /* If the eDMA is idle there is no preemption and the transfer can start immediately */
    DB_PRINT("DMA CSR:%d\n",s->reg_csr);
    if( ( DMA_IS_ACTIVE(s->reg_csr) ) == 0 ){
        s32k358_dma_transfer(s, ch);
    }
    else{
        DB_PRINT("DMA is already active!\n");
    }
    return;
}
void s32k358_dma_transfer(S32K358DMAState* s, S32K358DMAChannel* ch){
    int16_t linkCh = -1;
    DB_PRINT("---[QEMU] TCD Debug:\n\nSADDR:%d\nDADDR:%d\nCITER:%d\nBITER:%d\nNBYTES:%d\nSOFF:%d\nDOFF:%d\nSLAST:%d\nDLAST:%d\n---\n",ch->tcd.SADDR, ch->tcd.DADDR, ch->tcd.CITER, ch->tcd.BITER, ch->tcd.NBYTES, ch->tcd.SOFF, ch->tcd.DOFF, ch->tcd.SLAST_SDA, ch->tcd.DLAST_SGA);
    if ( CH_TCD_GET_ELINK(ch->tcd.CITER) != CH_TCD_GET_ELINK(ch->tcd.BITER) ){
        DB_PRINT("DMA CITER/BITER Configuration error, Aborting...\n");
        return;
    }
    ch->CSR = CH_CSR_SET_DONE(ch->CSR, 0);
    /* Set TCDn_CSR[ACTIVE] to 0 */
    ch->tcd.CSR -= 1;
    ch->CSR = CH_CSR_SET_ACTIVE(ch->CSR, 1);
    /* If the ELINK flag is ENABLED */
    if( CH_TCD_GET_ELINK(ch->tcd.CITER) ){
       /* 9-13 LINKCH */
        linkCh = CH_TCD_GET_LINKCH(ch->tcd.CITER);
        ch->tcd.CITER = CH_TCD_GET_CITER(ch->tcd.CITER);
    }
    else
        ch->tcd.CITER = CH_TCD_GET_CITER(ch->tcd.CITER);
    MemTxResult res;
    char* buf = g_new0(char, ch->tcd.DOFF);
    /* Major loop */
    for(; ch->tcd.CITER > 0; ch->tcd.CITER--){
        /* Minor loop */
        for(uint32_t j=0; j < ch->tcd.NBYTES; j+=ch->tcd.DOFF, ch->tcd.DADDR+=ch->tcd.DOFF ){
            for( uint16_t k=0; k < ch ->tcd.DOFF; k+=ch->tcd.SOFF, ch->tcd.SADDR+=ch->tcd.SOFF ){
                res = address_space_read(&s->system_as, ch->tcd.SADDR,MEMTXATTRS_UNSPECIFIED, &buf[k], ch->tcd.SOFF);
                DB_PRINT("Address space read:%s\n", dma_handle_rw_error(res));

            }

            res = address_space_write(&s->system_as, ch->tcd.DADDR,MEMTXATTRS_UNSPECIFIED, buf, ch->tcd.DOFF);
            DB_PRINT("Address space write:%s\n", dma_handle_rw_error(res));

        }

        if(ch->tcd.BITER > 1 && ch->tcd.CITER == (ch->tcd.BITER / 2) ){
            DB_PRINT("INTHALF Triggered\n");
        }
        /* Channel linking to another channel, if the major loop is exhausted this mechanism is suppressed in favor of MAJORLINK */
        if( linkCh >= 0 && (ch->tcd.CITER - 1 == 0) ){
            DB_PRINT("Interrupt request to another channel\n");
        }
    }
    /* Linking to another channel defined in MAJORLINKCH via an internal mechanism
     * started by setting up TCDn_CSR[START] to 1 of desired channel */

    if( CH_TCD_GET_MAJORELINK( ch->tcd.CSR ) ){

        uint8_t majorlinkCh = CH_TCD_GET_MAJORLINKCH(ch->tcd.CSR);

        DB_PRINT("MAJORELINK: Linking to DMA Channel:%d\n",majorlinkCh);
        S32K358DMAChannel *newCh = &s->channels[majorlinkCh];
        newCh->tcd.CSR |= 0x1;
        s32k358_dma_transfer(s,newCh);

    }

    ch->tcd.SADDR += ch->tcd.SLAST_SDA;
    ch->tcd.DADDR += ch->tcd.DLAST_SGA;
    ch->tcd.CITER = ch->tcd.BITER;
    ch->CSR = CH_CSR_SET_DONE(ch->CSR,1);
    ch->CSR = CH_CSR_SET_ACTIVE(ch->CSR,0);
    ch->INT = CH_INT_SET_INT(ch->INT,1);

    if(CH_TCD_GET_MAJORINT(ch->tcd.CSR)){
        DB_PRINT("Interrupt request to another channel\n");
    }
    DB_PRINT("s32k358_dma_transfer END\n");
    g_free(buf);
    return;
}
/* Those callbacks are made to set the channels registers (all mapped in TCD memory region) */
static void s32k358_tcd_write (void *opaque, hwaddr addr, uint64_t val, unsigned size){
    S32K358DMAState* s = S32K358_DMA(opaque);
    S32K358DMAChannel* ch = &s->channels[CH_GET_NUM(addr)];
    #ifdef DEBUG_DMA_TCD
    switch(CH_GET_REG(addr)){
        case CH_CSR_OFF:
        case CH_ES_OFF:
        case CH_INT_OFF:
        case CH_SBR_OFF:
        case CH_PRI_OFF:
        case CH_TCD_SADDR_OFF:
        case CH_TCD_NBYTES_MLOFF_OFF:
        case CH_TCD_DADDR_OFF:
            DB_PRINT("TCD Write Value: %d at %lx\n",(uint32_t) val,addr);
            break;
        case CH_TCD_SOFF_OFF:
        case CH_TCD_ATTR_OFF:
        case CH_TCD_DOFF_OFF:
        case CH_TCD_CITER_ELINK_OFF:
        case CH_TCD_CSR_OFF:
        case CH_TCD_BITER_ELINK:
            DB_PRINT("TCD Write Value: %d at %lx\n",(uint16_t) val,addr);
            break;
        case CH_TCD_SLAST_SDA_OFF:
        case CH_TCD_DLAST_SGA_OFF:
            DB_PRINT("TCD Write Value: %d at %lx\n",(int32_t) val,addr);
            break;
        default:
            DB_PRINT("Invalid tcd reg. kaboom.\n");
            break;
    }
    #endif
    switch(CH_GET_REG(addr)){
        case CH_CSR_OFF:
            ch->CSR=(uint32_t) val;
            break;
        case CH_ES_OFF:
            ch->ES=(uint32_t) val;
            break;
        case CH_INT_OFF:
            ch->INT=(uint32_t) val;
            break;
        case CH_SBR_OFF:
            ch->SBR=(uint32_t) val;
            break;
        case CH_PRI_OFF:
            ch->PRI=(uint32_t) val;
            break;
        case CH_TCD_SADDR_OFF:
            ch->tcd.SADDR=(uint32_t) val;
            break;
        case CH_TCD_SOFF_OFF:
            ch->tcd.SOFF=(uint16_t) val;
            break;
        case CH_TCD_ATTR_OFF:
            ch->tcd.ATTR=(uint16_t) val;
            break;
        case CH_TCD_NBYTES_MLOFF_OFF:
            ch->tcd.NBYTES=(uint32_t) val;
            break;
        case CH_TCD_SLAST_SDA_OFF:
            ch->tcd.SLAST_SDA=(int32_t) val;
            break;
        case CH_TCD_DADDR_OFF:
            ch->tcd.DADDR=(uint32_t) val;
            break;
        case CH_TCD_DOFF_OFF:
            ch->tcd.DOFF=(uint16_t) val;
            break;
        case CH_TCD_CITER_ELINK_OFF:
            ch->tcd.CITER = (uint16_t) val;
            break;
        case CH_TCD_DLAST_SGA_OFF:
            ch->tcd.DLAST_SGA=(int32_t) val;
            break;
        case CH_TCD_CSR_OFF:
            uint16_t csr_val = (uint16_t) val;
            ch->tcd.CSR= csr_val;
            // if(TCDn_CSR[START] = 1) then trigger a DMA software request
            if(csr_val & 1){
                DB_PRINT("Requested start for the channel:%ld\n",CH_GET_NUM(addr));
                s32k358_dma_preemption(s, ch);
            }
            break;
        case CH_TCD_BITER_ELINK:
            ch->tcd.BITER = (uint16_t) val;
            break;
        default:
            DB_PRINT("Invalid tcd reg. kaboom.\n");
            break;
    }
}
static uint64_t s32k358_tcd_read (void *opaque, hwaddr addr, unsigned size){
    S32K358DMAState* s = S32K358_DMA(opaque);
    S32K358DMAChannel* ch = &s->channels[CH_GET_NUM(addr)];
    DB_PRINT("TCD Read at channel:%lx reg:%lx\n",CH_GET_NUM(addr), CH_GET_REG(addr));
    //printf("Read at reg:%x\n",CH_GET_REG(addr));
    switch(CH_GET_REG(addr)){
        case CH_CSR_OFF:
            return ch->CSR;
        case CH_ES_OFF:
            return ch->ES;
        case CH_INT_OFF:
            return ch->INT;
        case CH_SBR_OFF:
            return ch->SBR;
        case CH_PRI_OFF:
            return ch->PRI;
        case CH_TCD_SADDR_OFF:
            return ch->tcd.SADDR;
        case CH_TCD_SOFF_OFF:
            return ch->tcd.SOFF;
        case CH_TCD_ATTR_OFF:
            return ch->tcd.ATTR;
        case CH_TCD_NBYTES_MLOFF_OFF:
            return ch->tcd.NBYTES;
        case CH_TCD_SLAST_SDA_OFF:
            return ch->tcd.SLAST_SDA;
        case CH_TCD_DADDR_OFF:
            return ch->tcd.DADDR;
        case CH_TCD_DOFF_OFF:
            return ch->tcd.DOFF;
        case CH_TCD_CITER_ELINK_OFF:
            return ch->tcd.CITER;
        case CH_TCD_DLAST_SGA_OFF:
            return ch->tcd.DLAST_SGA;
        case CH_TCD_CSR_OFF:
            return ch->tcd.CSR;
        case CH_TCD_BITER_ELINK:
            return ch->tcd.BITER;
        default:
            printf("invalid reg");
            break;
    }
    return 0;
    }
static const MemoryRegionOps s32k358_tcd_ops = {
.read = s32k358_tcd_read,
.write = s32k358_tcd_write,
.endianness = DEVICE_NATIVE_ENDIAN,
};


static void s32k358_dma_init(Object* obj){
    S32K358DMAState* dma = S32K358_DMA(obj);
    SysBusDevice *d = SYS_BUS_DEVICE(obj);
    memory_region_init_io(&dma->registers, obj, &s32k358_dma_ops, dma, "eDMA engine registers", EDMA_REGS_SIZE);
    sysbus_init_mmio(d, &dma->registers);

    memory_region_init_io(&dma->tcd_region[0], obj, &s32k358_tcd_ops, dma, "eDMA TCDs local memory (first part)", EDMA_TCD1_SIZE);
    sysbus_init_mmio(d, &dma->tcd_region[0]);

    memory_region_init_io(&dma->tcd_region[1], obj, &s32k358_tcd_ops, dma, "eDMA TCDs local memory (second part)", EDMA_TCD2_SIZE);
    sysbus_init_mmio(d, &dma->tcd_region[1]);

    for (int i = 0; i < S32K358_NUM_DMA_CH; i++) {
        sysbus_init_irq(d, &dma->irq[i]);
    }
    sysbus_init_irq(d, &dma->error_irq);
}

static Property s32k358_dma_properties[] = {
    DEFINE_PROP_LINK("memory", S32K358DMAState, system_memory,
                     TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void s32k358_dma_realize(DeviceState *dev, Error **errp){
    S32K358DMAState *s = S32K358_DMA(dev);
    if (!s->system_memory) {
        error_setg(errp, "s32k358-dma: memory property not set");
        return;
    }

    s->regprot_gcr = 0x0;

    // Create address space from memory region
    address_space_init(&s->system_as, s->system_memory, "eDMA-AddressSpace");

    // Setup registers...
    /* Reset values from documentation */
    for(int i=0; i < S32K358_NUM_DMA_CH; i++){
        s->channels[i].CSR = 0x0;
        s->channels[i].ES = 0x0;
        s->channels[i].INT = 0x0;
        s->channels[i].SBR = 0x00008002;
        s->channels[i].PRI = 0x0;
        s->channels[i].tcd.SADDR = 0x0;
        s->channels[i].tcd.SOFF = 0x0;
        s->channels[i].tcd.ATTR = 0x0;
        s->channels[i].tcd.NBYTES = 0x0;
        s->channels[i].tcd.CSR = 0x0;
        s->channels[i].tcd.SLAST_SDA = 0x0;
        s->channels[i].tcd.DADDR = 0x0;
        s->channels[i].tcd.DOFF = 0x0;
        s->channels[i].tcd.CITER = 0x0;
        s->channels[i].tcd.DLAST_SGA = 0x0;
        s->channels[i].tcd.CSR = 0x0;
        s->channels[i].tcd.BITER = 0x0;
    }
}

static void s32k358_dma_class_init(ObjectClass *klass, void *data){
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->desc = S32K358_EDMA_NAME;
    dc->realize = s32k358_dma_realize;
    device_class_set_props(dc, s32k358_dma_properties);
}

static const TypeInfo s32k358_dma_info = {
    .name = TYPE_S32K358_DMA,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K358DMAState),
    .instance_init = s32k358_dma_init,
    .class_init = s32k358_dma_class_init,
};

static void s32k358_dma_register_types(void)
{
    type_register_static(&s32k358_dma_info);
}

type_init(s32k358_dma_register_types)
