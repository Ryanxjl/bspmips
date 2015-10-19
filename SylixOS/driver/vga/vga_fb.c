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
** 文   件   名: vga_fb.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 18 日
**
** 描        述: VGA 驱动 FrameBuffer 驱动源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "string.h"
/*********************************************************************************************************
  VGA 控制器类型定义
*********************************************************************************************************/
typedef struct {
    LW_GM_DEVICE                VGAC_gmDev;                             /*  图形设备                    */
    CPCHAR                      VGAC_pcName;                            /*  设备名                      */
} __VGA_CONTROLER, *__PVGA_CONTROLER;
/*********************************************************************************************************
  FrameBuffer 相关定义
*********************************************************************************************************/
#define __VGA_XSIZE             320
#define __VGA_YSIZE             240
#define __VGA_BITS_PER_PIXEL    8
#define __VGA_BYTES_PER_PIXEL   (__VGA_BITS_PER_PIXEL / 8)

#define __VGA_FRAMEBUFFER_BASE  (BSP_CFG_ISA_MEM_PA_BASE + 0xA0000)
#define __VGA_FRAMEBUFFER_SIZE  (__VGA_XSIZE * __VGA_YSIZE * __VGA_BYTES_PER_PIXEL)
/*********************************************************************************************************
  调色板相关寄存器定义
*********************************************************************************************************/
#define __VGA_PAL_MASK_REG      (BSP_CFG_ISA_IO_BASE + 0x3C6)           /*  bit mask register           */
#define __VGA_PAL_READ_REG      (BSP_CFG_ISA_IO_BASE + 0x3C7)           /*  read index                  */
#define __VGA_PAL_WRITE_REG     (BSP_CFG_ISA_IO_BASE + 0x3C8)           /*  write index                 */
#define __VGA_PAL_DATA_REG      (BSP_CFG_ISA_IO_BASE + 0x3C9)           /*  send/receive data here      */
/*********************************************************************************************************
  VGA 相关寄存器定义
*********************************************************************************************************/
#define __VGA_MISC_OUTPUT_REG   (BSP_CFG_ISA_IO_BASE + 0x3C2)
#define __VGA_DATA_REG          (BSP_CFG_ISA_IO_BASE + 0x3C0)
#define __VGA_ADDRESS_REG       (BSP_CFG_ISA_IO_BASE + 0x3C0)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_GM_FILEOPERATIONS     _G_vgaGmFileOper;
static __VGA_CONTROLER          _G_vgaControler;

const UINT8  _G_ucVgaMode13[][32] = {
        { 0x03, 0x01, 0x0F, 0x00, 0x0E },                               /*  0x3C4, index 0-4            */

        { 0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
          0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
          0xFF },                                                       /*  0x3D4, index 0-0x18         */

        { 0, 0, 0, 0, 0, 0x40, 0x05, 0x0F, 0xFF },                      /*  0x3CE, index 0-8            */

        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
          0x41, 0, 0x0F, 0, 0 }                                         /*  0x3C0, index 0-0x14         */
};
/*********************************************************************************************************
  驱动函数
*********************************************************************************************************/
static INT      __vgaOpen(PLW_GM_DEVICE  pGmDev, INT  iFlag, INT  iMode);
static INT      __vgaClose(PLW_GM_DEVICE  pGmDev);
static INT      __vgaGetVarInfo(PLW_GM_DEVICE  pGmDev, PLW_GM_VARINFO  pGmVi);
static INT      __vgaGetScrInfo(PLW_GM_DEVICE  pGmDev, PLW_GM_SCRINFO  pGmSi);
/*********************************************************************************************************
** 函数名称: __vgaOpen
** 功能描述: 打开 VGA 设备
** 输　入  : pGmDev            图形设备
**           iFlag             标志
**           iMode             模式
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __vgaOpen (PLW_GM_DEVICE  pGmDev, INT  iFlag, INT  iMode)
{
    INT  i;

    write8(0x63, __VGA_MISC_OUTPUT_REG);
    write8(0x00, BSP_CFG_ISA_IO_BASE + 0x3DA);

    for (i = 0; i < 5; i++) {
        write8(i, BSP_CFG_ISA_IO_BASE + 0x3C4);
        write8(_G_ucVgaMode13[0][i], BSP_CFG_ISA_IO_BASE + 0x3C4 + 1);
    }

    write16(0x0E11, BSP_CFG_ISA_IO_BASE + 0x3D4);

    for (i = 0; i < 0x19; i++) {
        write8(i, BSP_CFG_ISA_IO_BASE + 0x3D4);
        write8(_G_ucVgaMode13[1][i], BSP_CFG_ISA_IO_BASE + 0x3D4 + 1);
    }

    for (i = 0; i < 0x9; i++) {
        write8(i, BSP_CFG_ISA_IO_BASE + 0x3CE);
        write8(_G_ucVgaMode13[2][i], BSP_CFG_ISA_IO_BASE + 0x3CE + 1);
    }

    read8(BSP_CFG_ISA_IO_BASE + 0x3DA);

    for (i = 0; i < 0x15; i++) {
        read16(__VGA_DATA_REG);
        write8(i, __VGA_ADDRESS_REG);
        write8(_G_ucVgaMode13[3][i], __VGA_DATA_REG);
    }

    write8(0x20, BSP_CFG_ISA_IO_BASE + 0x3C0);

    for (i = 0; i < 256; i++) {
        write8(0xFF,                __VGA_PAL_MASK_REG);
        write8(i,                   __VGA_PAL_WRITE_REG);
        write8((i & 7) * 32,        __VGA_PAL_DATA_REG);                /*  RED                         */
        write8(((i >> 3) & 7) * 32, __VGA_PAL_DATA_REG);                /*  GREEN                       */
        write8(((i >> 6) & 3) * 64, __VGA_PAL_DATA_REG);                /*  BLUE                        */
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vgaClose
** 功能描述: 关闭 VGA 设备
** 输　入  : pGmDev            图形设备
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT      __vgaClose (PLW_GM_DEVICE  pGmDev)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vgaGetVarInfo
** 功能描述: 获得 VGA 设备 LW_GM_VARINFO 参数
** 输　入  : pGmDev            图形设备
**           pGmVi             LW_GM_VARINFO 参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT      __vgaGetVarInfo (PLW_GM_DEVICE  pGmDev, PLW_GM_VARINFO  pGmVi)
{
    if (pGmVi) {
        pGmVi->GMVI_ulXRes              = __VGA_XSIZE;
        pGmVi->GMVI_ulYRes              = __VGA_YSIZE;

        pGmVi->GMVI_ulXResVirtual       = __VGA_XSIZE;
        pGmVi->GMVI_ulYResVirtual       = __VGA_YSIZE;

        pGmVi->GMVI_ulXOffset           = 0;
        pGmVi->GMVI_ulYOffset           = 0;

        pGmVi->GMVI_ulBitsPerPixel      = __VGA_BITS_PER_PIXEL;
        pGmVi->GMVI_ulBytesPerPixel     = __VGA_BYTES_PER_PIXEL;

        pGmVi->GMVI_ulGrayscale         = 1 << __VGA_BITS_PER_PIXEL;
        pGmVi->GMVI_ulRedMask           = 0x7 << (0);
        pGmVi->GMVI_ulGreenMask         = 0x7 << (3);
        pGmVi->GMVI_ulBlueMask          = 0x3 << (3 + 3);
        pGmVi->GMVI_ulTransMask         = 0;

        pGmVi->GMVI_bHardwareAccelerate = LW_FALSE;
        pGmVi->GMVI_ulMode              = LW_GM_SET_MODE;
        pGmVi->GMVI_ulStatus            = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vgaGetScrInfo
** 功能描述: 获得 VGA 设备 PLW_GM_SCRINFO 参数
** 输　入  : pGmDev            图形设备
**           pGmSi             PLW_GM_SCRINFO 参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT      __vgaGetScrInfo (PLW_GM_DEVICE  pGmDev, PLW_GM_SCRINFO  pGmSi)
{
    __PVGA_CONTROLER    pControler = &_G_vgaControler;

    if (pGmSi) {
        pGmSi->GMSI_pcName              = (PCHAR)pControler->VGAC_pcName;
        pGmSi->GMSI_ulId                = 0;
        pGmSi->GMSI_stMemSize           = __VGA_FRAMEBUFFER_SIZE;
        pGmSi->GMSI_stMemSizePerLine    = __VGA_XSIZE * __VGA_BYTES_PER_PIXEL;
        pGmSi->GMSI_pcMem               = (caddr_t)__VGA_FRAMEBUFFER_BASE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vgaSetPalette
** 功能描述: 设置调色板
** 输　入  : pGmDev            图形设备
**           uiStart           开始位置
**           uiLen             长度
**           pulRed            红色数据数组
**           pulGreen          蓝色数据数组
**           pulBlue           绿色数据数组
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT      __vgaSetPalette (PLW_GM_DEVICE  pGmDev,
                                 UINT           uiStart,
                                 UINT           uiLen,
                                 ULONG         *pulRed,
                                 ULONG         *pulGreen,
                                 ULONG         *pulBlue)
{
    INT  i;

    for (i = 0; i < uiLen; i++) {
        write8(0xFF,        __VGA_PAL_MASK_REG);
        write8(uiStart + i, __VGA_PAL_WRITE_REG);
        write8(pulRed[i],   __VGA_PAL_DATA_REG);                        /*  RED                         */
        write8(pulGreen[i], __VGA_PAL_DATA_REG);                        /*  GREEN                       */
        write8(pulBlue[i],  __VGA_PAL_DATA_REG);                        /*  BLUE                        */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __vgaGetPalette
** 功能描述: 获得调色板
** 输　入  : pGmDev            图形设备
**           uiStart           开始位置
**           uiLen             长度
**           pulRed            红色数据数组
**           pulGreen          蓝色数据数组
**           pulBlue           绿色数据数组
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT      __vgaGetPalette (PLW_GM_DEVICE  pGmDev,
                                 UINT           uiStart,
                                 UINT           uiLen,
                                 ULONG         *pulRed,
                                 ULONG         *pulGreen,
                                 ULONG         *pulBlue)
{
    INT  i;

    for (i = 0; i < uiLen; i++) {
        write8(0xFF,        __VGA_PAL_MASK_REG);
        write8(uiStart + i, __VGA_PAL_WRITE_REG);
        pulRed[i]   = read8(__VGA_PAL_DATA_REG);                        /*  RED                         */
        pulGreen[i] = read8(__VGA_PAL_DATA_REG);                        /*  GREEN                       */
        pulBlue[i]  = read8(__VGA_PAL_DATA_REG);                        /*  BLUE                        */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vgaFbDevCreate
** 功能描述: 创建 VGA FrameBuffer 设备
** 输　入  : cpcName           设备名
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vgaFbDevCreate (CPCHAR  cpcName)
{
    __PVGA_CONTROLER        pControler  = &_G_vgaControler;
    PLW_GM_FILEOPERATIONS   pGmFileOper = &_G_vgaGmFileOper;

    if (cpcName == LW_NULL) {
        return  (PX_ERROR);
    }

    /*
     *  仅支持 framebuffer 模式.
     */
    pGmFileOper->GMFO_pfuncOpen         = __vgaOpen;
    pGmFileOper->GMFO_pfuncClose        = __vgaClose;
    pGmFileOper->GMFO_pfuncGetVarInfo   = (INT (*)(LONG, PLW_GM_VARINFO))__vgaGetVarInfo;
    pGmFileOper->GMFO_pfuncGetScrInfo   = (INT (*)(LONG, PLW_GM_SCRINFO))__vgaGetScrInfo;
    pGmFileOper->GMFO_pfuncSetPalette   = (INT (*)(LONG, UINT, UINT, ULONG *, ULONG *, ULONG *))
                                          __vgaSetPalette;
    pGmFileOper->GMFO_pfuncGetPalette   = (INT (*)(LONG, UINT, UINT, ULONG *, ULONG *, ULONG *))
                                          __vgaGetPalette;

    pControler->VGAC_gmDev.GMDEV_gmfileop   = pGmFileOper;
    pControler->VGAC_gmDev.GMDEV_ulMapFlags = LW_VMM_FLAG_DMA | LW_VMM_FLAG_BUFFERABLE;
    pControler->VGAC_pcName                 = cpcName;

    return  (gmemDevAdd(pControler->VGAC_pcName, &pControler->VGAC_gmDev));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
