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
** ��   ��   ��: Ryan.xin (�Ž���)
**
** �ļ���������: 2015 �� 09 �� 09 ��
**
** ��        ��: ����������.
*********************************************************************************************************/
#ifndef __CONFIG_H
#define __CONFIG_H
/*********************************************************************************************************
  ��ַ�ռ�����
*********************************************************************************************************/
#define BSP_CFG_ROM_BASE            0xBFC00000
#define BSP_CFG_ROM_PA_BASE         0x1FC00000

#define BSP_CFG_ROM_SIZE            (12 * 1024 * 1024)

#define BSP_CFG_RAM_BASE            (0x80000000)
#define BSP_CFG_RAM_PA_BASE         (0x00000000)

#define BSP_CFG_RAM_SIZE            (256 * 1024 * 1024)

#define BSP_CFG_KERNEL_SIZE         (6 * 1024 * 1024)

#define BSP_CFG_DATA_BASE           (BSP_CFG_RAM_SIZE    + BSP_CFG_KERNEL_SIZE)
#define BSP_CFG_DATA_PA_BASE        (BSP_CFG_RAM_PA_BASE + BSP_CFG_KERNEL_SIZE)

#define BSP_CFG_COMMON_MEM_SIZE     (44 * 1024 * 1024)

#define BSP_CFG_SYS_INIT_SP_ADDR    (BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE)
/*********************************************************************************************************
  ISA
*********************************************************************************************************/
#define BSP_CFG_ISA_IO_BASE         0xB4000000
#define BSP_CFG_ISA_IO_PA_BASE      0x14000000
#define BSP_CFG_ISA_IO_SIZE         0x00010000

#define BSP_CFG_ISA_MEM_BASE        0xB0000000
#define BSP_CFG_ISA_MEM_PA_BASE     0x10000000
#define BSP_CFG_ISA_MEM_SIZE        0x01000000
/*********************************************************************************************************
  8259A PIC
*********************************************************************************************************/
#define BSP_CFG_8259A_VECTOR        (2)

#define BSP_CFG_8259A_VECTOR_BASE   (8)
#define BSP_CFG_8259A_SUB_VECTOR_NR (16)
#define BSP_CFG_8259A_SUB_VECTOR(x) (BSP_CFG_8259A_VECTOR_BASE + (x))
/*********************************************************************************************************
  16C550 UART
*********************************************************************************************************/
#define BSP_CFG_16C550_SIO_NR       1
#define BSP_CFG_16C550_OFFSET       (0x3F8)
#define BSP_CFG_16C550_BASE         (BSP_CFG_ISA_IO_BASE    + BSP_CFG_16C550_OFFSET)
#define BSP_CFG_16C550_PA_BASE      (BSP_CFG_ISA_IO_PA_BASE + BSP_CFG_16C550_OFFSET)
#define BSP_CFG_16C550_BAUD         (115200)
#define BSP_CFG_16C550_VECTOR       BSP_CFG_8259A_SUB_VECTOR(4)

#endif                                                                  /*  __CONFIG_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
