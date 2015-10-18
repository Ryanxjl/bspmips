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
** 文   件   名: 8259a_pic.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 18 日
**
** 描        述: INTEL 8259A PIC 驱动头文件.
*********************************************************************************************************/
#ifndef _8259A_PIC_H_
#define _8259A_PIC_H_

void disable_8259A_irq(unsigned int irq);
void enable_8259A_irq(unsigned int irq);
int i8259A_irq_pending(unsigned int irq);
void mask_and_ack_8259A(unsigned int irq);
void init_8259A(int auto_eoi);
int i8259_irq(void);

#define pic8259AInit                init_8259A
#define pic8259ADisableIrq          disable_8259A_irq
#define pic8259AEnableIrq           enable_8259A_irq
#define pic8259AIrq                 i8259_irq

#endif                                                                  /*  _8259A_PIC_H_               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
