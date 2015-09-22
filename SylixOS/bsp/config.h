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
** ��   ��   ��: config.h
**
** ��   ��   ��: Ryan.xin (����)
**
** �ļ���������: 2015 �� 09 �� 09 ��
**
** ��        ��: ����������.
*********************************************************************************************************/
#ifndef __CONFIG_H
#define __CONFIG_H

#define BSP_CFG_ROM_BASE            0xbfc00000
#define BSP_CFG_ROM_SIZE            (12 * 1024 * 1024)

#define BSP_CFG_RAM_BASE            (0x80000000)
#define BSP_CFG_RAM_SIZE            (128 * 1024 * 1024)

#define BSP_CFG_DATA_BASE           (BSP_CFG_RAM_SIZE + 6 * 1024 * 1024)

#define BSP_CFG_COMMON_MEM_SIZE     (44 * 1024 * 1024)

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define BSP_CFG_SYS_MIPS_CACHE_MODE  CONF_CM_CACHABLE_NONCOHERENT


#define BSP_CFG_SYS_INIT_SP_ADDR     (BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE)

#define MIPS_RELOC                  3
#define STATUS_SET                  0

#define CONF_CM_UNCACHED            2


/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define BSP_CFG_SYS_DCACHE_SIZE      16384
#define BSP_CFG_SYS_ICACHE_SIZE      16384
#define BSP_CFG_SYS_CACHELINE_SIZE   32

#define BSP_CFG_INDEX_STORE_TAG_I    0x08
#define BSP_CFG_INDEX_STORE_TAG_D    0x09

#define BSP_CFG_INDEX_BASE           A_K0BASE

#endif                                                                  /*  __CONFIG_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
