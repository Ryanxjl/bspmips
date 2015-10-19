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
** 文   件   名: 8254_timer.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 19 日
**
** 描        述: INTEL 8254 TIMER 驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"

/* Programmable Interrupt Timer Definitions */
#define PIT_REG_COUNTER0        (BSP_CFG_ISA_IO_BASE + 0x40)
#define PIT_REG_COUNTER1        (BSP_CFG_ISA_IO_BASE + 0x41)
#define PIT_REG_COUNTER2        (BSP_CFG_ISA_IO_BASE + 0x42)
#define PIT_REG_COMMAND         (BSP_CFG_ISA_IO_BASE + 0x43)

#define PIT_CLOCK               1193182ul

/* PIT command bit defintions */
#  define PIT_OCW_BINCOUNT_BCD  (1 << 0) /* vs binary */
#  define PIT_OCW_MODE_SHIFT    (1)
#  define PIT_OCW_MODE_MASK     (7 << PIT_OCW_MODE_SHIFT)
#    define PIT_OCW_MODE_TMCNT  (0 << PIT_OCW_MODE_SHIFT)  /* Terminal count */
#    define PIT_OCW_MODE_ONESHOT (1 << PIT_OCW_MODE_SHIFT) /* One shot */
#    define PIT_OCW_MODE_RATEGEN (2 << PIT_OCW_MODE_SHIFT) /* Rate gen */
#    define PIT_OCW_MODE_SQUARE (3 << PIT_OCW_MODE_SHIFT)  /* Square wave generation */
#    define PIT_OCW_MODE_SWTRIG (4 << PIT_OCW_MODE_SHIFT)  /* Software trigger */
#    define PIT_OCW_MODE_HWTRIG (5 << PIT_OCW_MODE_SHIFT)  /* Hardware trigger */
#  define PIT_OCW_RL_SHIFT      (4)
#  define PIT_OCW_RL_MASK       (3 << PIT_OCW_RL_SHIFT)
#    define PIT_OCW_RL_LATCH    (0 << PIT_OCW_RL_SHIFT)
#    define PIT_OCW_RL_LSBONLY  (1 << PIT_OCW_RL_SHIFT)
#    define PIT_OCW_RL_MSBONLY  (2 << PIT_OCW_RL_SHIFT)
#    define PIT_OCW_RL_DATA     (3 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_COUNTER_SHIFT (6)
#  define PIT_OCW_COUNTER_MASK  (3 << PIT_OCW_COUNTER_SHIFT)
#    define PIT_OCW_COUNTER_0   (0 << PIT_OCW_COUNTER_SHIFT)
#    define PIT_OCW_COUNTER_1   (1 << PIT_OCW_COUNTER_SHIFT)
#    define PIT_OCW_COUNTER_2   (2 << PIT_OCW_COUNTER_SHIFT)

#define PIT_DIVISOR  ((UINT32)PIT_CLOCK/(uint32_t)LW_TICK_HZ)

/*********************************************************************************************************
** 函数名称: timer8254Init
** 功能描述: 初始化 8254 定时器
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  timer8254Init (VOID)
{
    UINT32   uiDivisor = PIT_DIVISOR;

    /* Send the command byte to configure counter 0 */
    write8(PIT_OCW_MODE_SQUARE|PIT_OCW_RL_DATA|PIT_OCW_COUNTER_0, PIT_REG_COMMAND);

    /* Set the PIT input frequency divisor */
    write8((UINT8)(uiDivisor & 0xff),  PIT_REG_COUNTER0);
    write8((UINT8)((uiDivisor >> 8) & 0xff), PIT_REG_COUNTER0);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
