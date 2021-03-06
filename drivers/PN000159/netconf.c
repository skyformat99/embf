#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "global.h"
#include "netconf.h"
#include <stdio.h>
#include "drivers.h"

struct netif netif[2];
__IO uint32_t TCPTimer = 0;
__IO uint32_t ARPTimer = 0;

#ifdef LWIP_DHCP
__IO uint32_t DHCPfineTimer = 0;
__IO uint32_t DHCPcoarseTimer = 0;
#endif

bool     net_configured;  /* 网络是否已配置标志 */
bool     net_opened = false;          // 网络打开标志
bool     net_lineConnected = false;   // 网线是否连接

#if SYS_OS_SUPPORT

OS_EVENT *m_netmsg_queue = NULL;

#endif

/******************************************************************************
* Function       :ETH_IRQHandler
* Author         :llemmx
* Date           :2011-05-03
* Description    :This function handles ETH interrupt request..
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
extern OS_EVENT *g_ethbox;
void ETH_IRQHandler(void)
{
#if OS_CRITICAL_METHOD == 3
    OS_CPU_SR cpu_sr;
#endif
    
#if SYS_OS_SUPPORT
    if (NULL!=m_netmsg_queue){
        OS_ENTER_CRITICAL();
        OSIntNesting++;
        OSMboxPost(m_netmsg_queue,(void*)1);
        OS_EXIT_CRITICAL();
        OSIntExit();
    }
#endif
    /* Clear the Eth DMA Rx IT pending bits */
    ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
    ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
}

void ETH_GPIO_Conf(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // RESET PB0
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_0);

//   ETHERNET pins configuration. RMII mode
//   - ETH_MII_MDIO / ETH_RMII_MDIO: PA2
//   - ETH_MII_MDC / ETH_RMII_MDC: PC1
//   - ETH_MII_TX_EN / ETH_RMII_TX_EN: PB11
//   - ETH_MII_TXD0 / ETH_RMII_TXD0: PB12
//   - ETH_MII_TXD1 / ETH_RMII_TXD1: PB13
//   - ETH_MII_RX_CLK / ETH_RMII_REF_CLK: PA1
//   - ETH_MII_RX_DV / ETH_RMII_CRS_DV: PA7
//   - ETH_MII_RXD0 / ETH_RMII_RXD0: PC4
//   - ETH_MII_RXD1 / ETH_RMII_RXD1: PC5

    // RX_CLK, MDIO, RX_DV
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_ETH);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_ETH);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_ETH);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);
}


/******************************************************************************
* Function       :Ethernet_Configuration
* Author         :llemmx
* Date           :2011-05-03
* Description    :Configures the Ethernet Interface
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void Ethernet_Configuration(void)
{
    ETH_InitTypeDef ETH_InitStructure;

    /* Reset ETHERNET on AHB Bus */
    ETH_DeInit();

  /* MII/RMII Media interface selection */
#ifdef MII_MODE /* Mode MII */
  SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_MII);
#elif defined RMII_MODE  /* Mode RMII */
  SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII);
#endif

    /* Software reset */
    ETH_SoftwareReset();

    /* Wait for software reset */
    while (ETH_GetSoftwareResetStatus() == SET);
    /* ETHERNET Configuration ------------------------------------------------------*/
    /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
    ETH_StructInit(&ETH_InitStructure);

    /* Fill ETH_InitStructure parametrs */
    /*------------------------   MAC   -----------------------------------*/
    ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable  ;
    ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
    ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
    ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
    ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
    ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
    ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
    ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
    ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
#ifdef CHECKSUM_BY_HARDWARE
    ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif

    /*------------------------   DMA   -----------------------------------*/  

    /* When we use the Checksum offload feature, we need to enable the Store and Forward mode: 
    the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum, 
    if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
    ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; 
    ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;         
    ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;     

    ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;       
    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;   
    ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;                                                          
    ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;      
    ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;                
    ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;          
    ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;                                                                 
    ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;

    /* Configure Ethernet */
    if (0==ETH_Init(&ETH_InitStructure, PHY_ADDRESS)){
        //gGp.gBit.f_net=0;
    }else{
        //gGp.gBit.f_net=1;
    }

    /* Enable the Ethernet Rx Interrupt */
    ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE);
}


/* Lwip协议栈初始化 */
void LwIP_Init(NetParam *netp)
{
    if(netp != NULL)
    {
        struct ip_addr ipaddr;
        struct ip_addr netmask;
        struct ip_addr gw;

        /* Initializes the dynamic memory heap defined by MEM_SIZE.*/
        mem_init();

        /* Initializes the memory pools defined by MEMP_NUM_x.*/
        memp_init();

        if(netp->dhcp_flag == 1)
        {
            ipaddr.addr = 0;
            netmask.addr = 0;
            gw.addr = 0;
        }
        else
        {
            IP4_ADDR(&ipaddr, netp->ip[0], netp->ip[1], netp->ip[2], netp->ip[3]);
            IP4_ADDR(&netmask, netp->submask[0], netp->submask[1], netp->submask[2], netp->submask[3]);
            IP4_ADDR(&gw, netp->gateway[0], netp->gateway[1], netp->gateway[2], netp->gateway[3]);
        }

        //Set_MAC_Address(netp->mac);
        netif[0].hwaddr[0] =  netp->mac[0];
        netif[0].hwaddr[1] =  netp->mac[1];
        netif[0].hwaddr[2] =  netp->mac[2];
        netif[0].hwaddr[3] =  netp->mac[3];
        netif[0].hwaddr[4] =  netp->mac[4];
        netif[0].hwaddr[5] =  netp->mac[5];

        netif_add(&netif[0], &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);

        /* 设置默认网络端口 */
        netif_set_default(&netif[0]);

        if(netp->dhcp_flag == 1)
        {
            dhcp_start(&netif[0]);
        }

        netif_set_up(&netif[0]);
    }
}

/* Lwip数据包处理函数 */
void LwIP_Pkt_Handle(uint8_t id)
{
    ethernetif_input(&netif[0]);
}

/* Lwip周期处理函数，参数为时钟滴答 */
void LwIP_Periodic_Handle(__IO uint32_t localtime)
{
  /* TCP periodic process every 250 ms */
  if (localtime - TCPTimer >= TCP_TMR_INTERVAL)
  {
    TCPTimer =  localtime;
    tcp_tmr();
  }
  /* ARP periodic process every 5s */
  if (localtime - ARPTimer >= ARP_TMR_INTERVAL)
  {
    ARPTimer =  localtime;
    etharp_tmr();
  }

#if LWIP_DHCP
  /* Fine DHCP periodic process every 500ms */
  if (localtime - DHCPfineTimer >= DHCP_FINE_TIMER_MSECS)
  {
    DHCPfineTimer =  localtime;
    dhcp_fine_tmr();
  }
  /* DHCP Coarse periodic process every 60s */
  if (localtime - DHCPcoarseTimer >= DHCP_COARSE_TIMER_MSECS)
  {
    DHCPcoarseTimer =  localtime;
    dhcp_coarse_tmr();
  }
#endif

}


uint8_t GetDHCPStatus(void)
{
    if (netif[0].dhcp!=NULL){
        return netif[0].dhcp->state;
    }
    return 0;
}



void GetLocalIP(uint8_t id,uint8_t *ip)
{
    uint32_t IPaddress = netif[id].ip_addr.addr;

    ip[0] = (uint8_t)(IPaddress >> 24);
    ip[1] = (uint8_t)(IPaddress >> 16);
    ip[2] = (uint8_t)(IPaddress >> 8);
    ip[3] = (uint8_t)(IPaddress);
}

void GetLocalGW(uint8_t id,uint8_t *gw)
{
    uint32_t IPaddress = netif[id].gw.addr;

    gw[0] = (uint8_t)(IPaddress >> 24);
    gw[1] = (uint8_t)(IPaddress >> 16);
    gw[2] = (uint8_t)(IPaddress >> 8);
    gw[3] = (uint8_t)(IPaddress);
}

void GetLocalMask(uint8_t id,uint8_t *submask)
{
    uint32_t IPaddress = netif[id].netmask.addr;

    submask[0] = (uint8_t)(IPaddress >> 24);
    submask[1] = (uint8_t)(IPaddress >> 16);
    submask[2] = (uint8_t)(IPaddress >> 8);
    submask[3] = (uint8_t)(IPaddress);
}

/**********************************************************************************
 * Function       : net_open
 * Author         : Linfei
 * Date           : 2012.09.19
 * Description    : 打开网卡设备
 * Calls          : None
 * Input          : flag:未使用
 *　　　　　　　　mode:未使用
 * Output         : None
 * Return         : OPFAULT：开启失败；
 *                  DSUCCESS：开启成功
 *******************************************************************************
 * History:
 *------------------------------------------------------------------------------
 * 2012.09.19 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 *------------------------------------------------------------------------------
 ***********************************************************************************/
int32_t net_open(int32_t flags, int32_t mode)
{
    int32_t ret = OPFAULT;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    if (net_opened != true)
    {
        /* Enable the Ethernet global Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = (mode >> 8) & 0xFF;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = mode & 0xFF;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
                               RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
        
        ETH_GPIO_Conf();          /* GPIO口初始化 */
        Ethernet_Configuration(); /* 网络控制器初始化 */
        net_opened = true;
        ret = DSUCCESS;
    }
    return ret;
}

/**********************************************************************************
 * Function       : net_close
 * Author         : Linfei
 * Date           : 2012.09.19
 * Description    : 关闭网卡设备以及协议栈
 * Calls          : None
 * Input          : flag:未使用
 *　　　　　　　　mode:未使用
 * Output         : None
 * Return         : OPFAULT：开启失败；
 *                  DSUCCESS：开启成功
 *******************************************************************************
 * History:
 *------------------------------------------------------------------------------
 * 2012.09.19 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 *------------------------------------------------------------------------------
 ***********************************************************************************/
int32_t net_close(void)
{
    if(net_opened == true)
    {
        netif_remove(&netif[0]); /* 关闭Lwip */
        ETH_DeInit();            /* 关闭网络控制器 */
        net_opened = false;
    }
    return DSUCCESS;
}


/**********************************************************************************
 * Function       : net_ioctl
 * Author         : Linfei
 * Date           : 2012.09.19
 * Description    : 网卡设备以及协议栈配置，包括初始化协议栈，IP、子网掩码、网关设定
 *                  和读取，NET_SET_INIT_ALL操作码用于协议栈初始化
 * Calls          : None
 * Input          : op: 操作码，合法操作码见头文件
 *　　　　　　　　  : arg:操作码对应参数
 * Output         : None
 * Return         : OPFAULT：开启失败；
 *                  DSUCCESS：开启成功
 *******************************************************************************
 * History:
 *------------------------------------------------------------------------------
 * 2012.09.19 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 *------------------------------------------------------------------------------
 ***********************************************************************************/
int32_t net_ioctl(uint32_t op, void *arg)
{
    int32_t ret = OPFAULT;
    struct ip_addr ipaddr;
    struct ip_addr netmask;
    struct ip_addr gw;
    NetParam *netp = NULL;
    uint8_t *ip = NULL;
    uint8_t *submask = NULL;
    uint8_t *gateway = NULL;
    //uint8_t *mac = NULL;
    uint8_t *dhcp_flag = NULL;
    
    if(net_opened == true)
    {
        switch(op)
        {
        case NET_SET_INIT_ALL:         /* 所有网络参数初始化 */
            netp = (NetParam *)arg;
            if(netp != NULL)
            {
                LwIP_Init(netp);
                ret = DSUCCESS;
            }
            break;
        case NET_SET_IP:          /* 设置IP地址 */
            ip = (uint8_t *)arg;
            if(ip != NULL)
            {
                IP4_ADDR(&ipaddr, ip[0], ip[1], ip[2], ip[3]);
                netif_set_ipaddr(&netif[0], &ipaddr);
                ret = DSUCCESS;
            }
            break;
        case NET_SET_SUBMASK:     /* 设置子网掩码 */
            submask = (uint8_t *)arg;
            if(submask != NULL)
            {
                IP4_ADDR(&netmask, submask[0], submask[1], submask[2], submask[3]);
                netif_set_netmask(&netif[0], &netmask);
                ret = DSUCCESS;
            }
            break;
//         case NET_SET_MAC:         /* 设置MAC地址 */
//             mac = (uint8_t *)arg;
//             if(mac != NULL)
//             {
//                 Set_MAC_Address(mac);
//                 ret = DSUCCESS;
//             }
//             break;
        case NET_SET_GATEWAY:     /* 设置网关 */
            gateway = (uint8_t *)arg;
            if(gateway != NULL)
            {
                IP4_ADDR(&gw, gateway[0], gateway[1], gateway[2], gateway[3]);
                netif_set_gw(&netif[0], &gw);
                ret = DSUCCESS;
            }
            break;
        case NET_SET_DHCP_ON:     /* 启动DHCP */
            dhcp_start(&netif[0]);
            ret = DSUCCESS;
            break;
        case NET_SET_DHCP_OFF:
            dhcp_stop(&netif[0]); /* 关闭DHCP */
            ret = DSUCCESS;
            break;
        case NET_GET_IP:          /* 获取IP地址 */
            ip = (uint8_t *)arg;
            if(ip != NULL)
            {
                GetLocalIP(0, ip);
                ret = DSUCCESS;
            }
            break;
        case NET_GET_SUBMASK:     /* 获取子网掩码 */
            submask = (uint8_t *)arg;
            if(submask != NULL)
            {
                GetLocalMask(0, submask);
                ret = DSUCCESS;
            }
            break;
        case NET_GET_GATEWAY:     /* 获取网关 */
            gateway = (uint8_t *)arg;
            if(gateway != NULL)
            {
                GetLocalGW(0, gateway);
                ret = DSUCCESS;
            }
            ret = DSUCCESS;
            break;
        case NET_GET_DHCP_STATUS: /* 获取DHCP状态 */
            dhcp_flag = (uint8_t *)arg;
            if(dhcp_flag != NULL)
            {
                *dhcp_flag = GetDHCPStatus();
                ret = DSUCCESS;
            }
            break;
        default:
            break;
        }
    }
    return ret;
}


/********************************************************************************
 * Function       : netline_connected
 * Author         : Linfei
 * Date           : 2012.09.18
 * Description    : 检测网线是否已连接
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None
 ********************************************************************************/
bool netline_connected(void)
{
    bool ret = false;
    if((ETH_ReadPHYRegister(PHY_ADDRESS, PHY_BSR) & PHY_Linked_Status) != 0)
    {
        ret = true;
    }
    return ret;
}

#if SYS_OS_SUPPORT
int32_t net_poll(void *oe)
{
    int32_t ret = OPFAULT;
    
    m_netmsg_queue=(OS_EVENT *)oe;
    
    ret = DSUCCESS;
    return ret;
}
#endif

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
