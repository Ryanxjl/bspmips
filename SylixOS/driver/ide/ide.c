/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: ide.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 10 �� 19 ��
**
** ��        ��: IDE Ӳ������Դ�ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __IDE_MEDIA             "/media/hd"

#define __IDE_CACHE_BURST       (32)
#define __IDE_CACHE_SIZE        (128 * LW_CFG_KB_SIZE)

#define __IDE_BYTE_PER_SECTOR   (512)

#define __IDE_TIMEOUT           (2 * LW_OPTION_WAIT_A_SECOND)
/*********************************************************************************************************
  ATAPI registers ����
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
  IDE ͨ�����Ͷ���
*********************************************************************************************************/
typedef struct __ide_chan {
    ATA_DRV_FUNCS  *pDrvFuncs;

    ATA_CALLBACK    pfuncAtaDevChk;
    PVOID           pAtaDevChkArg;

    ATA_CALLBACK    pfuncAtaWrite;
    PVOID           pAtaWriteArg;
} IDE_CHAN, *PIDE_CHAN;
/*********************************************************************************************************
** ��������: __ideIoctl
** ��������: ���� ATA ͨ��
** �䡡��  : patachan      ATA ͨ��
**           iCmd          ����
**           pvArg         ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT      __ideIoctl (ATA_CHAN  *patachan,
                            INT        iCmd,
                            PVOID      pvArg)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __ideIoOutByte
** ��������: ���һ���ֽ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
**           uiData        ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT      __ideIoOutByte (ATA_CHAN  *patachan,
                                ULONG      ulIoAddr,
                                UINT       uiData)
{
    write8(uiData, ulIoAddr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ideIoInByte
** ��������: ����һ���ֽ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
** �䡡��  : ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT     __ideIoInByte (ATA_CHAN  *patachan,
                              ULONG      ulIoAddr)
{
    return  (read8(ulIoAddr));
}
/*********************************************************************************************************
** ��������: __ideIoOutWordString
** ��������: ���һ���ִ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
**           psBuff        ���ݻ���
**           iWord         ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __ideIoInWordString
** ��������: ����һ���ִ�
** �䡡��  : patachan      ATA ͨ��
**           ulIoAddr      IO ��ַ
**           psBuff        ���ݻ���
**           iWord         ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __ideSysReset
** ��������: ��λ ATA ͨ��
** �䡡��  : patachan      ATA ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT    __ideSysReset (ATA_CHAN  *patachan,
                             INT        iDrive)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ideCallbackInstall
** ��������: ��װ�ص�����
** �䡡��  : patachan      ATA ͨ��
**           iCallbackType �ص�����
**           callback      �ص�����
**           pvCallbackArg �ص�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
  ATA ��������
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
** ��������: ideCreateAtaChan
** ��������: IDE Ӳ�̴���һ�� ATA ͨ��
** �䡡��  : NONE
** �䡡��  : ATA ͨ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: idaInit
** ��������: IDE Ӳ�̳�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  idaInit (VOID)
{
    ATA_CHAN       *patachan = ideCreateAtaChan();
    ATA_CHAN_PARAM  param;
    PLW_BLK_DEV     pblkdev;
    INT             iError;

    if (patachan) {
        param.ATACP_iCtrlNum                = 0;                        /*  ��������                    */
        param.ATACP_iDrives                 = 1;                        /*  �豸����                    */
        param.ATACP_iBytesPerSector         = __IDE_BYTE_PER_SECTOR;    /*  ÿ�����ֽ���                */
        param.ATACP_iConfigType             = ATA_PIO_MULTI |
                                              ATA_BITS_16;              /*  ���ñ�־                    */
        param.ATACP_bIntEnable              = LW_FALSE;                 /*  ϵͳ�ж�ʹ�ܱ�־            */
        param.ATACP_ulSyncSemTimeout        = __IDE_TIMEOUT;            /*  ͬ���ȴ���ʱʱ��(ϵͳʱ��)  */

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
