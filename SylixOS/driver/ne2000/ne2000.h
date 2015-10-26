/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: ne2000.h
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 10 �� 21 ��
**
** ��        ��: NE2000 ��̫��оƬ����
*********************************************************************************************************/
#ifndef NE2000_H_
#define NE2000_H_

typedef struct ne2000_data {
    addr_t            uiBaseAddr;
    int               irq;
    u8_t              ucMacAddr[6];
} NE2000_DATA;

/*********************************************************************************************************
** ��������: ne2000_netif_init
** ��������: ��ʼ����̫������ӿ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
err_t ne2000_netif_init(struct netif *netif);

#endif                                                                  /*  NE2000_H_                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
