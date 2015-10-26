/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: 8259a_pic.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 18 日
**
** 描        述: INTEL 8259A PIC 驱动源文件.
*********************************************************************************************************/
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Code to handle x86 style IRQs plus some generic interrupt stuff.
 *
 * Copyright (C) 1992 Linus Torvalds
 * Copyright (C) 1994 - 2000 Ralf Baechle
 */
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"

/* i8259A PIC registers */
#define PIC_MASTER_CMD      (BSP_CFG_ISA_IO_BASE + 0x20)
#define PIC_MASTER_IMR      (BSP_CFG_ISA_IO_BASE + 0x21)
#define PIC_MASTER_ISR      PIC_MASTER_CMD
#define PIC_MASTER_POLL     PIC_MASTER_ISR
#define PIC_MASTER_OCW3     PIC_MASTER_ISR
#define PIC_SLAVE_CMD       (BSP_CFG_ISA_IO_BASE + 0xa0)
#define PIC_SLAVE_IMR       (BSP_CFG_ISA_IO_BASE + 0xa1)

/* i8259A PIC related value */
#define PIC_CASCADE_IR      2
#define MASTER_ICW4_DEFAULT 0x01
#define SLAVE_ICW4_DEFAULT  0x01
#define PIC_ICW4_AEOI       2

/*
 * This is the 'legacy' 8259A Programmable Interrupt Controller,
 * present in the majority of PC/AT boxes.
 * plus some generic x86 specific things if generic specifics makes
 * any sense at all.
 * this file should become arch/i386/kernel/irq.c when the old irq.c
 * moves to arch independent land
 */

/*
 * 8259A PIC functions to handle ISA devices:
 */

/*
 * This contains the irq mask for both 8259A irq controllers,
 */
static unsigned int cached_irq_mask = 0xffff;

#define cached_master_mask  (cached_irq_mask)
#define cached_slave_mask   (cached_irq_mask >> 8)

void disable_8259A_irq(unsigned int irq)
{
    unsigned int mask;

    irq -= BSP_CFG_8259A_VECTOR_BASE;

    mask = 1 << irq;
    cached_irq_mask |= mask;
    if (irq & 8)
        write8(cached_slave_mask, PIC_SLAVE_IMR);
    else
        write8(cached_master_mask, PIC_MASTER_IMR);
}

unsigned char isenable_8259A_irq(unsigned int irq)
{
    unsigned int mask;

    irq -= BSP_CFG_8259A_VECTOR_BASE;

    mask = 1 << irq;
    if (cached_irq_mask & mask) {
        return 0;
    } else {
        return 1;
    }
}

void enable_8259A_irq(unsigned int irq)
{
    unsigned int mask;

    irq -= BSP_CFG_8259A_VECTOR_BASE;

    mask = ~(1 << irq);
    cached_irq_mask &= mask;
    if (irq & 8)
        write8(cached_slave_mask, PIC_SLAVE_IMR);
    else
        write8(cached_master_mask, PIC_MASTER_IMR);
}

int i8259A_irq_pending(unsigned int irq)
{
    unsigned int mask;
    int ret;

    irq -= BSP_CFG_8259A_VECTOR_BASE;

    mask = 1 << irq;
    if (irq < 8)
        ret = read8(PIC_MASTER_CMD) & mask;
    else
        ret = read8(PIC_SLAVE_CMD) & (mask >> 8);

    return ret;
}

/*
 * This function assumes to be called rarely. Switching between
 * 8259A registers is slow.
 * This has to be protected by the irq controller spinlock
 * before being called.
 */
static inline int i8259A_irq_real(unsigned int irq)
{
    int value;
    int irqmask = 1 << irq;

    if (irq < 8) {
        write8(0x0B, PIC_MASTER_CMD); /* ISR register */
        value = read8(PIC_MASTER_CMD) & irqmask;
        write8(0x0A, PIC_MASTER_CMD); /* back to the IRR register */
        return value;
    }
    write8(0x0B, PIC_SLAVE_CMD);  /* ISR register */
    value = read8(PIC_SLAVE_CMD) & (irqmask >> 8);
    write8(0x0A, PIC_SLAVE_CMD);  /* back to the IRR register */
    return value;
}

/*
 * Careful! The 8259A is a fragile beast, it pretty
 * much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI
 * to the two 8259s is important!
 */
void mask_and_ack_8259A(unsigned int irq)
{
    unsigned int irqmask;

    irq -= BSP_CFG_8259A_VECTOR_BASE;

    irqmask = 1 << irq;

    /*
     * Lightweight spurious IRQ detection. We do not want
     * to overdo spurious IRQ handling - it's usually a sign
     * of hardware problems, so we only do the checks we can
     * do without slowing down good hardware unnecessarily.
     *
     * Note that IRQ7 and IRQ15 (the two spurious IRQs
     * usually resulting from the 8259A-1|2 PICs) occur
     * even if the IRQ is masked in the 8259A. Thus we
     * can check spurious 8259A IRQs without doing the
     * quite slow i8259A_irq_real() call for every IRQ.
     * This does not cover 100% of spurious interrupts,
     * but should be enough to warn the user that there
     * is something bad going on ...
     */
    if (cached_irq_mask & irqmask)
        goto spurious_8259A_irq;
    cached_irq_mask |= irqmask;

handle_real_irq:
    if (irq & 8) {
        read8(PIC_SLAVE_IMR); /* DUMMY - (do we need this?) */
        write8(cached_slave_mask, PIC_SLAVE_IMR);
        write8(0x60+(irq&7), PIC_SLAVE_CMD);/* 'Specific EOI' to slave */
        write8(0x60+PIC_CASCADE_IR, PIC_MASTER_CMD); /* 'Specific EOI' to master-IRQ2 */
    } else {
        read8(PIC_MASTER_IMR);    /* DUMMY - (do we need this?) */
        write8(cached_master_mask, PIC_MASTER_IMR);
        write8(0x60+irq, PIC_MASTER_CMD); /* 'Specific EOI to master */
    }
    return;

spurious_8259A_irq:
    /*
     * this is the slow path - should happen rarely.
     */
    if (i8259A_irq_real(irq))
        /*
         * oops, the IRQ _is_ in service according to the
         * 8259A - not spurious, go handle it.
         */
        goto handle_real_irq;

    {
        static int spurious_irq_mask;
        /*
         * At this point we can be sure the IRQ is spurious,
         * lets ACK and report it. [once per IRQ]
         */
        if (!(spurious_irq_mask & irqmask)) {
            printk(KERN_DEBUG "spurious 8259A interrupt: IRQ%d.\n", irq);
            spurious_irq_mask |= irqmask;
        }

        /*
         * Theoretically we do not have to handle this IRQ,
         * but in Linux this does not cause problems and is
         * simpler for us.
         */
        goto handle_real_irq;
    }
}

void init_8259A(int auto_eoi)
{
    write8(0xff, PIC_MASTER_IMR); /* mask all of 8259A-1 */
    write8(0xff, PIC_SLAVE_IMR);  /* mask all of 8259A-2 */

    /*
     * write8 - this has to work on a wide range of PC hardware.
     */
    write8(0x11, PIC_MASTER_CMD);   /* ICW1: select 8259A-1 init */
    write8(BSP_CFG_8259A_VECTOR_BASE + 0, PIC_MASTER_IMR);    /* ICW2: 8259A-1 IR0 mapped to I8259A_IRQ_BASE + 0x00 */
    write8(1U << PIC_CASCADE_IR, PIC_MASTER_IMR);   /* 8259A-1 (the master) has a slave on IR2 */
    if (auto_eoi)   /* master does Auto EOI */
        write8(MASTER_ICW4_DEFAULT | PIC_ICW4_AEOI, PIC_MASTER_IMR);
    else        /* master expects normal EOI */
        write8(MASTER_ICW4_DEFAULT, PIC_MASTER_IMR);

    write8(0x11, PIC_SLAVE_CMD);    /* ICW1: select 8259A-2 init */
    write8(BSP_CFG_8259A_VECTOR_BASE + 8, PIC_SLAVE_IMR); /* ICW2: 8259A-2 IR0 mapped to I8259A_IRQ_BASE + 0x08 */
    write8(PIC_CASCADE_IR, PIC_SLAVE_IMR);  /* 8259A-2 is a slave on master's IR2 */
    write8(SLAVE_ICW4_DEFAULT, PIC_SLAVE_IMR); /* (slave's support for AEOI in flat mode is to be investigated) */

    bspDelayUs(100);        /* wait for 8259A to initialize */

    write8(cached_master_mask, PIC_MASTER_IMR); /* restore master IRQ mask */
    write8(cached_slave_mask, PIC_SLAVE_IMR);   /* restore slave IRQ mask */
}

/*
 * Do the traditional i8259 interrupt polling thing.  This is for the few
 * cases where no better interrupt acknowledge method is available and we
 * absolutely must touch the i8259.
 */
int i8259_irq(void)
{
    int irq;

    /* Perform an interrupt acknowledge cycle on controller 1. */
    write8(0x0C, PIC_MASTER_CMD);     /* prepare for poll */
    irq = read8(PIC_MASTER_CMD) & 7;
    if (irq == PIC_CASCADE_IR) {
        /*
         * Interrupt is cascaded so perform interrupt
         * acknowledge on controller 2.
         */
        write8(0x0C, PIC_SLAVE_CMD);      /* prepare for poll */
        irq = (read8(PIC_SLAVE_CMD) & 7) + 8;
    }

    if (unlikely(irq == 7)) {
        /*
         * This may be a spurious interrupt.
         *
         * Read the interrupt status register (ISR). If the most
         * significant bit is not set then there is no valid
         * interrupt.
         */
        write8(0x0B, PIC_MASTER_ISR);     /* ISR register */
        if(~read8(PIC_MASTER_ISR) & 0x80)
            irq = -1;
    }

    return likely(irq >= 0) ? irq + BSP_CFG_8259A_VECTOR_BASE : irq;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
