                                                                               /**
  ******************************************************************************
  * @file    netconf.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    11/20/2009
  * @brief   This file contains all the functions prototypes for the netconf.c 
  *          file.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NETCONF_H
#define __NETCONF_H

#ifdef __cplusplus
 extern "C" {
#endif
     
#include "stm32f4x7_eth.h" 
#include "fcntl.h"

     
/* ioctl操作码 ------------------------*/
#define NET_SET_INIT_ALL      1U
#define NET_SET_IP            2U
#define NET_SET_SUBMASK       3U
#define NET_SET_MAC           4U
#define NET_SET_GATEWAY       5U  
#define NET_SET_DHCP_ON       6U
#define NET_SET_DHCP_OFF      7U
     
     
#define NET_GET_IP            8U
#define NET_GET_SUBMASK       9U
#define NET_GET_MAC           10U
#define NET_GET_GATEWAY       11U
#define NET_GET_DHCP_STATUS   12U
#define NET_GET_CONNECTED     13U
/* End --------------------------------*/



typedef struct
{   
    uint8_t  ip[4];           // IP地址
    uint8_t  submask[4];      // IP子网掩码
    uint8_t  gateway[4];      // 网关
    uint8_t  dns1[4];         // DNS1
    uint8_t  dns2[4];         // DNS2
    uint8_t  mac[6];          // MAC地址
    uint8_t  dhcp_flag;       /* DHCP标志 */   
}NetParam;     

int32_t net_open(int32_t flags, int32_t mode);
int32_t net_ioctl(uint32_t op, void *arg);
int32_t net_close(void);
int32_t net_poll(void *oe);

void LwIP_Pkt_Handle(uint8_t id); /* 协议栈数据接收处理函数 */
void LwIP_Periodic_Handle(__IO uint32_t localtime);  /* 协议栈周期处理函数，需周期调用 */

#ifdef __cplusplus
}
#endif

#endif /* __NETCONF_H */


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/

