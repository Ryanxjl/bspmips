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
** 文   件   名: ne2000.c
**
** 创   建   人: Ryan.Xin(信金龙)
**
** 文件创建日期: 015 年 10 月 21 日
**
** 描        述: NE2000 以太网芯片驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "ne2000.h"

#define IFNAME0                 'e'
#define IFNAME1                 'n'
#define HOSTNAME                "lwIP"
#define NE2000_SPEED            10000000
#define NE2000_MTU              1550
#define NE2000RINGPAGESIZE      256

#define E_INVALID_OPERATION (-11)
#define E_NO_ERROR (0)


typedef enum _InterruptStatusFlags_ {
    kPacketReceived             =  0x01,
    kPacketTransmitted          =  0x02,
    kReceiveError               =  0x04,
    kTransmitError              =  0x08,
    kOverwriteWarning           =  0x10,
    kCounterOverflow            =  0x20,
    kRemoteDMAComplete          =  0x40,
    kResetStatus                =  0x80,
}InterruptStatusFlags;

typedef enum _CommandFlags_ {
    kCommandPage0               =  0x00,
    kCommandStop                =  0x01,
    kCommandStart               =  0x02,
    kCommandTransmit            =  0x04,
    kCommandRead                =  0x08,
    kCommandWrite               =  0x10,
    kCommandDmaDisable          =  0x20,
    kCommandPage1               =  0x40,
    kCommandPage2               =  0x80
}CommandFlags;

typedef enum _Ne2kRegister_ {
    kCommandRegister            =  0x00,
    kPhysicalAddressRegister    =  0x01,
    kPageStartRegister          =  0x01,
    kPageStopRegister           =  0x02,
    kBoundaryRegister           =  0x03,
    kTransmitPageRegister       =  0x04,
    kTransmitByteCount0         =  0x05,
    kTransmitByteCount1         =  0x06,
    kCurrentPageRegister        =  0x07,
    kInterruptStatusRegister    =  0x07,
    kMulticastRegister          =  0x08,
    kRemoteStart0Register       =  0x08,
    kRemoteStart1Register       =  0x09,
    kRemoteCount0Register       =  0x0A,
    kRemoteCount1Register       =  0x0B,
    kReceiveConfigRegister      =  0x0C,
    kReceiveStatusRegister      =  0x0C,
    kTransmitConfigRegister     =  0x0D,
    kDataConfigRegister         =  0x0E,
    kInterruptMaskRegister      =  0x0F,
    kDataRegister               =  0x10,
    kResetRegister              =  0x1F
}Ne2kRegister;

struct ne2000_netif_priv {
    LW_OBJECT_HANDLE            lock;
    LW_OBJECT_HANDLE            tx_sync;
    struct ne2000_data          data;
};

static int iNE2000BaseAddress   = 0;
static int fNextReceivePacket   = 0;
static int fWordLength          = 0;
static int fReceiveRingStart    = 0;
static int fReceiveRingEnd      = 0;
static int fTransmitRingStart   = 0;


/*********************************************************************************************************
** Function name:           WriteRegister
** Descriptions:            Write Register
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static VOID WriteRegister(INT value, Ne2kRegister reg)
{
    write8(value, iNE2000BaseAddress + reg);
}

/*********************************************************************************************************
** Function name:           ReadRegister
** Descriptions:            Read Register
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static UINT8 ReadRegister(Ne2kRegister reg)
{
    return read8(iNE2000BaseAddress + reg);
}

/*********************************************************************************************************
** Function name:           ReadCardMemory
** Descriptions:            Read NE2000 Memory Data
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void ReadCardMemory(INT cardAddress, PVOID data, INT count)
{
    PUCHAR uchData;
    if (2 == fWordLength){
        count =  (count + 1) & 0XFFFE;
    }

    WriteRegister(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);
    WriteRegister(count & 0XFF, kRemoteCount0Register);
    WriteRegister(count >> 0x08, kRemoteCount1Register);
    WriteRegister(cardAddress & 0XFF, kRemoteStart0Register);
    WriteRegister(cardAddress >> 0x08, kRemoteStart1Register);
    WriteRegister(kCommandStart | kCommandRead | kCommandPage0, kCommandRegister);

    if (2 == fWordLength){
        ins16(iNE2000BaseAddress + kDataRegister, (INT16 *)data, (count>>1));
    }else{
        uchData = (PUCHAR)data;
        while (count-- > 0){
            *uchData++ = ReadRegister(kDataRegister);
        }
    }
}

/*********************************************************************************************************
** Function name:           WriteCardMemory
** Descriptions:            Write NE2000 Memory Data
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static void WriteCardMemory(int cardAddress, const void *data, int count)
{
    /*
     * Under certain conditions, the NIC can raise the read or write line before
     * raising the port request line, which can corrupt data in the transfer
     * or lock up the bus.  Issuing a "dummy remote read" first makes sure that
     * the line is high.
     */

    PUCHAR uchData;
    CHAR dummy[2] = {0};
    ReadCardMemory(cardAddress, (PVOID)dummy, 2);

    /*
     * Now the data can safely be sent to the card.
     */
    if (2 == fWordLength){
        count = (count + 1) & 0XFFFE;
    }

    WriteRegister(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);
    WriteRegister(count & 0XFF, kRemoteCount0Register);
    WriteRegister(count >> 0x08, kRemoteCount1Register);
    WriteRegister(cardAddress & 0XFF, kRemoteStart0Register);
    WriteRegister(cardAddress >> 0x08, kRemoteStart1Register);
    WriteRegister(kCommandStart | kCommandWrite, kCommandRegister);

    if (2 == fWordLength){
        outs16(iNE2000BaseAddress + kDataRegister, (short*)data, (count>>1));
    }else {
        uchData = (PUCHAR)data;
        while (count-- > 0)
            WriteRegister(*uchData++, kDataRegister);
    }
}

/*********************************************************************************************************
** Function name:           ne2000_recv
** Descriptions:            NE2000 接收数据
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static void ne2000_recv(struct netif *netif)
{
    struct ne2000_netif_priv  *netif_priv = netif->state;
    struct ne2000_data        *ne2000 = &netif_priv->data;
    char buffer[1024] = {0};

    /*
     * Grab the card header for this packet.
     */
    struct ReceiveBufferHeader {
        unsigned char status;
        unsigned char nextPacket;
        unsigned short receiveCount;
    }header;

    API_InterVectorDisable((ULONG)ne2000->irq);
    API_SemaphoreMPend(netif_priv->lock, LW_OPTION_WAIT_INFINITE);

    if (ReadRegister(kInterruptStatusRegister) & kOverwriteWarning){
        return;
    }

    ReadCardMemory(fNextReceivePacket * NE2000RINGPAGESIZE, &header, sizeof(header));
    /*
     * Read the body of the packet.
     */
    ReadCardMemory(fNextReceivePacket * NE2000RINGPAGESIZE + sizeof(header), buffer,
            header.receiveCount - sizeof(header));

    /*
     * Update to the next packet.
     */
    fNextReceivePacket = header.nextPacket;
    WriteRegister(fNextReceivePacket == fReceiveRingStart ?
            fReceiveRingEnd : fNextReceivePacket - 1, kBoundaryRegister);

    API_SemaphoreMPost(netif_priv->lock);
    API_InterVectorEnable((ULONG)ne2000->irq);
}


/*********************************************************************************************************
** Function name:           ne2000_eint_isr
** Descriptions:            NE2000 外部中断服务函数
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          中断返回值
*********************************************************************************************************/
static irqreturn_t ne2000_eint_isr(struct netif *netif, ULONG ulVector)
{
    struct ne2000_netif_priv  *netif_priv = netif->state;

    u8_t                       int_status;

    /*
     * Get NE2000 interrupt status
     */
    int_status = ReadRegister(kInterruptStatusRegister);                /*  Get ISR status              */

    if (int_status & kPacketReceived){
        if (netJobAdd((VOIDFUNCPTR)ne2000_recv,
                          netif,
                          0, 0, 0, 0, 0) == ERROR_NONE) {
            }
    }

    if (int_status & kPacketTransmitted){
        API_SemaphoreBPost(netif_priv->tx_sync);
    }

    WriteRegister(int_status, kInterruptStatusRegister);

    return LW_IRQ_HANDLED;
}

/*********************************************************************************************************
** Function name:           ne2000_output
** Descriptions:            NE2000 发送数据
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static err_t ne2000_output(struct netif *netif, struct pbuf *p)
{
    struct ne2000_netif_priv  *netif_priv = netif->state;
    struct ne2000_data        *ne2000 = &netif_priv->data;
    u16_t                      tx_len;

    API_SemaphoreBPend(netif_priv->tx_sync, LW_OPTION_WAIT_INFINITE);

    API_InterVectorDisable((ULONG)ne2000->irq);

    API_SemaphoreMPend(netif_priv->lock, LW_OPTION_WAIT_INFINITE);

    tx_len  = p->tot_len;

#if ETH_PAD_SIZE
    tx_len -= ETH_PAD_SIZE;
#endif

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);
#endif

    if (tx_len < 64) {
        tx_len = 64;
    }

    WriteCardMemory(fTransmitRingStart * NE2000RINGPAGESIZE, p->payload, tx_len);
    WriteRegister(fTransmitRingStart, kTransmitPageRegister);
    WriteRegister((tx_len >> 0X08), kTransmitByteCount0);
    WriteRegister((tx_len & 0XFF), kTransmitByteCount1);
    WriteRegister(kCommandTransmit | kCommandDmaDisable, kCommandRegister);

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif

    API_SemaphoreMPost(netif_priv->lock);

    API_InterVectorEnable((ULONG)ne2000->irq);

    LINK_STATS_INC(link.xmit);

#ifdef LINK_SNMP_STAT
    snmp_add_ifoutoctets(netif, tx_len);
    snmp_inc_ifoutucastpkts(netif);
#endif

    return ERR_OK;
}
/*********************************************************************************************************
** Function name:           ne2000_reset
** Descriptions:            初始化 NE2000 芯片
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static VOID ne2000_reset()
{
    WriteRegister(ReadRegister(kResetRegister), kResetRegister);
    while(!(ReadRegister(kInterruptStatusRegister) & kResetStatus));
    WriteRegister(0XFF, kInterruptStatusRegister);
}

/*********************************************************************************************************
** Function name:           ne2000_init
** Descriptions:            初始化 NE2000 芯片
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static err_t ne2000_init(struct netif *netif)
{
    struct ne2000_netif_priv  *netif_priv = netif->state;
    struct ne2000_data        *data = &netif_priv->data;
    CHAR prom[32] = {0};
    INT  index = 0;

    /*
     * 复位 NE2000 芯片
     */
    ne2000_reset();

    /*
     * Put the card in loopback mode
     */
    WriteRegister(kCommandStop | kCommandDmaDisable | kCommandPage0, kCommandRegister);
    WriteRegister(0X48, kDataConfigRegister);                           /*  byte wide transfers         */
    WriteRegister(0X00, kRemoteCount0Register);
    WriteRegister(0XFF, kRemoteCount1Register);
    WriteRegister(0XFF, kInterruptStatusRegister);
    WriteRegister(0XFF, kInterruptMaskRegister);
    WriteRegister(0X20, kReceiveConfigRegister);                        /*  Enter monitor mode          */
    WriteRegister(0X02, kTransmitConfigRegister);                       /*  Loopback transmit mode      */

    /*
     * Read data from the prom
     */
    ReadCardMemory(0, prom, 32);

    /*
     * This is a goofy card behavior.  See if every byte is doubled.
     * If so, this card accepts 16 bit wide transfers.
     */
    fWordLength = 2;
    for (index = 0; index < 32; index += 2){
        if (prom[index] != prom[index + 1]){
            fWordLength = 1;
            break;
        }
    }

    if (2 == fWordLength){
        for (index = 0; index < 32; index += 2){
            prom[index / 2] = prom[index];
        }
        fTransmitRingStart  = 0X40;
        fReceiveRingStart   = 0X4c;
        fReceiveRingEnd     = 0X80;
    }else{
        fTransmitRingStart  = 0X20;
        fReceiveRingStart   = 0X26;
        fReceiveRingEnd     = 0X40;
    }

    if (prom[14] != 0X57 || prom[15] != 0X57){
        printf("Ne2000: PROM signature does not match Ne2000 0x57.\n");
    }

    memcpy(data->ucMacAddr, prom, 6);

    /*
     * Set up card memory
     */
    fNextReceivePacket = fReceiveRingStart;
    WriteRegister(kCommandDmaDisable | kCommandPage0 | kCommandStop, kCommandRegister);
    WriteRegister(fWordLength == 2 ? 0X49 : 0X48, kDataConfigRegister);
    WriteRegister(0X00, kRemoteCount0Register);
    WriteRegister(0X00, kRemoteCount1Register);
    WriteRegister(0X20, kReceiveConfigRegister);                        /*  Enter monitor mode          */
    WriteRegister(0X02, kTransmitConfigRegister);                       /*  Loopback transmit mode      */
    WriteRegister(fTransmitRingStart, kTransmitPageRegister);
    WriteRegister(fReceiveRingStart, kPageStartRegister);
    WriteRegister(fNextReceivePacket - 1, kBoundaryRegister);
    WriteRegister(fReceiveRingEnd, kPageStopRegister);
    WriteRegister(0XFF, kInterruptStatusRegister);
    WriteRegister(0X00, kInterruptMaskRegister);

    /*
     * Now tell the card what its address is
     */
    WriteRegister(kCommandDmaDisable | kCommandPage1 | kCommandStop, kCommandRegister);
    for (index = 0; index < 6; index++){
        WriteRegister(prom[index], (kPhysicalAddressRegister + index));
    }

    /*
     * Set up multicast filter to accept all packets.
     */
    for(index = 0; index < 8; index++){
        WriteRegister(0XFF, (kMulticastRegister + index));
    }

    WriteRegister(fNextReceivePacket, kCurrentPageRegister);

    /*
     * Start up the card.
     */
    WriteRegister(kCommandDmaDisable | kCommandPage0 | kCommandStop, kCommandRegister);
    WriteRegister(0XFF, kInterruptStatusRegister);
    WriteRegister(kPacketReceived | kPacketTransmitted | kTransmitError | kOverwriteWarning,
            kInterruptMaskRegister);
    WriteRegister(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);
    WriteRegister(0X00, kTransmitConfigRegister);
    WriteRegister(0X04, kReceiveConfigRegister);

    return 0;
}
/*********************************************************************************************************
** 函数名称: ne2000_netif_init
** 功能描述: 初始化以太网网络接口
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
err_t ne2000_netif_init(struct netif *netif)
{
    struct ne2000_netif_priv  *netif_priv;
    int                        ret;
    LW_OBJECT_HANDLE           handle;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    if (netif->state == NULL) {
        printk(KERN_ERR "ne2000_netif_init: invalid ne2000_data\n");
        return ERR_ARG;
    }

    netif_priv = (struct ne2000_netif_priv *)mem_malloc(sizeof(struct ne2000_netif_priv));
    if (netif_priv == NULL) {
        printk(KERN_ERR "ne2000_netif_init: out of memory\n");
        return ERR_MEM;
    }

#if LWIP_NETIF_HOSTNAME
    /*
     * Initialize interface hostname
     */
    netif->hostname = HOSTNAME;
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, NE2000_SPEED);

    lib_memcpy(&netif_priv->data,
               netif->state,
               sizeof(netif_priv->data));

    netif->state   = netif_priv;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    /*
     * We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...)
     */
    netif->output     = etharp_output;
    netif->linkoutput = ne2000_output;
    netif->output_ip6 = ethip6_output;

    /*
     * Set MAC hardware address length
     */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /*
     * Set Maximum transfer unit
     */
    netif->mtu = NE2000_MTU;

    /*
     * Set Device capabilities
     */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    handle = API_SemaphoreMCreate("ethif_lock",
                                  0,
                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                  &netif_priv->lock);
    if (handle == LW_HANDLE_INVALID) {
        printk(KERN_ERR "ne2000_netif_init: out of memory\n");
        return ERR_MEM;
    }

    handle = API_SemaphoreBCreate("ethif_tx_sync",
                                  1,
                                  LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                  &netif_priv->tx_sync);
    if (handle == LW_HANDLE_INVALID) {
        printk(KERN_ERR "ne2000_netif_init: out of memory\n");
        return ERR_MEM;
    }

    iNE2000BaseAddress = (addr_t)netif_priv->data.uiBaseAddr;

    /*
     * 初始化 NE2000 芯片
     */
    ret = ne2000_init(netif);
    if (ret != 0) {
        printk(KERN_ERR "ne2000_netif_init: failed to init ne2000\n");
        return ERR_IF;
    }

    /*
     * Set MAC hardware address
     */
    lib_memcpy(netif->hwaddr, netif_priv->data.ucMacAddr, 6);

    API_InterVectorConnect((ULONG)netif_priv->data.irq,
                           (PINT_SVR_ROUTINE)ne2000_eint_isr,
                           (PVOID)netif, "ne2000_isr");

    API_InterVectorEnable((ULONG)netif_priv->data.irq);

    netif_set_link_up(netif);

    return ERR_OK;
}
/*********************************************************************************************************
** END FILE
*********************************************************************************************************/


