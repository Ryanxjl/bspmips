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

;/*********************************************************************************************************
;  cache_loop
;*********************************************************************************************************/

MACRO_DEF(cache_loop   curr, end, line_sz, op)
10: cache       \op, 0(\curr)
    PTR_ADDU    \curr, \curr, \line_sz
    bne     \curr, \end, 10b
MACRO_END()

;/*********************************************************************************************************
;  setup_c0_status
;*********************************************************************************************************/

MACRO_DEF(setup_c0_status set clr)
    .set    push
    mfc0    t0, CP0_STATUS
    or  t0, M_StatusCU0 | \set | 0x1f | \clr
    xor t0, 0x1f | \clr
    mtc0    t0, CP0_STATUS
    .set    noreorder
    sll     zero, 3
    .set    pop
MACRO_END()

;/*********************************************************************************************************
;  异常向量表
;*********************************************************************************************************/

    SECTION(.vector)

FUNC_DEF(vector)
     /* U-boot entry point */
     b   reset
     nop

    .org 0x10

    .org 0x200
    /* TLB refill, 32 bit task */
1:  b   1b
     nop

    .org 0x280
    /* XTLB refill, 64 bit task */
1:  b   1b
     nop

    .org 0x300
    /* Cache error exception */
1:  b   1b
     nop

    .org 0x380
    /* General exception */
1:  b   1b
     nop

    .org 0x400
    /* Catch interrupt exceptions */
1:  b   1b
     nop

    .org 0x480
    /* EJTAG debug exception */
1:  b   1b
     nop

FUNC_END(vector)

;/*********************************************************************************************************
;  复位入口
;*********************************************************************************************************/

    SECTION(.text)

FUNC_DEF(reset)

;    /* Clear watch registers */
    MTC0    zero, CP0_WATCHLO
    MTC0    zero, CP0_WATCHHI

;    /* WP(Watch Pending), SW0/1 should be cleared */
    mtc0    zero, CP0_CAUSE

    setup_c0_status STATUS_SET 0

;    /* Init Timer */
    mtc0    zero, CP0_COUNT
    mtc0    zero, CP0_COMPARE

;    /* CONFIG0 register */
    li      t0, CONF_CM_UNCACHED
    mtc0    t0, CP0_CONFIG

    .align  2
    bal     1f
    nop
    PTR _gp

1:
    PTR_L   gp, 0(ra)

;/*********************************************************************************************************
;  TODO 在这里加入初始化最核心硬件接口 (有 BOOTLOADER 就不需要初始化 SDRAM 与 PLL)
;*********************************************************************************************************/

;    /* Initialize any external memory */
    PTR_LA  t9, lowlevel_init
    jalr    t9
    nop

;    /* Initialize caches... */
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
    PTR_LI  t0, -16
    PTR_LI  t1, BSP_CFG_SYS_INIT_SP_ADDR
    and sp, t1, t0      ;# force 16 byte alignment
    move    fp, sp

;/*********************************************************************************************************
;  初始化 DATA 段
;*********************************************************************************************************/
    /*
     * t0 = source address
     * t1 = target address
     * t2 = source end address
     */
    PTR_LA  t0, _etext
    PTR_LA  t1, _data
    PTR_LA  t2, _edata

1:
    lw  t3, 0(t0)
    sw  t3, 0(t1)
    PTR_ADDU t0, 4
    blt t1, t2, 1b
    PTR_ADDU t1, 4  ;/*指令优化，流水线*/

;/*********************************************************************************************************
;  清零 BSS 段
;*********************************************************************************************************/

    /*
     * Clear BSS
     *
     * GOT is now relocated. Thus __bss_start and __bss_end can be
     * accessed directly via $gp.
     */
    PTR_LA  t1, __bss_start
    PTR_LA  t2, __bss_end

1:
    PTR_S   zero, 0(t1)
    blt t1, t2, 1b
    PTR_ADDI t1, PTRSIZE

;/*********************************************************************************************************
;  进入 bspInit 函数 (argc = 0, argv = NULL, frame pointer = NULL)
;*********************************************************************************************************/

    PTR_LA   t9, bspInit
    jr       t9
    nop

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
    li  t1, 0x00400000
    mtc0    t1, CP0_STATUS

;    /*
;     * Step 3) Establish CP0 Config0
;     * (set K0=3)
;     */
    li  t1, 0x00000003
    mtc0    t1, CP0_CONFIG

;    /*
;     * Step 7) Establish Cause
;     * (set IV bit)
;     */
    li  t1, 0x00800000
    mtc0    t1, CP0_CAUSE

;    /* Establish Wired (and Random) */
    mtc0    zero, CP0_WIRED
    nop

    jr  ra
    nop
FUNC_END(lowlevel_init)

;/*********************************************************************************************************
;  mips_cache_reset
;*********************************************************************************************************/

FUNC_DEF(mips_cache_reset)
    li  t2, BSP_CFG_SYS_ICACHE_SIZE
    li  t8, BSP_CFG_SYS_CACHELINE_SIZE

    li  t3, BSP_CFG_SYS_DCACHE_SIZE
    li  t9, BSP_CFG_SYS_CACHELINE_SIZE

;    /*
;     * The TagLo registers used depend upon the CPU implementation, but the
;     * architecture requires that it is safe for software to write to both
;     * TagLo selects 0 & 2 covering supported cases.
;     */
    mtc0   zero, CP0_TAGLO
    mtc0   zero, CP0_TAGLO, 2

;    /*
;     * Initialize the I-cache first,
;     */

    blez        t2, 1f
    PTR_LI      t0, BSP_CFG_INDEX_BASE
    PTR_ADDU    t1, t0, t2
    /* clear tag to invalidate */
    cache_loop  t0, t1, t8, BSP_CFG_INDEX_STORE_TAG_I

;    /*
;     * then initialize D-cache.
;     */

1:  blez        t3, 3f
    PTR_LI      t0, BSP_CFG_INDEX_BASE
    PTR_ADDU    t1, t0, t3
    /* clear all tags */
    cache_loop  t0, t1, t9, BSP_CFG_INDEX_STORE_TAG_D

3:  jr  ra
    nop
FUNC_END(mips_cache_reset)

FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
