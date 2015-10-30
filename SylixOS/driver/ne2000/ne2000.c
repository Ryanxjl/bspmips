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
#define IEEE_8023_MIN_FRAME     64      /* Smallest possible ethernet frame */
#define NESM_START_PG           0x40    /* First page of TX buffer */
#define NESM_STOP_PG            0x80    /* Last page +1 of RX ring */
#define TX_PAGES                12      /* Two Tx slots */

#define LINK_SNMP_STAT          1

typedef enum _InterruptStatusFlags_ {
    kPacketReceived             =  0x01,    /* Packet received */
    kPacketTransmitted          =  0x02,    /* Packet transmitted */
    kReceiveError               =  0x04,    /* Receive error */
    kTransmitError              =  0x08,    /* Transmit error */
    kOverwriteWarning           =  0x10,    /* Receive overflow */
    kCounterOverflow            =  0x20,    /* Tally counters need emptying */
    kRemoteDMAComplete          =  0x40,    /* Remote DMA complete */
    kResetStatus                =  0x80,    /* Device has reset (shutdown, error) */
}InterruptStatusFlags;

typedef enum _CommandFlags_ {
    kCommandPage0               =  0x00,    /* Page 0 */
    kCommandStop                =  0x01,    /* Stop: software reset */
    kCommandStart               =  0x02,    /* Start: initialize device */
    kCommandTransmit            =  0x04,    /* Transmit packet */
    kCommandRead                =  0x08,    /* Read DMA (recv data from device) */
    kCommandWrite               =  0x10,    /* Write DMA (send data to device) */
    kCommandDmaDisable          =  0x20,    /* Remote (or no) DMA */
    kCommandPage1               =  0x40,    /* Page 1 */
    kCommandPage2               =  0x80     /* Page 2 */
}CommandFlags;

typedef enum _Ne2kRegister_ {
    kCommandRegister            =  0x00,    /* The command register (for all pages) */
    kPhysicalAddressRegister    =  0x01,
    kPageStartRegister          =  0x01,    /* Starting page of ring bfr WR */
    kPageStopRegister           =  0x02,    /* Ending page +1 of ring bfr WR */
    kBoundaryRegister           =  0x03,    /* Boundary page of ring bfr RD WR */
    kTransmitPageRegister       =  0x04,    /* Transmit starting page WR */
    kTransmitByteCount0         =  0x05,    /* Low  byte of tx byte count WR */
    kTransmitByteCount1         =  0x06,    /* High byte of tx byte count WR */
    kCurrentPageRegister        =  0x07,
    kInterruptStatusRegister    =  0x07,    /* Interrupt status reg RD WR */
    kMulticastRegister          =  0x08,    /* low byte of current remote dma address RD */
    kRemoteStart0Register       =  0x08,    /* Remote start address reg 0 */
    kRemoteStart1Register       =  0x09,    /* Remote start address reg 1 */
    kRemoteCount0Register       =  0x0A,    /* Remote byte count reg WR */
    kRemoteCount1Register       =  0x0B,    /* Remote byte count reg WR */
    kReceiveConfigRegister      =  0x0C,    /* RX configuration reg WR */
    kReceiveStatusRegister      =  0x0C,    /* RX status reg RD */
    kTransmitConfigRegister     =  0x0D,    /* TX configuration reg WR */
    kDataConfigRegister         =  0x0E,    /* Data configuration reg WR */
    kInterruptMaskRegister      =  0x0F,    /* Interrupt mask reg WR */
    kDataRegister               =  0x10,    /* "eprom" data port */
    kResetRegister              =  0x1F     /* Issue a read to reset, a write to clear. */
}Ne2kRegister;

#define DP_P1_CR        0x00
#define DP_P1_PAR0      0x01
#define DP_P1_PAR1      0x02
#define DP_P1_PAR2      0x03
#define DP_P1_PAR3      0x04
#define DP_P1_PAR4      0x05
#define DP_P1_PAR5      0x06
#define DP_P1_CURP      0x07
#define DP_P1_MAR0      0x08
#define DP_P1_MAR1      0x09
#define DP_P1_MAR2      0x0a
#define DP_P1_MAR3      0x0b
#define DP_P1_MAR4      0x0c
#define DP_P1_MAR5      0x0d
#define DP_P1_MAR6      0x0e
#define DP_P1_MAR7      0x0f

/*
 * Data configuration register
 */
#define DP_DCR_WTS      0x01    /* 1=16 bit word transfers */
#define DP_DCR_BOS      0x02    /* 1=Little Endian */
#define DP_DCR_LAS      0x04    /* 1=Single 32 bit DMA mode */
#define DP_DCR_LS       0x08    /* 1=normal mode, 0=loopback */
#define DP_DCR_ARM      0x10    /* 0=no send command (program I/O) */
#define DP_DCR_FIFO_1   0x00    /* FIFO threshold */
#define DP_DCR_FIFO_2   0x20
#define DP_DCR_FIFO_4   0x40
#define DP_DCR_FIFO_6   0x60
#define DP_DCR_8BIT     (DP_DCR_LS|DP_DCR_FIFO_4)
#define DP_DCR_16BIT    (DP_DCR_WTS|DP_DCR_LS|DP_DCR_FIFO_4)

/*
 * Receiver control register
 */
#define DP_RCR_SEP      0x01    /* Save bad(error) packets */
#define DP_RCR_AR       0x02    /* Accept runt packets */
#define DP_RCR_AB       0x04    /* Accept broadcast packets */
#define DP_RCR_AM       0x08    /* Accept multicast packets */
#define DP_RCR_PROM     0x10    /* Promiscuous mode */
#define DP_RCR_MON      0x20    /* Monitor mode - 1=accept no packets */

/*
 * Transmitter control register
 */

#define DP_TCR_NOCRC    0x01    /* 1=inhibit CRC */
#define DP_TCR_NORMAL   0x00    /* Normal transmitter operation */
#define DP_TCR_LOCAL    0x02    /* Internal NIC loopback */
#define DP_TCR_INLOOP   0x04    /* Full internal loopback */
#define DP_TCR_OUTLOOP  0x08    /* External loopback */
#define DP_TCR_ATD      0x10    /* Auto transmit disable */
#define DP_TCR_OFFSET   0x20    /* Collision offset adjust */

/*
 * Interrupt status register
 */
#define DP_ISR_RxP      0x01    /* Packet received */
#define DP_ISR_TxP      0x02    /* Packet transmitted */
#define DP_ISR_RxE      0x04    /* Receive error */
#define DP_ISR_TxE      0x08    /* Transmit error */
#define DP_ISR_OFLW     0x10    /* Receive overflow */
#define DP_ISR_CNT      0x20    /* Tally counters need emptying */
#define DP_ISR_RDC      0x40    /* Remote DMA complete */
#define DP_ISR_RESET    0x80    /* Device has reset (shutdown, error) */

/*
 * Interrupt mask register
 */
#define DP_IMR_RxP      0x01    /* Packet received */
#define DP_IMR_TxP      0x02    /* Packet transmitted */
#define DP_IMR_RxE      0x04    /* Receive error */
#define DP_IMR_TxE      0x08    /* Transmit error */
#define DP_IMR_OFLW     0x10    /* Receive overflow */
#define DP_IMR_CNT      0x20    /* Tall counters need emptying */
#define DP_IMR_RDC      0x40    /* Remote DMA complete */
#define DP_IMR_All      0x3F    /* Everything but remote DMA */


struct ne2000_netif_priv {
    LW_OBJECT_HANDLE            lock;
    LW_OBJECT_HANDLE            tx_sync;
    struct ne2000_data          data;
};

static int iNE2000BaseAddress   = 0;
static UINT8 WordLength         = 0;
static UINT8 tx_start_page      = 0;
static UINT8 rx_start_page      = 0;
static UINT8 stop_page          = 0;
static UINT8 current_page       = 0;

/*
 * Grab the card header for this packet.
 */
struct ReceiveBufferHeader {
    unsigned char  status;
    unsigned char  nextPacket;
    unsigned short receiveCount;
} header;


/*********************************************************************************************************
** Function name:           n2k_outb
** Descriptions:            Write Register
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static VOID  n2k_outb (UINT8  value, Ne2kRegister  reg)
{
    write8(value, iNE2000BaseAddress + reg);
}
/*********************************************************************************************************
** Function name:           n2k_inb
** Descriptions:            Read Register
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static UINT8  n2k_inb (Ne2kRegister  reg)
{
    return  read8(iNE2000BaseAddress + reg);
}
/*********************************************************************************************************
** Function name:           ReadCardMemory
** Descriptions:            Read NE2000 Memory Data
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static VOID  ReadCardMemory (INT  cardAddress, PVOID  data, INT  count)
{
    if (2 == WordLength) {
        count = (count + 1) & 0XFFFE;
    }

    n2k_outb(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);

    n2k_outb(count & 0XFF,  kRemoteCount0Register);
    n2k_outb(count >> 0x08, kRemoteCount1Register);

    n2k_outb(cardAddress  & 0XFF, kRemoteStart0Register);
    n2k_outb(cardAddress >> 0x08, kRemoteStart1Register);

    n2k_outb(kCommandStart | kCommandRead | kCommandPage0, kCommandRegister);

    if (2 == WordLength){
        ins16(iNE2000BaseAddress + kDataRegister, data, (count >> 1));
    } else {
        ins8(iNE2000BaseAddress + kDataRegister, data, count);
    }
}

static VOID  ReadCardMemoryToPbuf (INT  cardAddress, struct pbuf  *p)
{
    struct pbuf    *q;

    n2k_outb(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);

    n2k_outb(p->tot_len &  0XFF,  kRemoteCount0Register);
    n2k_outb(p->tot_len >> 0x08,  kRemoteCount1Register);

    n2k_outb(cardAddress  & 0XFF, kRemoteStart0Register);
    n2k_outb(cardAddress >> 0x08, kRemoteStart1Register);

    n2k_outb(kCommandStart | kCommandRead | kCommandPage0, kCommandRegister);

    if (2 == WordLength){
        for (q = p; q != NULL; q = q->next) {
            ins16(iNE2000BaseAddress + kDataRegister, q->payload, (q->len >> 1));
        }
    } else {
        for (q = p; q != NULL; q = q->next) {
            ins8(iNE2000BaseAddress + kDataRegister, q->payload, q->len);
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
static VOID  WriteCardMemoryFromPbuf (INT  cardAddress, struct pbuf  *p)
{
    struct pbuf    *q;
    u16_t           tot_len = p->tot_len;

    /*
     * Under certain conditions, the NIC can raise the read or write line before
     * raising the port request line, which can corrupt data in the transfer
     * or lock up the bus.  Issuing a "dummy remote read" first makes sure that
     * the line is high.
     */
    CHAR dummy[2] = {0};

    ReadCardMemory(cardAddress, (PVOID)dummy, 2);

    /*
     * Now the data can safely be sent to the card.
     */
    if (2 == WordLength) {
        tot_len = (tot_len + 1) & 0XFFFE;
    }

    n2k_outb(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);

    n2k_outb(tot_len &  0XFF, kRemoteCount0Register);
    n2k_outb(tot_len >> 0x08, kRemoteCount1Register);

    n2k_outb(cardAddress  & 0XFF, kRemoteStart0Register);
    n2k_outb(cardAddress >> 0x08, kRemoteStart1Register);

    n2k_outb(kCommandStart | kCommandWrite, kCommandRegister);

    if (2 == WordLength){
        for (q = p; q != NULL; q = q->next) {
            outs16(iNE2000BaseAddress + kDataRegister, q->payload, (q->len >> 1));
        }
    } else {
        for (q = p; q != NULL; q = q->next) {
            outs8(iNE2000BaseAddress + kDataRegister, q->payload, q->len);
        }
    }

    if(n2k_inb(kTransmitPageRegister) & 0x01){
       printk(KERN_ERR "\nne2000_send success.\n");
   }
}

/*********************************************************************************************************
** Function name:           ne2000_get_hdr
** Descriptions:            Grab the n2000 specific header
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static VOID  ne2000_get_hdr (INT  cardAddress, struct ReceiveBufferHeader  *hdr, UINT8  count)
{
    n2k_outb(kCommandDmaDisable  + kCommandPage0 + kCommandStart, kCommandRegister);
    n2k_outb(sizeof(struct ReceiveBufferHeader), kRemoteCount0Register);
    n2k_outb(0X00, kRemoteCount1Register);
    n2k_outb(0X00, kRemoteStart0Register);       /* On page boundary */
    n2k_outb(count, kRemoteStart1Register);
    n2k_outb(kCommandRead | kCommandStart, kCommandRegister);

    if (2 == WordLength)
        ins16(iNE2000BaseAddress + kDataRegister, hdr, sizeof(struct ReceiveBufferHeader)>>1);
    else
        ins8(iNE2000BaseAddress + kDataRegister, hdr, sizeof(struct ReceiveBufferHeader));

    n2k_outb(DP_ISR_RDC, kInterruptStatusRegister);  /* Ack intr. */
}

/*********************************************************************************************************
** Function name:           ne2000_recv
** Descriptions:            NE2000 接收数据
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static VOID  ne2000_recv (struct netif  *netif)
{
    struct ne2000_netif_priv  *netif_priv = netif->state;
    struct ne2000_data        *ne2000     = &netif_priv->data;
    u16_t                      rx_len;
    struct pbuf               *p;
    err_t                      err;
#if 1
    u8_t                       rxing_page;
    u8_t                       this_frame;
    u8_t                       next_frame;
    u8_t                       current_offset;
#endif

    API_InterVectorDisable((ULONG)ne2000->irq);
    API_SemaphoreMPend(netif_priv->lock, LW_OPTION_WAIT_INFINITE);

#if 1
    /*
     * Get the rx page (incoming packet pointer).
     */
    n2k_outb(kCommandDmaDisable | kCommandPage1, kCommandRegister);
    rxing_page = n2k_inb(DP_P1_CURP);

    n2k_outb(kCommandDmaDisable | kCommandPage0, kCommandRegister);
    /*
     * Remove one frame from the ring.  Boundary is always a page behind.
     */
    this_frame = n2k_inb(kBoundaryRegister) + 1;
    if (this_frame >= stop_page){
        this_frame = rx_start_page;
    }

    current_offset = this_frame << 8;
#endif

    ne2000_get_hdr(0, &header, this_frame);

    rx_len = header.receiveCount - sizeof(header);

    /*
     * 实际就是下一个Buffer
     */
    next_frame = this_frame + 1 + ((rx_len + 4)>>8);

    if (rx_len < 64) {
        rx_len = 64;
    }

    if (2 == WordLength) {
        rx_len = (rx_len + 1) & 0XFFFE;
    }

#if ETH_PAD_SIZE
    rx_len += ETH_PAD_SIZE;
#endif

    if ((p = pbuf_alloc(PBUF_RAW, rx_len, PBUF_POOL)) != NULL) {
#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE);
#endif

        /*
         * Read Data from Hardware
         */
        ReadCardMemoryToPbuf(current_page * NE2000RINGPAGESIZE + sizeof(header), p);

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE);
#endif

        LINK_STATS_INC(link.recv);
#ifdef LINK_SNMP_STAT
        snmp_add_ifinoctets(netif, rx_len);
        snmp_inc_ifinucastpkts(netif);
#endif

        err = netif->input(p, netif);
        if (err != ERR_OK) {
            pbuf_free(p);
            p = NULL;
        }

        /*
         * Update to the next packet.
         */
        next_frame   = header.nextPacket;
        /*
         * This _should_ never happen: it's here for avoiding bad clones.
         */
        if (next_frame >= stop_page) {
            next_frame = rx_start_page;
        }
        current_page = next_frame;
        n2k_outb(current_page == rx_start_page ?
                 stop_page : current_page - 1, kBoundaryRegister);
    } else {
        /*
         *
         *
         *
         */
    }

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
static irqreturn_t  ne2000_eint_isr (struct netif  *netif, ULONG  ulVector)
{
    struct ne2000_netif_priv  *netif_priv = netif->state;
    u8_t                       int_status;

    /*
     * Change to page 0 and read the intr status reg.
     */
    n2k_outb(kCommandPage0 | kCommandDmaDisable, kCommandRegister);

    /*
     * Get NE2000 interrupt status
     */
    int_status = n2k_inb(kInterruptStatusRegister);                     /*  Get ISR status              */

    /*
     * Overwrite Interrupt check
     */
    if (int_status & kReceiveError) {
        printk(KERN_ERR "ne2000_isr: Receive error\n");
        LINK_STATS_INC(link.err);
    }

    if (int_status & kTransmitError) {
        printk(KERN_ERR "ne2000_isr: Transmit error\n");
        LINK_STATS_INC(link.err);
    }

    if (int_status & kOverwriteWarning) {
        printk(KERN_ERR "ne2000_isr: Receive overflow\n");
        LINK_STATS_INC(link.err);
    }

    if (int_status & kCounterOverflow) {
        printk(KERN_ERR "ne2000_isr: Tally counters need emptying\n");
        LINK_STATS_INC(link.err);
    }

    if (int_status & kPacketReceived) {
        netJobAdd((VOIDFUNCPTR)ne2000_recv,
                  netif,
                  0, 0, 0, 0, 0);
    }

    if (int_status & kPacketTransmitted) {
        API_SemaphoreCPost(netif_priv->tx_sync);
    }

    n2k_outb(int_status, kInterruptStatusRegister);                     /*  Clear ISR status            */
    n2k_outb(kCommandPage0 | kCommandDmaDisable | kCommandStart, kCommandRegister);

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

    API_SemaphoreCPend(netif_priv->tx_sync, LW_OPTION_WAIT_INFINITE);

    API_InterVectorDisable((ULONG)ne2000->irq);
    n2k_outb(0X00, kInterruptMaskRegister);
    API_SemaphoreMPend(netif_priv->lock, LW_OPTION_WAIT_INFINITE);

    tx_len = p->tot_len;

#if ETH_PAD_SIZE
    tx_len -= ETH_PAD_SIZE;
#endif

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);
#endif

    if (tx_len < IEEE_8023_MIN_FRAME) {
        tx_len = IEEE_8023_MIN_FRAME;
    }

    WriteCardMemoryFromPbuf(tx_start_page * NE2000RINGPAGESIZE, p);

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif

    /*
     * Trigger a transmit start, assuming the length is valid.
     */
    n2k_outb(kCommandTransmit | kCommandDmaDisable, kCommandRegister);
    n2k_outb(tx_start_page, kTransmitPageRegister);
    n2k_outb((tx_len  & 0XFF),  kTransmitByteCount0);
    n2k_outb((tx_len >> 0X08),  kTransmitByteCount1);
    n2k_outb(kCommandTransmit | kCommandDmaDisable | kCommandStart, kCommandRegister);

    API_SemaphoreMPost(netif_priv->lock);
    n2k_outb(DP_IMR_All, kInterruptMaskRegister);
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
    int iindex  = 0;

#if 0
    n2k_outb(kCommandStop | kCommandDmaDisable | kCommandPage0, kCommandRegister);
    printk(KERN_INFO "Page0 cmd (at %lx) is %x\n",
            iNE2000BaseAddress + kCommandRegister, n2k_inb(kCommandRegister));
    n2k_outb(kCommandStop | kCommandDmaDisable | kCommandPage1, kCommandRegister);
    printk(KERN_INFO "Page1 cmd (at %lx) is %x\n",
            iNE2000BaseAddress + kCommandRegister, n2k_inb(kCommandRegister));
    n2k_outb(kCommandStop | kCommandDmaDisable | kCommandPage0, kCommandRegister);
    printk(KERN_INFO "Page0 cmd (at %lx) is %x\n",
            iNE2000BaseAddress + kCommandRegister, n2k_inb(kCommandRegister));
    n2k_outb(kCommandStop | kCommandDmaDisable | kCommandPage0, kCommandRegister);
    printk(KERN_INFO "Page0 cmd (at %lx) is %x\n",
            iNE2000BaseAddress + kCommandRegister, n2k_inb(kCommandRegister));
#endif

    /*
     * Write the value of RESET into RESET register
     */
    n2k_outb(n2k_inb(kResetRegister), kResetRegister);

    /*
     * Wait for the RESET to complete
     */
    for (iindex = 0; iindex < 100; iindex++) {
        if ((n2k_inb(kInterruptStatusRegister) & kResetStatus) != 0){
            break;
        }
        API_TimeMSleep(1);
    }

    /*
     * Mask Interrupts, Ack all intr
     */
    n2k_outb(0XFF, kInterruptStatusRegister);

    if (iindex == 100) {
        printk(KERN_ERR "ne2000_reset() did not complete.\n");
    }
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
    UINT8 prom[32] = {0};
    UINT  index = 0;

    /*
     * 复位 NE2000 芯片
     */
    ne2000_reset();

    /*
     * 网卡没有Init，选择Page0 ，网卡Stop
     */
    n2k_outb(kCommandStop | kCommandDmaDisable | kCommandPage0, kCommandRegister);
    /*
     * 确保芯片进入停止模式
     */
    API_TimeMSleep(10);
    n2k_outb(DP_DCR_8BIT, kDataConfigRegister);                    /*  byte wide transfers         */
    n2k_outb(0X00, kRemoteCount0Register);                         /* Remote byte count            */
    n2k_outb(0X00, kRemoteCount1Register);
    n2k_outb(0XFF, kInterruptStatusRegister);
    n2k_outb(0X00, kInterruptMaskRegister);
    n2k_outb(DP_RCR_MON, kReceiveConfigRegister);                  /*  Enter monitor mode          */
    n2k_outb(DP_TCR_LOCAL, kTransmitConfigRegister);               /*  Loopback transmit mode      */
#if 0
    n2k_outb(0X20, kRemoteCount0Register);
    n2k_outb(0X00, kRemoteCount1Register);
    n2k_outb(0X00, kRemoteStart0Register);
    n2k_outb(0X00, kRemoteStart1Register);
    n2k_outb(kCommandStart | kCommandRead, kCommandRegister);
#endif
    /*
     * Read data from the prom
     */
    ReadCardMemory(0, prom, 32);

    if(prom[0] == 0x52 && prom[2] == 0x54 && prom[4] == 0x00){
        printk(KERN_INFO "matched board is Qemu\n");
    }

    /*
     * This is a goofy card behavior.  See if every byte is doubled.
     * If so, this card accepts 16 bit wide transfers.
     */
    WordLength = 2;
    for (index = 0; index < 32; index += 2){
        if (prom[index] != prom[index + 1]){
            WordLength = 1;
            break;
        }
    }

    if (2 == WordLength){
        for (index = 0; index < 32; index += 2){
            prom[index >> 1] = prom[index];
        }
        /*
         * 发送缓冲区0X40--0X4B,12个Ring Page，可以存2个最大以太网包 12*256=1536*2
         */
        tx_start_page   = NESM_START_PG;
        /*
         * 接受缓冲区0X4C--0X7F,52个Ring Page
         */
        rx_start_page   = NESM_START_PG + TX_PAGES;
        stop_page       = NESM_STOP_PG;
    }else{
        tx_start_page   = (NESM_START_PG >>1);
        rx_start_page   = ((NESM_START_PG + TX_PAGES) >> 1);
        stop_page       = (NESM_STOP_PG >> 1);
    }

    if (prom[14] != 0X57 || prom[15] != 0X57){
        printk(KERN_ERR "Ne2000: PROM signature does not match Ne2000 0x57.\n");
    }

    memcpy(data->ucMacAddr, prom, ETHER_ADDR_LEN);

    /*
     * Set up card memory
     */
    n2k_outb(kCommandDmaDisable | kCommandPage0 | kCommandStop, kCommandRegister);
    n2k_outb(WordLength == 2? DP_DCR_16BIT : DP_DCR_8BIT, kDataConfigRegister);
    /*
     * Clear the remote byte count registers.
     */
    n2k_outb(0X00, kRemoteCount0Register);
    n2k_outb(0X00, kRemoteCount1Register);
    /*
     * Set to monitor and loopback mode -- this is vital!.
     */
    n2k_outb(DP_RCR_MON, kReceiveConfigRegister);
    n2k_outb(DP_TCR_LOCAL, kTransmitConfigRegister);
    /*
     * Set the transmit page and receive ring.
     */
    n2k_outb(tx_start_page, kTransmitPageRegister);                     /*  发送Buffer范围*/
    n2k_outb(rx_start_page, kPageStartRegister);                        /*  接受Buffer范围*/
    n2k_outb(rx_start_page, kBoundaryRegister);
    current_page = rx_start_page + 1;                                       /* assert boundary + 1 */
    n2k_outb(stop_page, kPageStopRegister);
    /*
     * Clear the pending interrupts and mask.
     */
    n2k_outb(0XFF, kInterruptStatusRegister);
    n2k_outb(0x00, kInterruptMaskRegister);

    /*
     * Now tell the card what its address is
     */
    n2k_outb(kCommandDmaDisable | kCommandPage1 | kCommandStop, kCommandRegister);
    for (index = 0; index < ETHER_ADDR_LEN; index++){
        n2k_outb(data->ucMacAddr[index], (DP_P1_PAR0 + index));
    }
    n2k_outb(current_page, DP_P1_CURP);                        /* Current page - next free page for Rx */

    /*
     * Set up multicast filter to accept all packets.
     */
    for(index = 0; index < 8; index++){
        n2k_outb(0XFF, (DP_P1_MAR0 + index));
    }

    /*
     * Start up the card.
     */
    n2k_outb(kCommandDmaDisable | kCommandStop | kCommandPage0, kCommandRegister);
    n2k_outb(0XFF, kInterruptStatusRegister);
    n2k_outb(DP_IMR_All, kInterruptMaskRegister);
    n2k_outb(kCommandDmaDisable | kCommandStart | kCommandPage0, kCommandRegister);
    n2k_outb(DP_TCR_NORMAL, kTransmitConfigRegister);
    n2k_outb(DP_RCR_AB, kReceiveConfigRegister);

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

    handle = API_SemaphoreCCreate("ethif_tx_sync",
                                  1,
                                  100,
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
    lib_memcpy(netif->hwaddr, netif_priv->data.ucMacAddr, ETHER_ADDR_LEN);

    API_InterVectorConnect((ULONG)netif_priv->data.irq,
                           (PINT_SVR_ROUTINE)ne2000_eint_isr,
                           (PVOID)netif, "ne2000_isr");

    API_InterVectorEnable((ULONG)netif_priv->data.irq);
#if 0
    LW_CLASS_THREADATTR threakattr;
    API_ThreadAttrBuild(&threakattr,
                            LW_CFG_THREAD_IDLE_STK_SIZE,
                            LW_PRIO_IDLE,
                            (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL),
                            LW_NULL);

    API_ThreadCreate("t_ne2000send", (PTHREAD_START_ROUTINE)ne2000_output,
                                     &threakattr, LW_NULL);
#endif
    netif_set_link_up(netif);

    return ERR_OK;
}
/*********************************************************************************************************
** END FILE
*********************************************************************************************************/


