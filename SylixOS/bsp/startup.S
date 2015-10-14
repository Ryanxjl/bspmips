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
    IMPORT_LABEL(archCacheErrorEntry)
    IMPORT_LABEL(archExceptionEntry)

;/*********************************************************************************************************
;  cache_loop
;*********************************************************************************************************/

MACRO_DEF(cache_loop   curr, end, line_sz, op)
10: cache   \op , 0(\curr)
    ADDU    \curr , \curr , \line_sz
    BNE     \curr , \end , 10b
MACRO_END()

;/*********************************************************************************************************
;  异常向量表
;*********************************************************************************************************/

    SECTION(.vector)

FUNC_DEF(vector)
     B      reset                                                       ;/*  SylixOS entry point         */
     NOP

    .org    0x10

    .org    0x200                                                       ;/*  TLB refill, 32 bit task     */
    MIPS32_TLB_REFILL_HANDLE()

    .org    0x280                                                       ;/*  XTLB refill, 64 bit task    */
1:  B       1b

    .org    0x300                                                       ;/*  Cache error exception       */
    MIPS_CACHE_ERROR_HANDLE()

    .org    0x380                                                       ;/*  General exception           */
    MIPS_EXCEPTION_HANDLE()

    .org    0x400                                                       ;/*  Catch interrupt exceptions  */
    MIPS_INTERRUPT_HANDLE()

    .org    0x480                                                       ;/*  EJTAG debug exception       */
1:  B       1b
    NOP

FUNC_END(vector)

;/*********************************************************************************************************
;  复位入口
;*********************************************************************************************************/

    SECTION(.text)

FUNC_DEF(reset)

    ;/* Clear watch registers */
    MTC0(ZERO, CP0_WATCHLO)
    MTC0(ZERO, CP0_WATCHHI)

    ;/* WP(Watch Pending), SW0/1 should be cleared */
    MTC0(ZERO, CP0_CAUSE)

    MTC0(ZERO, CP0_STATUS)

    ;/* Init Timer */
    MTC0(ZERO, CP0_COUNT)
    MTC0(ZERO, CP0_COMPARE)

    ;/* CONFIG0 register */
    LI      T0 , MIPS_UNCACHED
    MTC0(T0,   CP0_CONFIG)

    .align  2
    BAL     1f
    NOP
    PTR     _gp

1:
    LW      GP , 0(RA)

;/*********************************************************************************************************
;  在这里加入初始化最核心硬件接口 (有 BOOTLOADER 就不需要初始化 SDRAM 与 PLL)
;*********************************************************************************************************/

    ;/* Initialize any external memory */
    LA      T9 , lowlevel_init
    JALR    T9
    NOP

    ;/* Initialize caches... */
    ;//PTR_LA  t9, mips_cache_reset
    ;//jalr    t9
    ;//nop

    ;/* ... and enable them */
    ;//li  t0, BSP_CFG_SYS_MIPS_CACHE_MODE
    ;//mtc0    t0, CP0_CONFIG

;/*********************************************************************************************************
;  初始化堆栈
;*********************************************************************************************************/

    LI      T0 , -16
    LI      T1 , BSP_CFG_SYS_INIT_SP_ADDR
    AND     SP , T1 , T0                                                ;/*  强制 16 字节对齐            */

;/*********************************************************************************************************
;  初始化 DATA 段
;*********************************************************************************************************/
    ;/*
    ; * T0 = source address
    ; * T1 = target address
    ; * T2 = source end address
    ; */
    LA      T0 , _etext
    LA      T1 , _data
    LA      T2 , _edata

1:
    LW      T3 , 0(T0)
    SW      T3 , 0(T1)
    ADDU    T0 , 4
    BLT     T1 , T2, 1b
    ADDU    T1 , 4                                                      ;/* 指令优化，流水线             */

;/*********************************************************************************************************
;  清零 BSS 段
;*********************************************************************************************************/

    ;/*
    ; * GOT is now relocated. Thus __bss_start and __bss_end can be
    ; * accessed directly via $gp.
    ; */
    LA      T1 , __bss_start
    LA      T2 , __bss_end

1:
    SW      ZERO , 0(T1)
    BLT     T1 , T2 , 1b
    ADDI    T1 , PTRSIZE

;/*********************************************************************************************************
;  进入 bspInit 函数 (argc = 0, argv = NULL, frame pointer = NULL)
;*********************************************************************************************************/

    MOV     FP , SP
    LA      T9 , bspInit
    JR      T9
    NOP

FUNC_END(reset)

;/*********************************************************************************************************
;  lowlevel_init
;*********************************************************************************************************/

FUNC_DEF(lowlevel_init)
    .set mips32
    ;/*
    ; * Step 2) Establish Status Register
    ; * (set BEV, clear ERL, clear EXL, clear IE)
    ; */
    LI      T1 , 0x00400000
    MTC0(T1, CP0_STATUS)

    ;/*
    ; * Step 3) Establish CP0 Config0
    ; * (set K0=3)
    ; */
    LI      T1 , 0x00000003
    MTC0(T1, CP0_CONFIG)

    ;/*
    ; * Step 7) Establish Cause
    ; * (set IV bit)
    ; */
    LI      T1 , 0x00800000
    MTC0(T1, CP0_CAUSE)

    ;/* Establish Wired (and Random) */
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
