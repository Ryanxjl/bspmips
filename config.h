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
** 文   件   名: config.h
**
** 创   建   人: RealEvo-IDE
**
** 文件创建日期: 2015 年11 月30 日
**
** 描        述: 本文由 RealEvo-IDE 生成，用于配置 Makefile 功能，请勿手动修改
*********************************************************************************************************/
#ifndef __CONFIG_H
#define __CONFIG_H
/*********************************************************************************************************
  地址空间配置
*********************************************************************************************************/
#define MIPS32_KSEG0_PA(va)             ((va) & (~(0x1 << 31)))
#define MIPS32_KSEG1_PA(va)             ((va) & (~(0x7 << 29)))

#define BSP_CFG_ROM_BASE                0xBFC00000
#define BSP_CFG_ROM_PA_BASE             MIPS32_KSEG1_PA(BSP_CFG_ROM_BASE)
#define BSP_CFG_ROM_SIZE                (12 * 1024 * 1024)

#define BSP_CFG_RAM_BASE                0x80000000
#define BSP_CFG_RAM_PA_BASE             MIPS32_KSEG0_PA(BSP_CFG_RAM_BASE)
#define BSP_CFG_RAM_SIZE                (256 * 1024 * 1024)

#define BSP_CFG_KERNEL_SIZE             (6 * 1024 * 1024)

#define BSP_CFG_DATA_BASE               (BSP_CFG_RAM_BASE + BSP_CFG_KERNEL_SIZE)
#define BSP_CFG_DATA_PA_BASE            MIPS32_KSEG0_PA(BSP_CFG_DATA_BASE)

#define BSP_CFG_COMMON_MEM_SIZE         (44 * 1024 * 1024)

#define BSP_CFG_SYS_INIT_SP_ADDR        (BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE)
/*********************************************************************************************************
  ISA
*********************************************************************************************************/
#define BSP_CFG_ISA_IO_BASE             0xB4000000
#define BSP_CFG_ISA_IO_PA_BASE          MIPS32_KSEG1_PA(BSP_CFG_ISA_IO_BASE)
#define BSP_CFG_ISA_IO_SIZE             0x00010000

#define BSP_CFG_ISA_MEM_BASE            0xB0000000
#define BSP_CFG_ISA_MEM_PA_BASE         MIPS32_KSEG1_PA(BSP_CFG_ISA_MEM_BASE)
#define BSP_CFG_ISA_MEM_SIZE            0x01000000
/*********************************************************************************************************
  8259A PIC
*********************************************************************************************************/
#define BSP_CFG_8259A_VECTOR            (2)

#define BSP_CFG_8259A_IO_BASE_MASTER    (BSP_CFG_ISA_IO_BASE + 0x20)
#define BSP_CFG_8259A_IO_BASE_SLAVE     (BSP_CFG_ISA_IO_BASE + 0xa0)
#define BSP_CFG_8259A_VECTOR_BASE       (8)
#define BSP_CFG_8259A_SUB_VECTOR_NR     (16)
#define BSP_CFG_8259A_SUB_VECTOR(x)     (BSP_CFG_8259A_VECTOR_BASE + (x))
#define BSP_CFG_8259A1_VECTOR_BASE      BSP_CFG_8259A_VECTOR
#define BSP_CFG_8259A2_VECTOR_BASE      BSP_CFG_8259A_SUB_VECTOR(2)
#define BSP_CFG_8259A_VECTOR_OFFSET     (8)
/*********************************************************************************************************
  16C550 UART
*********************************************************************************************************/
#define BSP_CFG_16C550_SIO_NR           1
#define BSP_CFG_16C550_BASE             (BSP_CFG_ISA_IO_BASE + 0x3F8)
#define BSP_CFG_16C550_BAUD             (115200)
#define BSP_CFG_16C550_VECTOR           BSP_CFG_8259A_SUB_VECTOR(4)
/*********************************************************************************************************
  IDE
*********************************************************************************************************/
#define BSP_CFG_IDE_BASE0               (BSP_CFG_ISA_IO_BASE + 0x1f0)
#define BSP_CFG_IDE_BASE1               (BSP_CFG_ISA_IO_BASE + 0x3f6)
/*********************************************************************************************************
  8254 TIMER
*********************************************************************************************************/
#define BSP_CFG_8254_VECTOR             BSP_CFG_8259A_SUB_VECTOR(0)
#define BSP_CFG_8254_IO_BASE            (BSP_CFG_ISA_IO_BASE + 0x40)
/*********************************************************************************************************
  ne2000 Net
*********************************************************************************************************/
#define BSP_CFG_NE2000_BASE             (BSP_CFG_ISA_IO_BASE + 0x300)
#define BSP_CFG_NE2000_VECTOR           BSP_CFG_8259A_SUB_VECTOR(9)

#endif                                                                  /*  __CONFIG_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
