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
** ��   ��   ��: 16c550_sio.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 08 �� 27 ��
**
** ��        ��: 16C550 SIO ����
*********************************************************************************************************/
#ifndef _16C550_SIO_H_
#define _16C550_SIO_H_
/*********************************************************************************************************
** ��������: sioChan16C550Create
** ��������: ����һ�� SIO ͨ��
** �䡡��  : uiChannel                 Ӳ��ͨ����
** �䡡��  : SIO ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
SIO_CHAN  *sioChan16C550Create(UINT  uiChannel);

#endif                                                                  /*  _16C550_SIO_H_              */
/*********************************************************************************************************
  END
*********************************************************************************************************/