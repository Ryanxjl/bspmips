/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: config.h
**
** 创   建   人: Ryan.xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 09 日
**
** 描        述: 处理器配置.
*********************************************************************************************************/
#ifndef __CONFIG_H
#define __CONFIG_H
/*********************************************************************************************************
  地址空间配置
*********************************************************************************************************/
#define BSP_CFG_ROM_BASE            0xBFC00000
#define BSP_CFG_ROM_PA_BASE         0x1FC00000

#define BSP_CFG_ROM_SIZE            (12 * 1024 * 1024)

#define BSP_CFG_RAM_BASE            (0x80000000)
#define BSP_CFG_RAM_PA_BASE         (0x00000000)

#define BSP_CFG_RAM_SIZE            (128 * 1024 * 1024)

#define BSP_CFG_DATA_BASE           (BSP_CFG_RAM_SIZE + 6 * 1024 * 1024)

#define BSP_CFG_COMMON_MEM_SIZE     (44 * 1024 * 1024)

#define BSP_CFG_SYS_INIT_SP_ADDR    (BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE)
/*********************************************************************************************************
  16C550
*********************************************************************************************************/
#define BSP_CFG_16C550_SIO_NR       1

#endif                                                                  /*  __CONFIG_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
