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
** 文   件   名: r4k_16c550_sio.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 09 月 09 日
**
** 描        述: QEMU R4K 平台 16C550 SIO 驱动配置文件
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include <SylixOS.h>
#include <driver/sio/16c550.h>
#include "16c550_sio_priv.h"
/*********************************************************************************************************
  QEMU R4K 平台 16C550 SIO 配置初始化
*********************************************************************************************************/
SIO16C550_CFG      _G_sio16C550Cfgs[BSP_CFG_16C550_SIO_NR] = {
        {
                BSP_CFG_16C550_BASE,
                0x2F76000,
                BSP_CFG_16C550_BAUD,
                BSP_CFG_16C550_VECTOR,
        }
};
/*********************************************************************************************************
  END
*********************************************************************************************************/
