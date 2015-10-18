/**********************************************************************************************************
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
** 文   件   名: bspMap.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 23 日
**
** 描        述: C 程序入口部分. 物理分页空间与全局映射关系表定义.
*********************************************************************************************************/

#ifndef __BSPMAP_H
#define __BSPMAP_H

/*********************************************************************************************************
   内存分配关系图

    +-----------------------+--------------------------------+
    |       通用内存区      |          VMM 管理区            |
    |         CACHE         |                                |
    +-----------------------+--------------------------------+

*********************************************************************************************************/

/*********************************************************************************************************
  physical memory zone
*********************************************************************************************************/
#ifdef  __BSPINIT_MAIN_FILE

LW_VMM_ZONE_DESC    _G_zonedescGlobal[] = {
    {
            BSP_CFG_RAM_PA_BASE + BSP_CFG_COMMON_MEM_SIZE,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  均可被 DMA 使用             */
    },

    {
            BSP_CFG_RAM_PA_BASE + BSP_CFG_COMMON_MEM_SIZE +
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  均可被 DMA 使用             */
    },
};
/*********************************************************************************************************
  初始化内存类型与布局 (VMM 管理区的内存不需要在全局初始化表中初始化)
*********************************************************************************************************/

LW_MMU_GLOBAL_DESC  _G_globaldescMap[] = {
#ifdef __BOOT_INRAM
    {
            BSP_CFG_RAM_BASE,
            BSP_CFG_RAM_PA_BASE,
            BSP_CFG_KERNEL_SIZE,                                        /*  SylixOS Kernel Text         */
            (LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR)                       /*  此段可修改, 方便调试        */
    },
#else
    {
            BSP_CFG_ROM_BASE,
            BSP_CFG_ROM_PA_BASE,
            BSP_CFG_KERNEL_SIZE,                                        /*  SylixOS Kernel Text         */
            LW_VMM_FLAG_EXEC
    },
#endif

    {
            BSP_CFG_DATA_BASE,
            BSP_CFG_DATA_PA_BASE,
            BSP_CFG_COMMON_MEM_SIZE,
            LW_VMM_FLAG_RDWR
    },

    {
            BSP_CFG_ISA_IO_BASE,
            BSP_CFG_ISA_IO_PA_BASE,
            BSP_CFG_ISA_IO_SIZE,
            LW_VMM_FLAG_DMA
    },

    {
            BSP_CFG_ISA_MEM_BASE,
            BSP_CFG_ISA_MEM_PA_BASE,
            BSP_CFG_ISA_MEM_SIZE,
            LW_VMM_FLAG_DMA
    },

    {
            0,
            0,
            0,
            0
    }
};

#endif                                                                  /*  __BSPINIT_MAIN_FILE         */
#endif                                                                  /*  __BSPMAP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
