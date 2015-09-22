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
            BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  均可被 DMA 使用             */
    },

    {
            BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE + (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  均可被 DMA 使用             */
    },
};
/*********************************************************************************************************
  初始化内存类型与布局 (VMM 管理区的内存不需要在全局初始化表中初始化)
*********************************************************************************************************/

LW_MMU_GLOBAL_DESC  _G_globaldescMap[] = {

#ifdef __BOOT_INRAM
    /*
     *  ARM except table
     */
    {                                                                   /*  中断向量表                  */
        BSP_CFG_ROM_BASE,                                               /*  ARM except table            */
        BSP_CFG_RAM_BASE,                                               /*  RAM except table            */
        LW_CFG_VMM_PAGE_SIZE,                                           /*  one page size               */
        (LW_VMM_FLAG_EXEC)                                              /*  文字池类型                  */
    },
    /*
     *  nor-flash area
     */
    {
        (BSP_CFG_ROM_BASE + LW_CFG_VMM_PAGE_SIZE),                      /*  flash 剩下的空间直接映射    */
        (BSP_CFG_ROM_BASE + LW_CFG_VMM_PAGE_SIZE),
        (BSP_CFG_ROM_SIZE - LW_CFG_VMM_PAGE_SIZE),
        (LW_VMM_FLAG_EXEC)                                              /*  文字池类型                  */
    },
#else                                                                   /*  __DEBUG_IN_FLASH            */
    /*
     *  nor-flash area
     */
    {
        (BSP_CFG_ROM_BASE),                                             /*  flash 直接映射              */
        (BSP_CFG_ROM_BASE),
        (BSP_CFG_ROM_SIZE),
        (LW_VMM_FLAG_EXEC)                                              /*  文字池类型                  */
    },
#endif                                                                  /*  __DEBUG_IN_RAM              */
    
    /*
     *  kernel space
     */
    {
        (BSP_CFG_RAM_BASE),                                             /*  SylixOS Kernel Text         */
        (BSP_CFG_RAM_BASE),
        (BSP_CFG_DATA_BASE - BSP_CFG_RAM_BASE),
        (LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR)                           /*  此段可修改, 方便调试        */
    },
    
    {                                                                   /*  SylixOS Kernel Memory       */
        (BSP_CFG_DATA_BASE),
        (BSP_CFG_DATA_BASE),
        (BSP_CFG_COMMON_MEM_SIZE - (BSP_CFG_DATA_BASE - BSP_CFG_RAM_BASE)),
        (LW_VMM_FLAG_RDWR)                                              /*  状态属性 CB                 */
    },
    
    /*
     *  TODO: 加入硬件特殊功能寄存器的映射, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    {
        0x48000000,                                                     /*  特殊功能寄存器的物理地址    */
        0x48000000,                                                     /*  映射到的虚拟地址            */
        LW_CFG_VMM_PAGE_SIZE,                                           /*  映射的大小                  */
        (LW_VMM_FLAG_DMA)                                               /*  映射的属性                  */
    },
#endif                                                                  /*  参考代码结束                */
    
    {                                                                   /*  结束                        */
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
