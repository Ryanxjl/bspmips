/**********************************************************************************************************
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
** ��   ��   ��: bspMap.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 23 ��
**
** ��        ��: C ������ڲ���. �����ҳ�ռ���ȫ��ӳ���ϵ����.
*********************************************************************************************************/

#ifndef __BSPMAP_H
#define __BSPMAP_H

/*********************************************************************************************************
   �ڴ�����ϵͼ

    +-----------------------+--------------------------------+
    |       ͨ���ڴ���      |          VMM ������            |
    |         CACHE         |                                |
    +-----------------------+--------------------------------+

*********************************************************************************************************/

/*********************************************************************************************************
  physical memory zone
*********************************************************************************************************/
#ifdef  __BSPINIT_MAIN_FILE

LW_VMM_ZONE_DESC    _G_zonedescGlobal[] = {
    {
            BSP_CFG_RAM_PA_BASE + BSP_CFG_COMMON_MEM_SIZE,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  ���ɱ� DMA ʹ��             */
    },

    {
            BSP_CFG_RAM_PA_BASE + BSP_CFG_COMMON_MEM_SIZE +
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  ���ɱ� DMA ʹ��             */
    },
};
/*********************************************************************************************************
  ��ʼ���ڴ������벼�� (VMM ���������ڴ治��Ҫ��ȫ�ֳ�ʼ�����г�ʼ��)
*********************************************************************************************************/

LW_MMU_GLOBAL_DESC  _G_globaldescMap[] = {
#ifdef __BOOT_INRAM
    {
            BSP_CFG_RAM_BASE,
            BSP_CFG_RAM_PA_BASE,
            BSP_CFG_KERNEL_SIZE,                                        /*  SylixOS Kernel Text         */
            (LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR)                       /*  �˶ο��޸�, �������        */
    },
#else
    {
            BSP_CFG_ROM_BASE,
            BSP_CFG_ROM_PA_BASE,
            BSP_CFG_KERNEL_SIZE,                                        /*  SylixOS Kernel Text         */
            LW_VMM_FLAG_EXEC
    },
#endif

    {
            BSP_CFG_DATA_BASE,
            BSP_CFG_DATA_PA_BASE,
            BSP_CFG_COMMON_MEM_SIZE,
            LW_VMM_FLAG_RDWR
    },

    {
            BSP_CFG_ISA_IO_BASE,
            BSP_CFG_ISA_IO_PA_BASE,
            BSP_CFG_ISA_IO_SIZE,
            LW_VMM_FLAG_DMA
    },

    {
            BSP_CFG_ISA_MEM_BASE,
            BSP_CFG_ISA_MEM_PA_BASE,
            BSP_CFG_ISA_MEM_SIZE,
            LW_VMM_FLAG_DMA
    },

    {
            0,
            0,
            0,
            0
    }
};

#endif                                                                  /*  __BSPINIT_MAIN_FILE         */
#endif                                                                  /*  __BSPMAP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
