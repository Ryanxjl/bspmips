/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: sio.c
**
** ��   ��   ��: Jiao.JinXing(������)
**
** �ļ���������: 2015 �� 08 �� 14 ��
**
** ��        ��: ���� SIO ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sio.h"
/*********************************************************************************************************
  UART �������
*********************************************************************************************************/
#define BSP_CFG_UART_NR                     1
#define BSP_CFG_UART_DEFAULT_BAUD           SIO_BAUD_115200
#define BSP_CFG_UART_DEFAULT_OPT            (CREAD | CS8)
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  bspDebugPutc(CHAR  cChar);
extern CHAR  bspDebugGetc(VOID);
/*********************************************************************************************************
** ��������: debugUartChannelVaild
** ��������: �ж� UART ͨ�����Ƿ���Ч
** �䡡��  : uiChannel                     Ӳ��ͨ����
** �䡡��  : LW_FALSE OR LW_TRUE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE BOOL  debugUartChannelVaild (INT  uiChannel)
{
    switch (uiChannel) {
    case 0:
        return (LW_TRUE);

    default:
        return (LW_FALSE);
    }
}
/*********************************************************************************************************
  STREAM UART CHANNEL (SIO CHANNEL)
*********************************************************************************************************/
typedef struct {

    SIO_DRV_FUNCS              *pdrvFuncs;                              /*  SIO ����������              */

    INT                       (*pcbGetTxChar)();                        /*  �жϻص�����                */
    INT                       (*pcbPutRcvChar)();

    PVOID                       pvGetTxArg;                             /*  �ص���������                */
    PVOID                       pvPutRcvArg;

    INT                         iChannelMode;                           /*  ͨ��ģʽ                    */

    UCHAR                     (*pfuncHwInByte)(INT);                    /*  ����Ӳ������һ���ֽ�        */
    VOID                      (*pfuncHwOutByte)(INT, CHAR);             /*  ����Ӳ������һ���ֽ�        */

    INT                         iBaud;                                  /*  ������                      */

    INT                         iHwOption;                              /*  Ӳ��ѡ��                    */

} __SIO_CHANNEL;
typedef __SIO_CHANNEL          *__PSIO_CHANNEL;                         /*  ָ������                    */
/*********************************************************************************************************
  SIO ͨ�����ƿ�
*********************************************************************************************************/
static __SIO_CHANNEL            __G_sioChannels[BSP_CFG_UART_NR];
/*********************************************************************************************************
  SIO ��������
*********************************************************************************************************/
static INT   debugSioIoctl(SIO_CHAN  *psiochanChan,
                        INT        iCmd,
                        LONG       lArg);                             	/*  �˿ڿ���                    */
static INT   debugSioStartup(SIO_CHAN  *psiochanChan);                   	/*  ����                        */
static INT   debugSioCbInstall(SIO_CHAN        *psiochanChan,
                            INT              iCallbackType,
                            VX_SIO_CALLBACK  callbackRoute,
                            PVOID            pvCallbackArg);          	/*  ��װ�ص�                    */
static INT   debugSioPollRxChar(SIO_CHAN    *psiochanChan,
                             PCHAR        pcInChar);                  	/*  ��ѯ����                    */
static INT   debugSioPollTxChar(SIO_CHAN    *psiochanChan,
                             CHAR         cOutChar);                  	/*  ��ѯ����                    */
/*********************************************************************************************************
  SIO ��������
*********************************************************************************************************/
static SIO_DRV_FUNCS    __G_sioDrvFuncs = {
             (INT (*)(SIO_CHAN *,INT, PVOID))debugSioIoctl,
             debugSioStartup,
             debugSioCbInstall,
             debugSioPollRxChar,
             debugSioPollTxChar
};
/*********************************************************************************************************
** ��������: sioChanCreatePoll
** ��������: ����һ�� SIO ͨ��
** �䡡��  : uiChannel                 Ӳ��ͨ����
** �䡡��  : SIO ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
SIO_CHAN    *sioChanCreatePoll (UINT   uiChannel)
{
    __PSIO_CHANNEL          psiochanUart;
    
    if (!debugUartChannelVaild(uiChannel)) {
        return  (LW_NULL);                                              /*  ͨ���Ŵ���                  */
    }

    psiochanUart               = &__G_sioChannels[uiChannel];
    psiochanUart->pdrvFuncs    = &__G_sioDrvFuncs;                      /*  SIO FUNC                    */
    psiochanUart->iChannelMode = SIO_MODE_POLL;                         /*  ʹ�� POLL ģʽ              */
    psiochanUart->iBaud        = BSP_CFG_UART_DEFAULT_BAUD;             /*  ��ʼ��������                */
    psiochanUart->iHwOption    = BSP_CFG_UART_DEFAULT_OPT;              /*  ��ʼ��Ӳ��״̬              */

    return  ((SIO_CHAN *)psiochanUart);
}
/*********************************************************************************************************
** ��������: debugSioCbInstall
** ��������: SIO ͨ����װ�ص�����
** �䡡��  : psiochanChan                 SIO ͨ��
**           iCallbackType                �ص�����
**           callbackRoute                �ص�����
**           pvCallbackArg                �ص�����
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  debugSioCbInstall (SIO_CHAN        *psiochanChan,
                            INT              iCallbackType,
                            VX_SIO_CALLBACK  callbackRoute,
                            PVOID            pvCallbackArg)
{
    __PSIO_CHANNEL          psiochanUart = (__PSIO_CHANNEL)psiochanChan;
    
    switch (iCallbackType) {
    
    case SIO_CALLBACK_GET_TX_CHAR:                                      /*  ���ͻص纯��                */
        psiochanUart->pcbGetTxChar = callbackRoute;
        psiochanUart->pvGetTxArg   = pvCallbackArg;
        return  (ERROR_NONE);
        
    case SIO_CALLBACK_PUT_RCV_CHAR:                                     /*  ���ջص纯��                */
        psiochanUart->pcbPutRcvChar = callbackRoute;
        psiochanUart->pvPutRcvArg   = pvCallbackArg;
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: debugSioPollRxChar
** ��������: SIO ͨ����ѯ����
** �䡡��  : psiochanChan                 SIO ͨ��
**           pcInChar                     ���յ��ֽ�
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  debugSioPollRxChar (SIO_CHAN  *psiochanChan, PCHAR  pcInChar)
{
    *pcInChar = bspDebugGetc();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: debugSioDoRx
** ��������: SIO ͨ����ѯ����
** �䡡��  : psiochanChan                 SIO ͨ��
** �䡡��  : LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *debugSioDoRx (SIO_CHAN  *psiochanChan)
{
    __PSIO_CHANNEL          psiochanUart = (__PSIO_CHANNEL)psiochanChan;
    CHAR 					cChar;

	while (psiochanChan) {
		if (debugSioPollRxChar(psiochanChan, &cChar) == ERROR_NONE) {
	        psiochanUart->pcbPutRcvChar(psiochanUart->pvPutRcvArg, cChar);
	    }
	}

	return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: debugSioPollTxChar
** ��������: SIO ͨ����ѯ����
** �䡡��  : psiochanChan                 SIO ͨ��
**           cOutChar                     ���͵��ֽ�
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  debugSioPollTxChar (SIO_CHAN  *psiochanChan, CHAR  cOutChar)
{
	bspDebugPutc(cOutChar);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: debugSioStartup
** ��������: SIO ͨ������(û��ʹ���ж�)
** �䡡��  : psiochanChan                 SIO ͨ��
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  debugSioStartup (SIO_CHAN  *psiochanChan)
{
    __PSIO_CHANNEL          psiochanUart = (__PSIO_CHANNEL)psiochanChan;
    CHAR                    cChar;

	while (psiochanUart->pcbGetTxChar(psiochanUart->pvGetTxArg, &cChar) == ERROR_NONE) {
		debugSioPollTxChar(psiochanChan, cChar);
	}

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: debugSioIoctl
** ��������: SIO ͨ������
** �䡡��  : psiochanChan                 SIO ͨ��
**           iCmd                         ����
**           lArg                         ����
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  debugSioIoctl (SIO_CHAN  *psiochanChan, INT  iCmd, LONG  lArg)
{
    __PSIO_CHANNEL          psiochanUart = (__PSIO_CHANNEL)psiochanChan;
    LW_CLASS_THREADATTR     threakattr;
    
    switch (iCmd) {
    
    case SIO_BAUD_SET:                                                  /*  ���ò�����                  */
        psiochanUart->iBaud = (INT)lArg;                                /*  ��¼������                  */
        break;
        
    case SIO_BAUD_GET:                                                  /*  ��ò�����                  */
        *((LONG *)lArg) = psiochanUart->iBaud;
        break;
    
    case SIO_HW_OPTS_SET:                                               /*  ����Ӳ������                */
        psiochanUart->iHwOption = (INT)lArg;                            /*  ��¼Ӳ������                */
        break;
        
    case SIO_HW_OPTS_GET:                                               /*  ��ȡӲ������                */
        *((LONG *)lArg) = psiochanUart->iHwOption;
        break;
    
    case SIO_OPEN:                                                      /*  �򿪴���                    */
        API_ThreadAttrBuild(&threakattr,
        					(16 * LW_CFG_KB_SIZE),
        					LW_PRIO_LOW,
                            LW_OPTION_THREAD_STK_CHK,
                            psiochanChan);
        API_ThreadCreate("t_uart",
                         (PTHREAD_START_ROUTINE)debugSioDoRx,
                         &threakattr,
                         LW_NULL);                                   	/*  Create t_uart thread        */
        break;
        
    case SIO_HUP:                                                       /*  �رմ���                    */
        break;

    default:
        _ErrorHandle(ENOSYS);
	    return  (ENOSYS);
	}
	
	return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
