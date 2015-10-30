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
** 文   件   名: ne2000.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 10 月 21 日
**
** 描        述: NE2000 以太网芯片驱动
*********************************************************************************************************/
#ifndef NE2000_H_
#define NE2000_H_

#define ETHER_ADDR_LEN 6
typedef struct ne2000_data {
    addr_t            uiBaseAddr;
    int               irq;
    u8_t              ucMacAddr[ETHER_ADDR_LEN];
} NE2000_DATA;

/*********************************************************************************************************
** 函数名称: ne2000_netif_init
** 功能描述: 初始化以太网网络接口
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
err_t ne2000_netif_init(struct netif *netif);

#endif                                                                  /*  NE2000_H_                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

