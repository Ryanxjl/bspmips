;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                SylixOS(TM)  LW : long wing
;**
;**                               Copyright All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: startup.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 09 日
;**
;** 描        述: MIPS 处理器系统启动.
;*********************************************************************************************************/
    
#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "config.h"

    FILE_BEGIN()
;/*********************************************************************************************************
;  引入外部符号
;*********************************************************************************************************/

    IMPORT_LABEL(bspInit)
    IMPORT_LABEL(archInterruptEntry)

;/*********************************************************************************************************
;  cache_loop
;*********************************************************************************************************/

MACRO_DEF(cache_loop   curr, end, line_sz, op)
10: cache   \op , 0(\curr)
    ADDU    \curr , \curr , \line_sz
    BNE     \curr , \end , 10b
MACRO_END()

;/*********************************************************************************************************
;  setup_c0_status
;*********************************************************************************************************/

MACRO_DEF(setup_c0_status set clr)
    .set push
    MFC0(T0, CP0_STATUS)
    OR      T0 ,  M_StatusCU0 | \set | 0x1f | \clr
    XOR     T0 ,  0X1F | \clr
    MTC0(T0, CP0_STATUS)
    .set noreorder
    SLL     ZERO , 3
    .set pop
MACRO_END()

;/*********************************************************************************************************
;  异常向量表
;*********************************************************************************************************/

    SECTION(.vector)

FUNC_DEF(vector)
;     /* U-boot entry point */
     b   reset
     NOP

    .org 0x10

    .org 0x200
;    /* TLB refill, 32 bit task */
1:  b   1b
    NOP

    .org 0x280
;    /* XTLB refill, 64 bit task */
1:  b   1b
    NOP

    .org 0x300
;    /* Cache error exception */
1:  b   1b
    NOP

    .org 0x380
;    /* General exception */
    LA   T7, archInterruptEntry
    JR   T7
    NOP

    .org 0x400
;    /* Catch interrupt exceptions */
    LA   T7, archInterruptEntry
    JR   T7
    NOP

    .org 0x480
;    /* EJTAG debug exception */
1:  b   1b
    NOP

FUNC_END(vector)

;/*********************************************************************************************************
;  复位入口
;*********************************************************************************************************/

    SECTION(.text)

FUNC_DEF(reset)

;    /* Clear watch registers */
    MTC0(ZERO, CP0_WATCHLO)
    MTC0(ZERO, CP0_WATCHHI)

;    /* WP(Watch Pending), SW0/1 should be cleared */
    MTC0(ZERO, CP0_CAUSE)

    setup_c0_status STATUS_SET 0

;    /* Init Timer */
    MTC0(ZERO, CP0_COUNT)
    MTC0(ZERO, CP0_COMPARE)

;    /* CONFIG0 register */
    LI      T0 ,    CONF_CM_UNCACHED
    MTC0(T0,   CP0_CONFIG)

    .align  2
    BAL   1f
    NOP
    PTR   _gp

1:
    LW      GP ,    0(RA)

;/*********************************************************************************************************
;  在这里加入初始化最核心硬件接口 (有 BOOTLOADER 就不需要初始化 SDRAM 与 PLL)
;*********************************************************************************************************/

;    /* Initialize any external memory */
    LA      T9 ,  lowlevel_init
    JALR    T9
    NOP

;   /* Initialize caches... */
    ;//PTR_LA  t9, mips_cache_reset
    ;//jalr    t9
    ;//nop

;    /* ... and enable them */
    ;//li  t0, BSP_CFG_SYS_MIPS_CACHE_MODE
    ;//mtc0    t0, CP0_CONFIG

;/*********************************************************************************************************
;  初始化堆栈
;*********************************************************************************************************/

;    /* Set up temporary stack */
    LI      T0 ,   -16
    LI      T1 ,   BSP_CFG_SYS_INIT_SP_ADDR
    AND     SP ,   T1 ,   T0                                                   ;/* force 16 byte alignment     */
    MOV     FP ,   SP

;/*********************************************************************************************************
;  初始化 DATA 段
;*********************************************************************************************************/
;    /*
;     * t0 = source address
;     * t1 = target address
;     * t2 = source end address
;     */
    LA      T0 ,   _etext
    LA      T1 ,   _data
    LA      T2 ,   _edata

1:
    LW      T3 ,   0(T0)
    SW      T3 ,   0(T1)
    ADDU    T0 ,   4
    BLT     T1 ,   T2,  1b
    ADDU    T1 ,   4                                                    ;/*指令优化，流水线             */

;/*********************************************************************************************************
;  清零 BSS 段
;*********************************************************************************************************/

;    /*
;     * Clear BSS
;     *
;     * GOT is now relocated. Thus __bss_start and __bss_end can be
;     * accessed directly via $gp.
;     */
    LA      T1 ,   __bss_start
    LA      T2 ,   __bss_end

1:
    SW      ZERO , 0(T1)
    BLT     T1 ,   T2 ,  1b
    ADDI    T1 ,   PTRSIZE

;/*********************************************************************************************************
;  进入 bspInit 函数 (argc = 0, argv = NULL, frame pointer = NULL)
;*********************************************************************************************************/

    LA      T9 ,   bspInit
    JR      T9
    NOP

FUNC_END(reset)

;/*********************************************************************************************************
;  lowlevel_init
;*********************************************************************************************************/

FUNC_DEF(lowlevel_init)
    .set mips32
;    /*
;     * Step 2) Establish Status Register
;     * (set BEV, clear ERL, clear EXL, clear IE)
;     */
    LI      T1 ,   0x00400000
    MTC0(T1,   CP0_STATUS)

;    /*
;     * Step 3) Establish CP0 Config0
;     * (set K0=3)
;     */
    LI      T1 ,   0x00000003
    MTC0(T1,   CP0_CONFIG)

;    /*
;     * Step 7) Establish Cause
;     * (set IV bit)
;     */
    LI      T1 ,   0x00800000
    MTC0(T1,   CP0_CAUSE)

;    /* Establish Wired (and Random) */
    MTC0(ZERO, CP0_WIRED)

    JR      RA
    NOP
FUNC_END(lowlevel_init)

;/*********************************************************************************************************
;  mips_cache_reset
;*********************************************************************************************************/

FUNC_DEF(mips_cache_reset)
    JR      RA
    NOP
FUNC_END(mips_cache_reset)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
