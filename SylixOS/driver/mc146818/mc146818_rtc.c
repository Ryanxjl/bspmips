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
** 文   件   名: mc146818_rtc.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 18 日
**
** 描        述: mc146818 RTC 驱动源文件.
*********************************************************************************************************/
/*
 * (C) Copyright 2001
 * Denis Peter MPL AG Switzerland. d.peter@mpl.ch
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "time.h"

#undef in8
#undef out8
#define in8(p)      read8(p)
#define out8(p, v)  write8(v, p)

static inline unsigned int bcd2bin(unsigned int val)
{
    return ((val) & 0x0f) + ((val & 0xff) >> 4) * 10;
}

static inline unsigned int bin2bcd(unsigned int val)
{
    return (((val / 10) << 4) | (val % 10));
}

/*
 * Date & Time support for the MC146818 (PIXX4) RTC
 */

/* Set this to 1 to clear the CMOS RAM */
#define CLEAR_CMOS          0

#define RTC_PORT_MC146818   BSP_CFG_ISA_IO_BASE + 0x70
#define RTC_SECONDS         0x00
#define RTC_SECONDS_ALARM   0x01
#define RTC_MINUTES         0x02
#define RTC_MINUTES_ALARM   0x03
#define RTC_HOURS           0x04
#define RTC_HOURS_ALARM     0x05
#define RTC_DAY_OF_WEEK     0x06
#define RTC_DATE_OF_MONTH   0x07
#define RTC_MONTH           0x08
#define RTC_YEAR            0x09
#define RTC_CONFIG_A        0x0a
#define RTC_CONFIG_B        0x0b
#define RTC_CONFIG_C        0x0c
#define RTC_CONFIG_D        0x0d
#define RTC_REG_SIZE        0x80

#define RTC_CONFIG_A_REF_CLCK_32KHZ (1 << 5)
#define RTC_CONFIG_A_RATE_1024HZ    6

#define RTC_CONFIG_B_24H        (1 << 1)

#define RTC_CONFIG_D_VALID_RAM_AND_TIME 0x80

static INT  mc146818_read8 (INT reg)
{
#ifdef CONFIG_SYS_RTC_REG_BASE_ADDR
    return in8(CONFIG_SYS_RTC_REG_BASE_ADDR + reg);
#else
    INT  ofs = 0;

    if (reg >= 128) {
        ofs = 2;
        reg -= 128;
    }
    out8(RTC_PORT_MC146818 + ofs, reg);

    return in8(RTC_PORT_MC146818 + ofs + 1);
#endif
}

static VOID  mc146818_write8 (INT  reg, UINT8  val)
{
#ifdef CONFIG_SYS_RTC_REG_BASE_ADDR
    out8(CONFIG_SYS_RTC_REG_BASE_ADDR + reg, val);
#else
    INT  ofs = 0;

    if (reg >= 128) {
        ofs = 2;
        reg -= 128;
    }
    out8(RTC_PORT_MC146818 + ofs, reg);
    out8(RTC_PORT_MC146818 + ofs + 1, val);
#endif
}

static INT  mc146818_get (PLW_RTC_FUNCS  pRtcFuncs, time_t  *pTimeNow)
{
    UINT8       sec, _min, hour, mday, wday, mon, year;
    struct tm   tmNow;

    /* here check if rtc can be accessed */
    while ((mc146818_read8(RTC_CONFIG_A) & 0x80) == 0x80) {
        ;
    }

    sec     = mc146818_read8(RTC_SECONDS);
    _min    = mc146818_read8(RTC_MINUTES);
    hour    = mc146818_read8(RTC_HOURS);
    mday    = mc146818_read8(RTC_DATE_OF_MONTH);
    wday    = mc146818_read8(RTC_DAY_OF_WEEK);
    mon     = mc146818_read8(RTC_MONTH);
    year    = mc146818_read8(RTC_YEAR);
    tmNow.tm_sec  = bcd2bin(sec & 0x7f);
    tmNow.tm_min  = bcd2bin(_min & 0x7f);
    tmNow.tm_hour = bcd2bin(hour & 0x3f);
    tmNow.tm_mday = bcd2bin(mday & 0x3f);
    tmNow.tm_mon  = bcd2bin(mon & 0x1f) - 1;
    tmNow.tm_year = bcd2bin(year);
    tmNow.tm_wday = bcd2bin(wday & 0x07);

    if (tmNow.tm_year < 70) {
        tmNow.tm_year += 100;
    }

    tmNow.tm_yday = 0;
    tmNow.tm_isdst = 0;

    if (pTimeNow) {
        *pTimeNow = timegm(&tmNow);
    }

    return  (ERROR_NONE);
}

static INT  mc146818_set (PLW_RTC_FUNCS  pRtcFuncs, time_t  *pTimeNow)
{
    INTREG          iregInterLevel;
    struct tm       tmNow;

    gmtime_r(pTimeNow, &tmNow);                                         /*  转换成 tm 时间格式          */

    API_InterLock(&iregInterLevel);

    /* Disable the RTC to update the regs */
    mc146818_write8(RTC_CONFIG_B, 0x82);

    mc146818_write8(RTC_YEAR, bin2bcd(tmNow.tm_year % 100));
    mc146818_write8(RTC_MONTH, bin2bcd(tmNow.tm_mon + 1));
    mc146818_write8(RTC_DAY_OF_WEEK, bin2bcd(tmNow.tm_wday));
    mc146818_write8(RTC_DATE_OF_MONTH, bin2bcd(tmNow.tm_mday));
    mc146818_write8(RTC_HOURS, bin2bcd(tmNow.tm_hour));
    mc146818_write8(RTC_MINUTES, bin2bcd(tmNow.tm_min));
    mc146818_write8(RTC_SECONDS, bin2bcd(tmNow.tm_sec));

    /* Enable the RTC to update the regs */
    mc146818_write8(RTC_CONFIG_B, 0x02);

    API_InterUnlock(iregInterLevel);

    return  (ERROR_NONE);
}

#if 0
static VOID  mc146818_reset (VOID)
{
    /* Disable the RTC to update the regs */
    mc146818_write8(RTC_CONFIG_B, 0x82);

    /* Normal OP */
    mc146818_write8(RTC_CONFIG_A, 0x20);
    mc146818_write8(RTC_CONFIG_B, 0x00);
    mc146818_write8(RTC_CONFIG_B, 0x00);

    /* Enable the RTC to update the regs */
    mc146818_write8(RTC_CONFIG_B, 0x02);
}
#endif

static VOID  mc146818_init (VOID)
{
    /* Setup the real time clock */
    mc146818_write8(RTC_CONFIG_B, RTC_CONFIG_B_24H);
    /* Setup the frequency it operates at */
    mc146818_write8(RTC_CONFIG_A, RTC_CONFIG_A_REF_CLCK_32KHZ |
            RTC_CONFIG_A_RATE_1024HZ);
    /* Ensure all reserved bits are 0 in register D */
    mc146818_write8(RTC_CONFIG_D, RTC_CONFIG_D_VALID_RAM_AND_TIME);

    /* Clear any pending interrupts */
    mc146818_read8(RTC_CONFIG_C);
}

static LW_RTC_FUNCS     _G_mc146818RtcFuncs = {
        mc146818_init,
        mc146818_set,
        mc146818_get,
        LW_NULL
};
/*********************************************************************************************************
** 函数名称: mc146818RtcGetFuncs
** 功能描述: 获取 RTC 驱动程序
** 输　入  : NONE
** 输　出  : RTC 驱动程序
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_RTC_FUNCS  mc146818RtcGetFuncs (VOID)
{
    return (&_G_mc146818RtcFuncs);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
