#include <string.h>
#include <main.h>
#include "includes.h"
#include "lpc17xx_emac.h"
#include  "uip.h"
#include  "LPC177x_8x.h"
//#include "icmp.h"

#define EMAC_NUM_RX_FRAG         4           /**< Num.of RX Fragments 4*1536= 6.0kB */
#define KEIL_BOARD_MCB17XX
/* The following macro definitions may be used to select the speed
   of the physical link:

  _10MBIT_   - connect at 10 MBit only
  _100MBIT_  - connect at 100 MBit only

  By default an autonegotiation of the link speed is used. This may take
  longer to connect, but it works for 10MBit and 100MBit physical links.     */

/* Local Function Prototypes */
static __align(8) RX_Stat Rx_Stat[EMAC_NUM_RX_FRAG];
static void   rx_descr_init(void);
static void   tx_descr_init(void);
static void   write_PHY (uint32_t PhyReg, uint32_t Value);
static uint16_t read_PHY (uint8_t PhyReg) ;

/*--------------------------- int_enable_eth --------------------------------*/

void int_enable_eth (void)
{
    /* 使能Ethernet中断*/
    NVIC_EnableIRQ(ENET_IRQn);
}

/*--------------------------- int_disable_eth -------------------------------*/

void int_disable_eth (void)
{
    /* 失能Ethernet中断*/
    NVIC_DisableIRQ(ENET_IRQn);
}
/*--------------------------- LPC_EMAC_Init ---------------------------------*/
uint32_t LPC_EMAC_Init(void)
{
    /*初始化EMAC以太网控制器*/
    uint32_t regv,tout,id1,id2;

    /*使能以太网控制器电源*/
    LPC_SC->PCONP |= 0x40000000;
    /* Enable P1 Ethernet Pins. */
    LPC_IOCON->P1_0 = 0x01;     //ENET_TXD0
    LPC_IOCON->P1_1 = 0x01;     //ENET_TXD1
    LPC_IOCON->P1_4 = 0x01;     //ENET_TX_EN
    LPC_IOCON->P1_8 = 0x01;     //ENET_CRS_DV
    LPC_IOCON->P1_9 = 0x01;     //ENET_RXD0
    LPC_IOCON->P1_10 = 0x01;     //ENET_RXD1
    LPC_IOCON->P1_14 = 0x01;     //ENET_RX_ER
    LPC_IOCON->P1_15 = 0x01;     //ENET_RX_CLK  
    LPC_IOCON->P1_16 = 0x01;     //ENET_MDC
    LPC_IOCON->P1_17 = 0x01;     //ENET_MDIO

    /*复位以太网模块*/
    LPC_EMAC->MAC1 = MAC1_RES_TX | MAC1_RES_MCS_TX | MAC1_RES_RX | MAC1_RES_MCS_RX |
                     MAC1_SIM_RES | MAC1_SOFT_RES;
    LPC_EMAC->Command = CR_REG_RES | CR_TX_RES | CR_RX_RES;

    /*延时*/
    for (tout = 100; tout; tout--);

    /*初始化以太网控制器寄存器*/
    LPC_EMAC->MAC1 = MAC1_PASS_ALL;
    LPC_EMAC->MAC2 = MAC2_CRC_EN | MAC2_PAD_EN;
    LPC_EMAC->MAXF = ETH_MAX_FLEN;
    LPC_EMAC->CLRT = CLRT_DEF;
    LPC_EMAC->IPGR = IPGR_DEF;

    /*PCLK=18MHz, clock select=6, MDC=18/6=3MHz */
    /* Enable Reduced MII interface. */
    LPC_EMAC->MCFG = MCFG_CLK_DIV20 | MCFG_RES_MII;
    for (tout = 100; tout; tout--);
    LPC_EMAC->MCFG = MCFG_CLK_DIV20;

    /* Enable Reduced MII interface. */
    LPC_EMAC->Command = CR_RMII | CR_PASS_RUNT_FRM ;//| CR_PASS_RX_FILT;

    /* Reset Reduced MII Logic. */
    LPC_EMAC->SUPP = SUPP_RES_RMII | SUPP_SPEED;
    for (tout = 100; tout; tout--);
    LPC_EMAC->SUPP = SUPP_SPEED;

    /* Put the PHY in reset mode */
    write_PHY (PHY_REG_BMCR, 0x8000);
    for (tout = 1000; tout; tout--);

    /*****************************等待硬件复位结束****************************/
    /*    for (tout = 0; tout < 0x100000; tout++)
        {
            regv = read_PHY (PHY_REG_BMCR);
            if (!(regv & 0x8000))
            {
                //Reset complete
                break;
            }
        }
        if (tout >= 0x100000)
            return FALSE; //硬件复位失败
      */
    //等待硬件复位结束
    while( read_PHY (PHY_REG_BMCR) & 0x8000 )
    {
        OSTimeDly(5);
    }

    /****************************校验DP83848C_ID********************************/
    id1 = read_PHY (PHY_REG_IDR1);
    id2 = read_PHY (PHY_REG_IDR2);

    if (((id1 << 16) | (id2 & 0xFFF0)) != DP83848C_ID)
        return FALSE;

    /*****************************PHY 的连接速率设置****************************/
    /*
    #if (_10MBIT_)
        // 手动设置连接速率为10 MBit
        write_PHY (PHY_REG_BMCR, PHY_FULLD_10M);
    #elif (_100MBIT_)
        // 手动设置连接速率为100 MBit
        write_PHY (PHY_REG_BMCR, PHY_FULLD_100M);
    #else
        // 设置连接速率为自适应模式
        write_PHY (PHY_REG_BMCR, PHY_AUTO_NEG);
        //等待自动协商完成
        for (tout = 0; tout < 0x100000; tout++)
        {
            regv = read_PHY (PHY_REG_BMSR);
            if (regv & 0x0020)
            {
                //自动协商完成
                break;
            }
        }
    #endif
        if (tout >= 0x100000)    return FALSE; //自动协商失败 退出 !!!!!
    */
    //设置连接速率为自适应模式
    write_PHY (PHY_REG_BMCR, PHY_AUTO_NEG);
    //等待自动协商完成
    while( !((regv=read_PHY (PHY_REG_BMSR))&0x0020))
    {
        OSTimeDly(5);
    }

    /*****************************检测link 的连接状态****************************/
    /*
    //检测link 的连接状态
    for (tout = 0; tout < 0x10000; tout++)
    {
        regv = read_PHY (PHY_REG_STS);
        if (regv & 0x0001)
        {
            //link连接上
            break;
        }
    }
    if (tout >= 0x10000)  return FALSE;
    */
    //检测link 的连接状态
    while( !(read_PHY (PHY_REG_STS)&0x0001))
    {
        OSTimeDly(5);
    }
    /******************************************************************************/

    /* Configure Full/Half Duplex mode. */
    if (regv & 0x0004)
    {
        /* Full duplex is enabled. */
        LPC_EMAC->MAC2    |= MAC2_FULL_DUP;
        LPC_EMAC->Command |= CR_FULL_DUP;
        LPC_EMAC->IPGT     = IPGT_FULL_DUP;
    }
    else
    {
        /* Half duplex mode. */
        LPC_EMAC->IPGT = IPGT_HALF_DUP;
    }

    /* Configure 100MBit/10MBit mode. */
    if (regv & 0x0002)
    {
        /* 10MBit mode. */
        LPC_EMAC->SUPP = 0;
    }
    else
    {
        /* 100MBit mode. */
        LPC_EMAC->SUPP = SUPP_SPEED;
    }

    /* Set the Ethernet MAC Address registers */
    LPC_EMAC->SA0 = (uip_ethaddr.addr[5] << 8) | uip_ethaddr.addr[4];
    LPC_EMAC->SA1 = (uip_ethaddr.addr[3] << 8) | uip_ethaddr.addr[2];
    LPC_EMAC->SA2 = (uip_ethaddr.addr[1] << 8) | uip_ethaddr.addr[0];


    /* Initialize Tx and Rx DMA Descriptors */
    rx_descr_init ();
    tx_descr_init ();

    /* Receive Broadcast and Perfect Match Packets */
    LPC_EMAC->RxFilterCtrl=  RFC_UCAST_EN | RFC_BCAST_EN | RFC_PERFECT_EN;   //

    /*使能EMAC中断*/
    LPC_EMAC->IntEnable = INT_RX_DONE ;//| INT_TX_DONE;

    /*清除所有中断标志*/
    LPC_EMAC->IntClear  = 0xFFFF;

    /* Enable receive and transmit mode of MAC Ethernet core */
    LPC_EMAC->Command  |= (CR_RX_EN | CR_TX_EN);
    LPC_EMAC->MAC1     |= MAC1_REC_EN;

    /* Configure VIC for LPC_EMAC interrupt. */
    //VICVectAddrxx = (uint32)xx;

    int_enable_eth ();

    return TRUE;
}

/*--------------------------- write_PHY -------------------------------------*/

static void write_PHY (uint32_t PhyReg, uint32_t Value)
{
    unsigned int tout;

    LPC_EMAC->MADR = DP83848C_DEF_ADR | PhyReg;
    LPC_EMAC->MWTD = Value;

    /*等待完成*/
    tout = 0;
    for (tout = 0; tout < MII_WR_TOUT; tout++)
    {
        if ((LPC_EMAC->MIND & MIND_BUSY) == 0)
        {
            break;
        }
    }
}

/*--------------------------- read_PHY -------------------------------------*/

static uint16_t read_PHY (uint8_t PhyReg)
{
    uint32 tout;

    LPC_EMAC->MADR = DP83848C_DEF_ADR | PhyReg;
    LPC_EMAC->MCMD = MCMD_READ;

    /* Wait until operation completed */
    tout = 0;
    for (tout = 0; tout < MII_RD_TOUT; tout++)
    {
        if ((LPC_EMAC->MIND & MIND_BUSY) == 0)
        {
            break;
        }
    }
    LPC_EMAC->MCMD = 0;
    return (LPC_EMAC->MRDD);
}

/*--------------------------- LPC_EMAC_ReadPacket ---------------------------------*/

#define ETH_MTU           1514      // Ethernet Frame Max Transfer Unit       
#define UIP_PROTO_ICMP     1
#define ICMP_ECHO_REPLY    0
#define BUF_ICMP(x)        ((struct uip_icmpip_hdr *)(RX_BUF(x) + UIP_LLH_LEN))

uint16_t EMAC_ReadPacket(void)
{
    uint32_t Index = LPC_EMAC->RxConsumeIndex;
    uint32_t size;
    uint8_t  ch_num;

    if(Index == LPC_EMAC->RxProduceIndex)
    {
        return(0);
    }

    size = (RX_STAT_INFO(Index) & 0x7ff)+1;
    if (size > ETH_FRAG_SIZE)
        size = ETH_FRAG_SIZE;

    //判断收到的数据包是否是ICMP协议包
    if( BUF_ICMP(Index)->proto == UIP_PROTO_ICMP  &&
            BUF_ICMP(Index)->type == ICMP_ECHO_REPLY  )
    {
        for( ch_num=0; ch_num<CHANNEL_NUM; ch_num++ )
        {
            if(BUF_ICMP(Index)->srcipaddr[0] == uip_pingaddr[ch_num][0]&&
                    BUF_ICMP(Index)->srcipaddr[1] == uip_pingaddr[ch_num][1]  )
            {
                //icmp_recseqno[ch_num] = BUF_ICMP(Index)->seqno;
                ICMP_reply_flag[ch_num]=1;
            }
        }
        if(++Index > LPC_EMAC->RxDescriptorNumber)
        {
            Index = 0;
        }
        LPC_EMAC->RxConsumeIndex = Index;
        return(0);
    }
    //是其他数据包，复制到uip_buf 缓存中，到后面在uip_input()处理
    else
    {
        memcpy(uip_buf,(unsigned int *)RX_BUF(Index),size);

        if(++Index > LPC_EMAC->RxDescriptorNumber)
        {
            Index = 0;
        }
        LPC_EMAC->RxConsumeIndex = Index;

        return(size);
    }
}
//-------------------------DP83848中断接收------------------------------
/*#define ETH_MTU         1514      // Ethernet Frame Max Transfer Unit
void ENET_IRQHandler (void)
{
   uint32 idx,int_stat,RxLen,info;
   OS_CPU_SR cpu_sr;
   OS_ENTER_CRITICAL(); //禁止中断
   while((int_stat=(IntStatus & IntEnable)) != 0)
   {
      IntClear = int_stat;
      if (int_stat & INT_RX_DONE)
      {
         idx = RxConsumeIndex;
         while (idx != RxProduceIndex)
         {
            info = RX_STAT_INFO(idx);
            if (!(info & RINFO_LAST_FLAG))
            {
               goto rel;
            }
          	RxLen = (info & 0x7ff)+1;
  			if (RxLen > ETH_FRAG_SIZE)     RxLen = ETH_FRAG_SIZE;

			memcpy(uip_buf,(unsigned int *)RX_BUF(idx),RxLen);

rel:        if (++idx == NUM_RX_FRAG) idx = 0;
            RxConsumeIndex = idx;
         }
      }
	  uip_len=RxLen;
	  OSSemPost(EMAC_ReadPacket_Sem);
	  OS_EXIT_CRITICAL();    //打开中断
   }

} */
/*--------------------------- LPC_EMAC_SendPacket ---------------------------------*/

uint8_t  EMAC_SendPacket(void *pPacket,uint32_t size)
{
//    OS_CPU_SR  cpu_sr;
    uint32_t 	Index;
    uint32_t	IndexNext;

    //OS_ENTER_CRITICAL(); //禁止中断

    IndexNext = LPC_EMAC->TxProduceIndex + 1;
    if(size == 0)
    {
        return(TRUE);
    }
    if(IndexNext > LPC_EMAC->TxDescriptorNumber)
    {
        IndexNext = 0;
    }

    if(IndexNext == LPC_EMAC->TxConsumeIndex)
    {
        return(FALSE);
    }
    Index = LPC_EMAC->TxProduceIndex;
    if (size > ETH_FRAG_SIZE)
        size = ETH_FRAG_SIZE;

    memcpy((unsigned int *)TX_BUF(Index),pPacket,size);

    TX_DESC_CTRL(Index) &= ~0x7ff;
    TX_DESC_CTRL(Index) |= (size - 1) & 0x7ff;

    TX_DESC_CTRL(Index) = (size-1) | (TCTRL_INT | TCTRL_LAST);

    LPC_EMAC->TxProduceIndex = IndexNext;
    
    //OS_EXIT_CRITICAL();    //打开中断
    return(TRUE);
}

/*--------------------------- rx_descr_init ---------------------------------*/

static void rx_descr_init (void)
{
    uint32_t i;

    for (i = 0; i < NUM_RX_FRAG; i++)
    {
        RX_DESC_PACKET(i)  = RX_BUF(i);
        RX_DESC_CTRL(i)    = RCTRL_INT | (ETH_FRAG_SIZE-1);
        RX_STAT_INFO(i)    = 0;
        RX_STAT_HASHCRC(i) = 0;
    }

    /* Set LPC_EMAC Receive Descriptor Registers. */
    LPC_EMAC->RxDescriptor    = RX_DESC_BASE;
    LPC_EMAC->RxStatus        = RX_STAT_BASE;
    LPC_EMAC->RxDescriptorNumber = NUM_RX_FRAG-1;

    /* Rx Descriptors Point to 0 */
    LPC_EMAC->RxConsumeIndex  = 0;
}


/*--------------------------- tx_descr_init ---------------------------------*/

static void tx_descr_init (void)
{
    uint32_t i;

    for (i = 0; i < NUM_TX_FRAG; i++)
    {
        TX_DESC_PACKET(i) = TX_BUF(i);
        TX_DESC_CTRL(i)   = (1U<<31) | (1<<30) | (1<<29) | (1<<28) | (1<<26) | (ETH_FRAG_SIZE-1);
        TX_STAT_INFO(i)   = 0;
    }

    /* Set LPC_EMAC Transmit Descriptor Registers. */
    LPC_EMAC->TxDescriptor    = TX_DESC_BASE;
    LPC_EMAC->TxStatus        = TX_STAT_BASE;
    LPC_EMAC->TxDescriptorNumber = NUM_TX_FRAG-1;

    /* Tx Descriptors Point to 0 */
    LPC_EMAC->TxProduceIndex  = 0;
}
/*********************************************************************//**
 * @brief		Get current status value of receive data (due to RxConsumeIndex)
 * @param[in]	ulRxStatType	Received Status type, should be one of following:
 * 							- EMAC_RINFO_CTRL_FRAME: Control Frame
 * 							- EMAC_RINFO_VLAN: VLAN Frame
 * 							- EMAC_RINFO_FAIL_FILT: RX Filter Failed
 * 							- EMAC_RINFO_MCAST: Multicast Frame
 * 							- EMAC_RINFO_BCAST: Broadcast Frame
 * 							- EMAC_RINFO_CRC_ERR: CRC Error in Frame
 * 							- EMAC_RINFO_SYM_ERR: Symbol Error from PHY
 * 							- EMAC_RINFO_LEN_ERR: Length Error
 * 							- EMAC_RINFO_RANGE_ERR: Range error(exceeded max size)
 * 							- EMAC_RINFO_ALIGN_ERR: Alignment error
 * 							- EMAC_RINFO_OVERRUN: Receive overrun
 * 							- EMAC_RINFO_NO_DESCR: No new Descriptor available
 * 							- EMAC_RINFO_LAST_FLAG: last Fragment in Frame
 * 							- EMAC_RINFO_ERR: Error Occurred (OR of all error)
 * @return		Current value of receive data (due to RxConsumeIndex)
 **********************************************************************/
FlagStatus EMAC_CheckReceiveDataStatus(uint32_t ulRxStatType)
{
    uint32_t idx;
    idx = LPC_EMAC->RxConsumeIndex;
    return (((Rx_Stat[idx].Info) & ulRxStatType) ? SET : RESET);
}
/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
