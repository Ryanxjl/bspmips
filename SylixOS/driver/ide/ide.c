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
** 文   件   名: ide.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 19 日
**
** 描        述: IDE 硬盘驱动源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __IDE_MEDIA             "/media/hd"

#define __IDE_CACHE_BURST       (32)
#define __IDE_CACHE_SIZE        (128 * LW_CFG_KB_SIZE)

#define __IDE_BYTE_PER_SECTOR   (512)

#define __IDE_TIMEOUT           (2 * LW_OPTION_WAIT_A_SECOND)
/*********************************************************************************************************
  ATAPI registers 定义
*********************************************************************************************************/
#define ATA_DATA(base0)         (base0 + 0)                             /* (RW) data register (16 bits) */
#define ATA_ERROR(base0)        (base0 + 1)                             /* (R)  error register          */
#define ATA_FEATURE(base0)      (base0 + 1)                             /* (W)  feature/precompensation */
#define ATA_SECCNT(base0)       (base0 + 2)                             /* (RW) sector count for ATA.
                                                                         * R-Interrupt reason W-unused  */
#define ATA_SECTOR(base0)       (base0 + 3)                             /* (RW) first sector number.
                                                                         * ATAPI- Reserved for SAMTAG   */
#define ATA_CYL_LO(base0)       (base0 + 4)                             /* (RW) cylinder low byte
                                                                         * ATAPI - Byte count Low       */
#define ATA_CYL_HI(base0)       (base0 + 5)                             /* (RW) cylinder high byte
                                                                         * ATAPI - Byte count High      */
#define ATA_SDH(base0)          (base0 + 6)                             /* (RW) sector size/drive/head
                                                                         * ATAPI - drive select         */
#define ATA_COMMAND(base0)      (base0 + 7)                             /* (W)  command register        */
#define ATA_STATUS(base0)       (base0 + 7)                             /* (R)  immediate status        */

#define ATA_A_STATUS(base1)     (base1 + 0)                             /* (R)  alternate status        */
#define ATA_D_CONTROL(base1)    (base1 + 0)                             /* (W)  disk controller control */
#define ATA_D_ADDRESS(base1)    (base1 + 1)                             /* (R)  disk controller address */
/*********************************************************************************************************
  IDE 通道类型定义
*********************************************************************************************************/
typedef struct __ide_chan {
    ATA_DRV_FUNCS  *pDrvFuncs;

    ATA_CALLBACK    pfuncAtaDevChk;
    PVOID           pAtaDevChkArg;

    ATA_CALLBACK    pfuncAtaWrite;
    PVOID           pAtaWriteArg;
} IDE_CHAN, *PIDE_CHAN;
/*********************************************************************************************************
** 函数名称: __ideIoctl
** 功能描述: 控制 ATA 通道
** 输　入  : patachan      ATA 通道
**           iCmd          命令
**           pvArg         参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT      __ideIoctl (ATA_CHAN  *patachan,
                            INT        iCmd,
                            PVOID      pvArg)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __ideIoOutByte
** 功能描述: 输出一个字节
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
**           uiData        数据
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT      __ideIoOutByte (ATA_CHAN  *patachan,
                                ULONG      ulIoAddr,
                                UINT       uiData)
{
    write8(uiData, ulIoAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ideIoInByte
** 功能描述: 输入一个字节
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
** 输　出  : 数据
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT     __ideIoInByte (ATA_CHAN  *patachan,
                              ULONG      ulIoAddr)
{
    return  (read8(ulIoAddr));
}
/*********************************************************************************************************
** 函数名称: __ideIoOutWordString
** 功能描述: 输出一个字串
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
**           psBuff        数据缓冲
**           iWord         字数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT    __ideIoOutWordString (ATA_CHAN  *patachan,
                                    ULONG      ulIoAddr,
                                    INT16     *psBuff,
                                    INT        iWord)
{
    writes16(ulIoAddr, psBuff, iWord);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ideIoInWordString
** 功能描述: 输入一个字串
** 输　入  : patachan      ATA 通道
**           ulIoAddr      IO 地址
**           psBuff        数据缓冲
**           iWord         字数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT    __ideIoInWordString (ATA_CHAN  *patachan,
                                   ULONG      ulIoAddr,
                                   INT16     *psBuff,
                                   INT        iWord)
{
    reads16(ulIoAddr, psBuff, iWord);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ideSysReset
** 功能描述: 复位 ATA 通道
** 输　入  : patachan      ATA 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT    __ideSysReset (ATA_CHAN  *patachan,
                             INT        iDrive)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ideCallbackInstall
** 功能描述: 安装回调函数
** 输　入  : patachan      ATA 通道
**           iCallbackType 回调类型
**           callback      回调函数
**           pvCallbackArg 回调参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT   __ideCallbackInstall (ATA_CHAN     *patachan,
                                   INT           iCallbackType,
                                   ATA_CALLBACK  callback,
                                   PVOID         pvCallbackArg)
{
    PIDE_CHAN       pidechan = (PIDE_CHAN)patachan;

    switch (iCallbackType) {
    case ATA_CALLBACK_CHECK_DEV:
        pidechan->pfuncAtaDevChk = callback;
        pidechan->pAtaDevChkArg  = pvCallbackArg;
        break;


    case ATA_CALLBACK_WRITE_DATA:
        pidechan->pfuncAtaWrite = callback;
        pidechan->pAtaWriteArg  = pvCallbackArg;
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ATA 驱动函数
*********************************************************************************************************/
static ATA_DRV_FUNCS  _G_ideDrvFuncs = {
        __ideIoctl,
        __ideIoOutByte,
        __ideIoInByte,
        __ideIoOutWordString,
        __ideIoInWordString,
        __ideSysReset,
        __ideCallbackInstall,
};
/*********************************************************************************************************
** 函数名称: ideCreateAtaChan
** 功能描述: IDE 硬盘创建一个 ATA 通道
** 输　入  : NONE
** 输　出  : ATA 通道
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ATA_CHAN  *ideCreateAtaChan(VOID)
{
    PIDE_CHAN  pidechan = __SHEAP_ALLOC(sizeof(IDE_CHAN));

    if (pidechan) {
        pidechan->pDrvFuncs = &_G_ideDrvFuncs;
    }
    return  ((ATA_CHAN  *)pidechan);
}
/*********************************************************************************************************
** 函数名称: idaInit
** 功能描述: IDE 硬盘初始化
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  idaInit (VOID)
{
    ATA_CHAN       *patachan = ideCreateAtaChan();
    ATA_CHAN_PARAM  param;
    PLW_BLK_DEV     pblkdev;
    INT             iError;

    if (patachan) {
        param.ATACP_iCtrlNum                = 0;                        /*  控制器号                    */
        param.ATACP_iDrives                 = 1;                        /*  设备个数                    */
        param.ATACP_iBytesPerSector         = __IDE_BYTE_PER_SECTOR;    /*  每扇区字节数                */
        param.ATACP_iConfigType             = ATA_PIO_MULTI |
                                              ATA_BITS_16;              /*  配置标志                    */
        param.ATACP_bIntEnable              = LW_FALSE;                 /*  系统中断使能标志            */
        param.ATACP_ulSyncSemTimeout        = __IDE_TIMEOUT;            /*  同步等待超时时间(系统时钟)  */

        param.ATACP_atareg.ATAREG_ulData    = ATA_DATA(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulError   = ATA_ERROR(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulFeature = ATA_FEATURE(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulSeccnt  = ATA_SECCNT(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulSector  = ATA_SECTOR(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulCylLo   = ATA_CYL_LO(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulCylHi   = ATA_CYL_HI(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulSdh     = ATA_SDH(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulCommand = ATA_COMMAND(BSP_CFG_IDE_BASE0);
        param.ATACP_atareg.ATAREG_ulStatus  = ATA_STATUS(BSP_CFG_IDE_BASE0);

        param.ATACP_atareg.ATAREG_ulAStatus = ATA_A_STATUS(BSP_CFG_IDE_BASE1);
        param.ATACP_atareg.ATAREG_ulDControl= ATA_D_CONTROL(BSP_CFG_IDE_BASE1);

        iError = API_AtaDrv(patachan, &param);
        if (iError == ERROR_NONE) {

            pblkdev = API_AtaDevCreate(param.ATACP_iCtrlNum,
                                       0,
                                       0,
                                       0);
            if (pblkdev) {
                API_OemDiskMount(__IDE_MEDIA,
                                 pblkdev,
                                 LW_NULL,
                                 __IDE_CACHE_SIZE,
                                 __IDE_CACHE_BURST);

                return  (ERROR_NONE);
            }
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
