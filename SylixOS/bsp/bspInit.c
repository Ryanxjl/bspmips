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
** 文   件   名: bspInit.c
**
** 创   建   人: Ryan.xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 09 日
**
** 描        述: BSP 用户 C 程序入口
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  工程配置 & 处理器相关       */
/*********************************************************************************************************
  操作系统相关
*********************************************************************************************************/
#include "SylixOS.h"                                                    /*  操作系统                    */
#include "stdlib.h"                                                     /*  for system() function       */
#include "gdbserver.h"                                                  /*  GDB 调试器                  */
#include "sys/compiler.h"                                               /*  编译器相关                  */
#include "lwip/tcpip.h"                                                 /*  网络系统                    */
#include "netif/etharp.h"                                               /*  以太网网络接口              */
#include "netif/aodvif.h"                                               /*  aodv 多跳自组网网络接口     */
/*********************************************************************************************************
  BSP 及 驱动程序
*********************************************************************************************************/
#include "driver/8250/8250_uart.h"                                      /*  调试端口                    */
#include "driver/16c550/16c550_sio.h"
#include "driver/mc146818/mc146818_rtc.h"
#include "driver/vga/vga_fb.h"
#include "driver/ide/ide.h"
#include "driver/ne2000/ne2000.h"
/*********************************************************************************************************
  操作系统符号表
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0 && defined(__GNUC__)
#include "symbol.h"
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
                                                                        /*  defined(__GNUC__)           */
/*********************************************************************************************************
  内存初始化映射表
*********************************************************************************************************/
#define  __BSPINIT_MAIN_FILE
#include "bspMap.h"
/*********************************************************************************************************
  主线程与启动线程堆栈 (t_boot 可以大一点, startup.sh 中可能有很多消耗堆栈的操作)
*********************************************************************************************************/
#define  __LW_THREAD_BOOT_STK_SIZE      (16 * LW_CFG_KB_SIZE)
#define  __LW_THREAD_MAIN_STK_SIZE      (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  主线程声明
*********************************************************************************************************/
VOID  t_main(VOID);
/*********************************************************************************************************
** 函数名称: halModeInit
** 功能描述: 初始化目标系统运行的模式
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halModeInit (VOID)
{
}
/*********************************************************************************************************
** 函数名称: halTimeInit
** 功能描述: 初始化目标系统时间系统 (系统默认时区为: 东8区)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static VOID  halTimeInit (VOID)
{
    PLW_RTC_FUNCS   prtcfuncs = mc146818RtcGetFuncs();

    rtcDrv();
    rtcDevCreate(prtcfuncs);                                            /*  创建硬件 RTC 设备           */
    rtcToSys();                                                         /*  将 RTC 时间同步到系统时间   */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** 函数名称: halIdleInit
** 功能描述: 初始化目标系统空闲时间作业
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halIdleInit (VOID)
{
#if 0
    API_SystemHookAdd(armWaitForInterrupt,
                      LW_OPTION_THREAD_IDLE_HOOK);                      /*  空闲时暂停 CPU              */
#endif
}
/*********************************************************************************************************
** 函数名称: halCacheInit
** 功能描述: 目标系统 CPU 高速缓冲初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

static VOID  halCacheInit (VOID)
{
    API_CacheLibInit(CACHE_COPYBACK, CACHE_COPYBACK, MIPS_MACHINE_24KF);/*  初始化 CACHE 系统           */
    API_CacheEnable(INSTRUCTION_CACHE);
    API_CacheEnable(DATA_CACHE);                                        /*  使能 CACHE                  */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** 函数名称: halFpuInit
** 功能描述: 目标系统 FPU 浮点运算单元初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

static VOID  halFpuInit (VOID)
{
    API_KernelFpuInit(MIPS_MACHINE_24KF, ARM_FPU_NONE);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** 函数名称: halPmInit
** 功能描述: 初始化目标系统电源管理系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

static VOID  halPmInit (VOID)
{
}

#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
** 函数名称: halBusInit
** 功能描述: 初始化目标系统总线系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halBusInit (VOID)
{
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halDrvInit
** 功能描述: 初始化目标系统静态驱动程序
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDrvInit (VOID)
{
    /*
     *  standard device driver (rootfs and procfs need install first.)
     */
    rootFsDrv();                                                        /*  ROOT   device driver        */
    procFsDrv();                                                        /*  proc   device driver        */
    shmDrv();                                                           /*  shm    device driver        */
    randDrv();                                                          /*  random device driver        */
    ptyDrv();                                                           /*  pty    device driver        */
    ttyDrv();                                                           /*  tty    device driver        */
    memDrv();                                                           /*  mem    device driver        */
    pipeDrv();                                                          /*  pipe   device driver        */
    spipeDrv();                                                         /*  spipe  device driver        */
    fatFsDrv();                                                         /*  FAT FS device driver        */
    ramFsDrv();                                                         /*  RAM FS device driver        */
    romFsDrv();                                                         /*  ROM FS device driver        */
    nfsDrv();                                                           /*  nfs    device driver        */
    yaffsDrv();                                                         /*  yaffs  device driver        */
    canDrv();                                                           /*  CAN    device driver        */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halDevInit
** 功能描述: 初始化目标系统静态设备组件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDevInit (VOID)
{
    /*
     *  创建根文件系统时, 将自动创建 dev, mnt, var 目录.
     */
    rootFsDevCreate();                                                  /*  创建根文件系统              */
    procFsDevCreate();                                                  /*  创建 proc 文件系统          */
    shmDevCreate();                                                     /*  创建共享内存设备            */
    randDevCreate();                                                    /*  创建随机数文件              */

    SIO_CHAN    *psio0 = sioChan16C550Create(0);                        /*  创建串口 0 通道             */
    ttyDevCreate("/dev/ttyS0", psio0, 30, 50);                          /*  add    tty   device         */

    vgaFbDevCreate("/dev/fb0");                                         /*  创建 framebuffer device     */

    idaInit();                                                          /*  初始化 IDE 硬盘             */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halLogInit
** 功能描述: 初始化目标系统日志系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

static VOID  halLogInit (VOID)
{
    fd_set      fdLog;

    FD_ZERO(&fdLog);
    FD_SET(STD_OUT, &fdLog);
    API_LogFdSet(STD_OUT + 1, &fdLog);                                  /*  初始化日志                  */
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
/*********************************************************************************************************
** 函数名称: halStdFileInit
** 功能描述: 初始化目标系统标准文件系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halStdFileInit (VOID)
{
    int     iFd = open("/dev/ttyS0", O_RDWR, 0);

    if (iFd >= 0) {
        ioctl(iFd, FIOBAUDRATE,   SIO_BAUD_115200);
        ioctl(iFd, FIOSETOPTIONS, (OPT_TERMINAL & (~OPT_7_BIT)));       /*  system terminal 8 bit mode  */

        ioGlobalStdSet(STD_IN,  iFd);
        ioGlobalStdSet(STD_OUT, iFd);
        ioGlobalStdSet(STD_ERR, iFd);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halShellInit
** 功能描述: 初始化目标系统 shell 环境, (getopt 使用前一定要初始化 shell 环境)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID  halShellInit (VOID)
{
    API_TShellInit();

    /*
     *  初始化 appl 中间件 shell 接口
     */
    zlibShellInit();
    viShellInit();

    /*
     *  初始化 GDB 调试器
     */
    gdbInit();
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: halVmmInit
** 功能描述: 初始化目标系统虚拟内存管理组件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  halVmmInit (VOID)
{
    API_VmmLibInit(_G_zonedescGlobal, _G_globaldescMap, MIPS_MACHINE_24KF);
    API_VmmMmuEnable();
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** 函数名称: halNetInit
** 功能描述: 网络组件初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

static VOID  halNetInit (VOID)
{
    API_NetInit();                                                      /*  初始化网络系统              */

    /*
     *  初始化网络附加工具
     */
#if LW_CFG_NET_PING_EN > 0
    API_INetPingInit();
    API_INetPing6Init();
#endif                                                                  /*  LW_CFG_NET_PING_EN > 0      */

#if LW_CFG_NET_NETBIOS_EN > 0
    API_INetNetBiosInit();
    API_INetNetBiosNameSet("sylixos");
#endif                                                                  /*  LW_CFG_NET_NETBIOS_EN > 0   */

#if LW_CFG_NET_TFTP_EN > 0
    API_INetTftpServerInit("/tmp");
#endif                                                                  /*  LW_CFG_NET_TFTP_EN > 0      */

#if LW_CFG_NET_FTPD_EN > 0
    API_INetFtpServerInit("/");
#endif                                                                  /*  LW_CFG_NET_FTP_EN > 0       */

#if LW_CFG_NET_TELNET_EN > 0
    API_INetTelnetInit(LW_NULL);
#endif                                                                  /*  LW_CFG_NET_TELNET_EN > 0    */

#if LW_CFG_NET_NAT_EN > 0
    API_INetNatInit();
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */

#if LW_CFG_NET_NPF_EN > 0
    API_INetNpfInit();
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */

#if LW_CFG_NET_VPN_EN > 0
    API_INetVpnInit();
#endif                                                                  /*  LW_CFG_NET_VPN_EN > 0       */
}

static NE2000_DATA  _G_ne2000Data = {
        .uiBaseAddr = BSP_CFG_NE2000_BASE,
        .irq        = BSP_CFG_NE2000_VECTOR,
        .ucMacAddr  = { 0x08, 0x08, 0x3E, 0x26, 0x0A, 0x5A},
};
/*********************************************************************************************************
** 函数名称: halNetifAttch
** 功能描述: 网络接口连接
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halNetifAttch (VOID)
{
    static struct netif  ethernetif;
    ip_addr_t            ip, submask, gateway;

    IP4_ADDR(&ip,       192, 168,   7,  30);
    IP4_ADDR(&submask,  255, 255, 255,   0);
    IP4_ADDR(&gateway,  192, 168,   7,   1);

    netif_add(&ethernetif, &ip, &submask, &gateway,
              &_G_ne2000Data, ne2000_netif_init,
              tcpip_input);

    ethernetif.ip6_autoconfig_enabled = 0;                              /*  允许 IPv6 地址自动配置      */

    netif_set_up(&ethernetif);

    netif_set_default(&ethernetif);

    netif_create_ip6_linklocal_address(&ethernetif, 1);                 /*  构造 ipv6 link address      */

    lib_srand((ethernetif.hwaddr[0] << 24) |
              (ethernetif.hwaddr[1] << 16) |
              (ethernetif.hwaddr[2] <<  8) |
              (ethernetif.hwaddr[3]));                                  /*  可以用 mac 设置随机数种子   */

}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** 函数名称: halMonitorInit
** 功能描述: 内核监控器上传初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

static VOID  halMonitorInit (VOID)
{
    /*
     *  可以再这里创建内核监控器上传通道, 也可以使用 shell 命令操作.
     */
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
** 函数名称: halPosixInit
** 功能描述: 初始化 posix 子系统 (如果系统支持 proc 文件系统, 则必须放在 proc 文件系统安装之后!)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

static VOID  halPosixInit (VOID)
{
    API_PosixInit();
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** 函数名称: halSymbolInit
** 功能描述: 初始化目标系统符号表环境, (为 module loader 提供环境)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0

static VOID  halSymbolInit (VOID)
{
#ifdef __GNUC__
    void *__aeabi_read_tp();
#endif                                                                  /*  __GNUC__                    */

    API_SymbolInit();

#ifdef __GNUC__
    symbolAddAll();

    /*
     *  GCC will emit calls to this routine under -mtp=soft.
     */
    API_SymbolAdd("__aeabi_read_tp", (caddr_t)__aeabi_read_tp, LW_SYMBOL_FLAG_XEN);
#endif                                                                  /*  __GNUC__                    */
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** 函数名称: halLoaderInit
** 功能描述: 初始化目标系统程序或模块装载器
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  halLoaderInit (VOID)
{
    API_LoaderInit();
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** 函数名称: halStdDirInit
** 功能描述: 设备标准文件系统创建
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halStdDirInit (VOID)
{
    /*
     *  当 LW_CFG_PATH_VXWORKS == 0 时, 操作系统会在根目录下自动创建三个挂载点 /dev /mnt /var
     *  其余挂载点需要通过手动创建, 例如: /etc /bin /sbin /tmp 等等,
     *  一般来说 /tmp /bin /sbin /ftk /etc ... 可以做成链接文件, 链接到指定的文件系统.
     */
    mkdir("/usb", DEFAULT_DIR_PERM);

    if (access("/media/hd0/boot", R_OK) < 0) {
        mkdir("/media/hd0/boot", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/etc", R_OK) < 0) {
        mkdir("/media/hd0/etc", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/ftk", R_OK) < 0) {
        mkdir("/media/hd0/ftk", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/qt", R_OK) < 0) {
        mkdir("/media/hd0/qt", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/lib", R_OK) < 0) {
        mkdir("/media/hd0/lib", DEFAULT_DIR_PERM);
        mkdir("/media/hd0/lib/modules", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/usr", R_OK) < 0) {
        mkdir("/media/hd0/usr", DEFAULT_DIR_PERM);
        mkdir("/media/hd0/usr/lib", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/bin", R_OK) < 0) {
        mkdir("/media/hd0/bin", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/sbin", R_OK) < 0) {
        mkdir("/media/hd0/sbin", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/apps", R_OK) < 0) {
        mkdir("/media/hd0/apps", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/home", R_OK) < 0) {
        mkdir("/media/hd0/home", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/root", R_OK) < 0) {
        mkdir("/media/hd0/root", DEFAULT_DIR_PERM);
    }
    if (access("/media/hd0/var", R_OK) < 0) {
        mkdir("/media/hd0/var", DEFAULT_DIR_PERM);
    }

    symlink("/media/hd0/boot", "/boot");
    symlink("/media/hd0/etc",  "/etc");                                 /*  创建根目录符号链接          */
    symlink("/media/hd0/ftk",  "/ftk");                                 /*  创建 FTK 图形系统符号链接   */
    symlink("/media/hd0/qt",   "/qt");                                  /*  创建 Qt 图形系统符号链接    */
    symlink("/media/hd0/lib",  "/lib");
    symlink("/media/hd0/usr",  "/usr");
    symlink("/media/hd0/bin",  "/bin");
    symlink("/media/hd0/sbin", "/sbin");
    symlink("/media/hd0/apps", "/apps");
    symlink("/media/hd0/home", "/home");
    symlink("/media/hd0/root", "/root");
    symlink("/media/hd0/var",  "/var");
    system("mount -t ramfs 0 /tmp");

    system("mount -t ramfs 0 /ramdisk");
}
/*********************************************************************************************************
** 函数名称: halBootThread
** 功能描述: 多任务状态下的初始化启动任务
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  halBootThread (PVOID  pvBootArg)
{
    LW_CLASS_THREADATTR     threakattr = API_ThreadAttrGetDefault();    /*  使用默认属性                */

    (VOID)pvBootArg;

#if LW_CFG_SHELL_EN > 0
    halShellInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_POWERM_EN > 0
    halPmInit();
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

#if LW_CFG_DEVICE_EN > 0
    halBusInit();
    halDrvInit();
    halDevInit();
    halStdFileInit();
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

#if LW_CFG_LOG_LIB_EN > 0
    halLogInit();
    console_loglevel = default_message_loglevel;                        /*  设置 printk 打印信息等级    */
#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */

    halStdDirInit();                                                    /*  创建标准目录                */

    /*
     *  只有初始化了 shell 并获得了 TZ 环境变量标示的时区, 才可以调用 rtcToRoot()
     */
    system("varload");                                                  /*  从/etc/profile中读取环境变量*/
    lib_tzset();                                                        /*  通过 TZ 环境变量设置时区    */
    rtcToRoot();                                                        /*  将 RTC 时间同步到根文件系统 */

    /*
     *  网络初始化一般放在 shell 初始化之后, 因为初始化网络组件时, 会自动注册 shell 命令.
     */
#if LW_CFG_NET_EN > 0
    halNetInit();
    halNetifAttch();                                                    /*  wlan 网卡需要下载固件       */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

#if LW_CFG_POSIX_EN > 0
    halPosixInit();
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#if LW_CFG_SYMBOL_EN > 0
    halSymbolInit();
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */

#if LW_CFG_MODULELOADER_EN > 0
    halLoaderInit();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_MONITOR_EN > 0
    halMonitorInit();
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

    system("shfile /yaffs2/n0/etc/startup.sh");                         /*  执行启动脚本                */
                                                                        /*  必须在初始化 shell 后调用!! */

    API_ThreadAttrSetStackSize(&threakattr, __LW_THREAD_MAIN_STK_SIZE); /*  设置 main 线程的堆栈大小    */
    API_ThreadCreate("t_main",
                     (PTHREAD_START_ROUTINE)t_main,
                     &threakattr,
                     LW_NULL);                                          /*  Create "t_main()" thread    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: usrStartup
** 功能描述: 初始化应用相关组件, 创建操作系统的第一个任务.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  usrStartup (VOID)
{
    LW_CLASS_THREADATTR     threakattr;

    /*
     *  注意, 不要修改该初始化顺序 (必须先初始化 vmm 才能正确的初始化 cache,
     *                              网络需要其他资源必须最后初始化)
     */
    halIdleInit();
#if LW_CFG_CPU_FPU_EN > 0
    halFpuInit();
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_RTC_EN > 0
    halTimeInit();
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */

#if LW_CFG_VMM_EN > 0
    halVmmInit();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#if LW_CFG_CACHE_EN > 0
    halCacheInit();
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    API_ThreadAttrBuild(&threakattr,
                        __LW_THREAD_BOOT_STK_SIZE,
                        LW_PRIO_CRITICAL,
                        LW_OPTION_THREAD_STK_CHK,
                        LW_NULL);
    API_ThreadCreate("t_boot",
                     (PTHREAD_START_ROUTINE)halBootThread,
                     &threakattr,
                     LW_NULL);                                          /*  Create boot thread          */
}
/*********************************************************************************************************
** 函数名称: main
** 功能描述: C 入口
** 输　入  : NONE
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT bspInit (VOID)
{
    /*
     *  系统内核堆与系统堆
     */
    static __section(.noinit) CHAR  cKernelHeap[24 * LW_CFG_MB_SIZE];

    halModeInit();                                                      /*  初始化硬件                  */

    /*
     *  这里的调试端口是脱离操作系统的, 所以他应该不依赖于操作系统而存在.
     *  当系统出现错误时, 这个端口显得尤为关键. (项目成熟后可以通过配置关掉)
     */
#if 0
    debugChannelInit(0);                                                /*  初始化调试接口              */
#endif

    /*
     *  这里使用 bsp 设置启动参数, 如果 bootloader 支持, 可使用 bootloader 设置.
     *  为了兼容以前的项目, 这里 kfpu=yes 允许内核中(包括中断)使用 FPU.
     */
    API_KernelStartParam("ncpus=1 kdlog=no kderror=yes kfpu=no heapchk=yes hz=100 "
                         "varea=0xc0000000 vsize=0x3fffe000");
                                                                        /*  操作系统启动参数设置        */
    API_KernelStart(usrStartup,
                    cKernelHeap,
                    sizeof(cKernelHeap),
                    LW_NULL,
                    0);                                                 /*  启动内核                    */

    return  (0);                                                        /*  不会执行到这里              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
