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
** 文   件   名: sio.c
**
** 创   建   人: Jiao.JinXing(焦进星)
**
** 文件创建日期: 2015 年 08 月 14 日
**
** 描        述: 串口 SIO 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sio.h"
/*********************************************************************************************************
  UART 相关配置
*********************************************************************************************************/
#define BSP_CFG_UART_NR                     1
#define BSP_CFG_UART_DEFAULT_BAUD           SIO_BAUD_115200
#define BSP_CFG_UART_DEFAULT_OPT            (CREAD | CS8)
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID  bspDebugPutc(CHAR  cChar);
extern CHAR  bspDebugGetc(VOID);
/*********************************************************************************************************
** 函数名称: debugUartChannelVaild
** 功能描述: 判断 UART 通道号是否有效
** 输　入  : uiChannel                     硬件通道号
** 输　出  : LW_FALSE OR LW_TRUE
** 全局变量:
** 调用模块:
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

    SIO_DRV_FUNCS              *pdrvFuncs;                              /*  SIO 驱动程序组              */

    INT                       (*pcbGetTxChar)();                        /*  中断回调函数                */
    INT                       (*pcbPutRcvChar)();

    PVOID                       pvGetTxArg;                             /*  回调函数参数                */
    PVOID                       pvPutRcvArg;

    INT                         iChannelMode;                           /*  通道模式                    */

    UCHAR                     (*pfuncHwInByte)(INT);                    /*  物理硬件接收一个字节        */
    VOID                      (*pfuncHwOutByte)(INT, CHAR);             /*  物理硬件发送一个字节        */

    INT                         iBaud;                                  /*  波特率                      */

    INT                         iHwOption;                              /*  硬件选项                    */

} __SIO_CHANNEL;
typedef __SIO_CHANNEL          *__PSIO_CHANNEL;                         /*  指针类型                    */
/*********************************************************************************************************
  SIO 通道控制块
*********************************************************************************************************/
static __SIO_CHANNEL            __G_sioChannels[BSP_CFG_UART_NR];
/*********************************************************************************************************
  SIO 驱动程序
*********************************************************************************************************/
static INT   debugSioIoctl(SIO_CHAN  *psiochanChan,
                        INT        iCmd,
                        LONG       lArg);                             	/*  端口控制                    */
static INT   debugSioStartup(SIO_CHAN  *psiochanChan);                   	/*  发送                        */
static INT   debugSioCbInstall(SIO_CHAN        *psiochanChan,
                            INT              iCallbackType,
                            VX_SIO_CALLBACK  callbackRoute,
                            PVOID            pvCallbackArg);          	/*  安装回调                    */
static INT   debugSioPollRxChar(SIO_CHAN    *psiochanChan,
                             PCHAR        pcInChar);                  	/*  轮询接收                    */
static INT   debugSioPollTxChar(SIO_CHAN    *psiochanChan,
                             CHAR         cOutChar);                  	/*  轮询发送                    */
/*********************************************************************************************************
  SIO 驱动程序
*********************************************************************************************************/
static SIO_DRV_FUNCS    __G_sioDrvFuncs = {
             (INT (*)(SIO_CHAN *,INT, PVOID))debugSioIoctl,
             debugSioStartup,
             debugSioCbInstall,
             debugSioPollRxChar,
             debugSioPollTxChar
};
/*********************************************************************************************************
** 函数名称: sioChanCreatePoll
** 功能描述: 创建一个 SIO 通道
** 输　入  : uiChannel                 硬件通道号
** 输　出  : SIO 通道
** 全局变量:
** 调用模块:
*********************************************************************************************************/
SIO_CHAN    *sioChanCreatePoll (UINT   uiChannel)
{
    __PSIO_CHANNEL          psiochanUart;
    
    if (!debugUartChannelVaild(uiChannel)) {
        return  (LW_NULL);                                              /*  通道号错误                  */
    }

    psiochanUart               = &__G_sioChannels[uiChannel];
    psiochanUart->pdrvFuncs    = &__G_sioDrvFuncs;                      /*  SIO FUNC                    */
    psiochanUart->iChannelMode = SIO_MODE_POLL;                         /*  使用 POLL 模式              */
    psiochanUart->iBaud        = BSP_CFG_UART_DEFAULT_BAUD;             /*  初始化波特率                */
    psiochanUart->iHwOption    = BSP_CFG_UART_DEFAULT_OPT;              /*  初始化硬件状态              */

    return  ((SIO_CHAN *)psiochanUart);
}
/*********************************************************************************************************
** 函数名称: debugSioCbInstall
** 功能描述: SIO 通道安装回调函数
** 输　入  : psiochanChan                 SIO 通道
**           iCallbackType                回调类型
**           callbackRoute                回调函数
**           pvCallbackArg                回调参数
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  debugSioCbInstall (SIO_CHAN        *psiochanChan,
                            INT              iCallbackType,
                            VX_SIO_CALLBACK  callbackRoute,
                            PVOID            pvCallbackArg)
{
    __PSIO_CHANNEL          psiochanUart = (__PSIO_CHANNEL)psiochanChan;
    
    switch (iCallbackType) {
    
    case SIO_CALLBACK_GET_TX_CHAR:                                      /*  发送回电函数                */
        psiochanUart->pcbGetTxChar = callbackRoute;
        psiochanUart->pvGetTxArg   = pvCallbackArg;
        return  (ERROR_NONE);
        
    case SIO_CALLBACK_PUT_RCV_CHAR:                                     /*  接收回电函数                */
        psiochanUart->pcbPutRcvChar = callbackRoute;
        psiochanUart->pvPutRcvArg   = pvCallbackArg;
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: debugSioPollRxChar
** 功能描述: SIO 通道轮询接收
** 输　入  : psiochanChan                 SIO 通道
**           pcInChar                     接收的字节
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  debugSioPollRxChar (SIO_CHAN  *psiochanChan, PCHAR  pcInChar)
{
    *pcInChar = bspDebugGetc();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: debugSioDoRx
** 功能描述: SIO 通道轮询接收
** 输　入  : psiochanChan                 SIO 通道
** 输　出  : LW_NULL
** 全局变量:
** 调用模块:
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
** 函数名称: debugSioPollTxChar
** 功能描述: SIO 通道轮询发送
** 输　入  : psiochanChan                 SIO 通道
**           cOutChar                     发送的字节
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  debugSioPollTxChar (SIO_CHAN  *psiochanChan, CHAR  cOutChar)
{
	bspDebugPutc(cOutChar);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: debugSioStartup
** 功能描述: SIO 通道发送(没有使用中断)
** 输　入  : psiochanChan                 SIO 通道
** 输　出  : 错误号
** 全局变量:
** 调用模块:
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
** 函数名称: debugSioIoctl
** 功能描述: SIO 通道控制
** 输　入  : psiochanChan                 SIO 通道
**           iCmd                         命令
**           lArg                         参数
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  debugSioIoctl (SIO_CHAN  *psiochanChan, INT  iCmd, LONG  lArg)
{
    __PSIO_CHANNEL          psiochanUart = (__PSIO_CHANNEL)psiochanChan;
    LW_CLASS_THREADATTR     threakattr;
    
    switch (iCmd) {
    
    case SIO_BAUD_SET:                                                  /*  设置波特率                  */
        psiochanUart->iBaud = (INT)lArg;                                /*  记录波特率                  */
        break;
        
    case SIO_BAUD_GET:                                                  /*  获得波特率                  */
        *((LONG *)lArg) = psiochanUart->iBaud;
        break;
    
    case SIO_HW_OPTS_SET:                                               /*  设置硬件参数                */
        psiochanUart->iHwOption = (INT)lArg;                            /*  记录硬件参数                */
        break;
        
    case SIO_HW_OPTS_GET:                                               /*  获取硬件参数                */
        *((LONG *)lArg) = psiochanUart->iHwOption;
        break;
    
    case SIO_OPEN:                                                      /*  打开串口                    */
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
        
    case SIO_HUP:                                                       /*  关闭串口                    */
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
