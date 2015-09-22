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
            BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  ���ɱ� DMA ʹ��             */
    },

    {
            BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE + (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            (BSP_CFG_RAM_SIZE - BSP_CFG_COMMON_MEM_SIZE) / 2,
            LW_ZONE_ATTR_DMA                                            /*  ���ɱ� DMA ʹ��             */
    },
};
/*********************************************************************************************************
  ��ʼ���ڴ������벼�� (VMM ���������ڴ治��Ҫ��ȫ�ֳ�ʼ�����г�ʼ��)
*********************************************************************************************************/

LW_MMU_GLOBAL_DESC  _G_globaldescMap[] = {

#ifdef __BOOT_INRAM
    /*
     *  ARM except table
     */
    {                                                                   /*  �ж�������                  */
        BSP_CFG_ROM_BASE,                                               /*  ARM except table            */
        BSP_CFG_RAM_BASE,                                               /*  RAM except table            */
        LW_CFG_VMM_PAGE_SIZE,                                           /*  one page size               */
        (LW_VMM_FLAG_EXEC)                                              /*  ���ֳ�����                  */
    },
    /*
     *  nor-flash area
     */
    {
        (BSP_CFG_ROM_BASE + LW_CFG_VMM_PAGE_SIZE),                      /*  flash ʣ�µĿռ�ֱ��ӳ��    */
        (BSP_CFG_ROM_BASE + LW_CFG_VMM_PAGE_SIZE),
        (BSP_CFG_ROM_SIZE - LW_CFG_VMM_PAGE_SIZE),
        (LW_VMM_FLAG_EXEC)                                              /*  ���ֳ�����                  */
    },
#else                                                                   /*  __DEBUG_IN_FLASH            */
    /*
     *  nor-flash area
     */
    {
        (BSP_CFG_ROM_BASE),                                             /*  flash ֱ��ӳ��              */
        (BSP_CFG_ROM_BASE),
        (BSP_CFG_ROM_SIZE),
        (LW_VMM_FLAG_EXEC)                                              /*  ���ֳ�����                  */
    },
#endif                                                                  /*  __DEBUG_IN_RAM              */
    
    /*
     *  kernel space
     */
    {
        (BSP_CFG_RAM_BASE),                                             /*  SylixOS Kernel Text         */
        (BSP_CFG_RAM_BASE),
        (BSP_CFG_DATA_BASE - BSP_CFG_RAM_BASE),
        (LW_VMM_FLAG_EXEC | LW_VMM_FLAG_RDWR)                           /*  �˶ο��޸�, �������        */
    },
    
    {                                                                   /*  SylixOS Kernel Memory       */
        (BSP_CFG_DATA_BASE),
        (BSP_CFG_DATA_BASE),
        (BSP_CFG_COMMON_MEM_SIZE - (BSP_CFG_DATA_BASE - BSP_CFG_RAM_BASE)),
        (LW_VMM_FLAG_RDWR)                                              /*  ״̬���� CB                 */
    },
    
    /*
     *  TODO: ����Ӳ�����⹦�ܼĴ�����ӳ��, �ο���������:
     */
#if 0                                                                   /*  �ο����뿪ʼ                */
    {
        0x48000000,                                                     /*  ���⹦�ܼĴ����������ַ    */
        0x48000000,                                                     /*  ӳ�䵽�������ַ            */
        LW_CFG_VMM_PAGE_SIZE,                                           /*  ӳ��Ĵ�С                  */
        (LW_VMM_FLAG_DMA)                                               /*  ӳ�������                  */
    },
#endif                                                                  /*  �ο��������                */
    
    {                                                                   /*  ����                        */
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
