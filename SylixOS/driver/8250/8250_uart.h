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
** ��   ��   ��: 8250_uart.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 08 �� 14 ��
**
** ��        ��: INTEL 8250 UART ����
*********************************************************************************************************/
#ifndef _8250_UART_H_
#define _8250_UART_H_
/*********************************************************************************************************
** ��������: uart8250PutStr
** ��������: 8250 UART ����ַ���
** �䡡��  : addrBase              8250 UART ����ַ
**           pcMsg                 �ַ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  uart8250PutStr(addr_t  addrBase, CPCHAR  pcMsg);
/*********************************************************************************************************
** ��������: uart8250PutChar
** ��������: 8250 UART ����ַ�
** �䡡��  : addrBase              8250 UART ����ַ
**           cChar                 �ַ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  uart8250PutChar(addr_t  addrBase, CHAR  cChar);
/*********************************************************************************************************
** ��������: uart8250GetChar
** ��������: 8250 UART ��ȡ�ַ�
** �䡡��  : addrBase              8250 UART ����ַ
** �䡡��  : �ַ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CHAR  uart8250GetChar(addr_t  addrBase);

#endif                                                                  /*  _8250_UART_H_               */
/*********************************************************************************************************
  END
*********************************************************************************************************/