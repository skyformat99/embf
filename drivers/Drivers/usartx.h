/****************************************************************************** 
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :usartx.h 
 * Author         :Xu Shun'an
 * Date           :2011.05.27
 * Description    :STM32F10x系列处理器的串口模块函数的头文件，定义了串口个数、
 *                 默认波特率、PCB板485接口的控制引脚所对应的端口及端口号、进
 *                 行串口配置的类型、串口基本设置以及串口驱动函数
 * Others         :None
 *******************************************************************************
 *------------------------------------------------------------------------------
 * 2011-06-01 : 1.0.1 : xusa
 * Modification   :  整理代码格式
 *------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 ********************************************************************************/
#ifndef USARTX_H_
#define USARTX_H_


/*lint -e40 *//*由于STM32库stm32f10x.h中的USARTx_IRQn未定义数据类型*/
/*lint -e506 *//*该警告目前还没有找到解决办法*/

#include "string.h"
#include "stm32f4xx_usart.h"
#include "fcntl.h"
#include "upoll.h"
#include "io_interface.h"

typedef struct //与USART_InitTypeDef保持一致
{
    /*串口基本设置*/
    uint32_t baudrate; /*波特率(1200~256000)*/
    uint16_t wordlength; /*位长*/
    uint16_t stopbits; /*停止位*/
    uint16_t parity; /*效验方式*/
    uint16_t flowcontrol; /*流控*/
    uint16_t mode; /*模式*/

    /*串口缓冲设置*/
    uint8_t *sbuf; /*发送缓冲*/
    uint16_t sbuflen; /*缓冲长度*/
    uint8_t *rbuf; /*接收缓冲*/
    uint16_t rbuflen; /*缓冲长度*/
} TUsartX_ioctl;


/*处理器中usart硬件的个数*/
#define USART_NUM    6U  //添加对USART6的支持

/*打开串口后默认的波特率*/
#define DEFAULT_BAUD    9600U


/*对串口进行的操作种类*/
#define USART_SET_PORT    1U       /*重新初始化串口*/
#define USART_SET_BUF     2U       /*设置缓冲区*/
#define USART_SET_485E    3U       /*开启485支持*/
#define USART_SET_485D    4U       /*禁止485支持*/
#define USART_SET_SYNC    5U       /*设置为同步模式*/
#define USART_SET_ASYN    6U      /*设置为异步模式*/
#define USART_SET_BAUD    7U      //设置波特率 //wujing 2012.12.04 
#define USART_GET_BAUD    8U      //获取波特率 //wujing 2012.12.04 

/*驱动用定时器(1ms)*/
/*在这里加入驱动需要使用的定时器函数*/
#define UART_TIMER      \
    do                  \
    {                   \
        usartx_timer(); \
    }while(0)           


/*串口通信总线空闲判断函数*/
void usartx_timer(void);


/*usart1驱动*/
int32_t usart1_open(int32_t flags, int32_t mode);
int32_t usart1_close(void);
int32_t usart1_write(const uint8_t buf[], uint32_t count);
int32_t usart1_read(uint8_t buf[], uint32_t count);
int32_t usart1_ioctl(uint32_t cmd, void *arg);

/*usart2驱动*/
int32_t usart2_open(int32_t flags, int32_t mode);
int32_t usart2_close(void);
int32_t usart2_write(const uint8_t buf[], uint32_t count);
int32_t usart2_read(uint8_t buf[], uint32_t count);
int32_t usart2_ioctl(uint32_t cmd, void *arg);

/*usart3驱动*/
int32_t usart3_open(int32_t flags, int32_t mode);
int32_t usart3_close(void);
int32_t usart3_write(const uint8_t buf[], uint32_t count);
int32_t usart3_read(uint8_t buf[], uint32_t count);
int32_t usart3_ioctl(uint32_t cmd, void *arg);

/*usart4驱动*/
int32_t usart4_open(int32_t flags, int32_t mode);
int32_t usart4_close(void);
int32_t usart4_write(const uint8_t buf[], uint32_t count);
int32_t usart4_read(uint8_t buf[], uint32_t count);
int32_t usart4_ioctl(uint32_t cmd, void *arg);

/*usart5驱动*/
int32_t usart5_open(int32_t flags, int32_t mode);
int32_t usart5_close(void);
int32_t usart5_write(const uint8_t buf[], uint32_t count);
int32_t usart5_read(uint8_t buf[], uint32_t count);
int32_t usart5_ioctl(uint32_t cmd, void *arg);

/*usart6驱动*/
int32_t usart6_open(int32_t flags, int32_t mode);
int32_t usart6_close(void);
int32_t usart6_write(const uint8_t buf[], uint32_t count);
int32_t usart6_read(uint8_t buf[], uint32_t count);
int32_t usart6_ioctl(uint32_t cmd, void *arg);

#if SYS_OS_SUPPORT
int32_t usart1_poll(void *ev);
int32_t usart2_poll(void *ev);
int32_t usart3_poll(void *ev);
int32_t usart4_poll(void *ev);
int32_t usart5_poll(void *ev);
#endif


#ifndef  DSUCCESS
 #define  DSUCCESS                      0           /* 操作成功 */
#endif

#ifndef  OPFAULT
 #define  OPFAULT                        (-1)        /* 操作出错 */
#endif

#ifndef DEVBUSY
 #define DEVBUSY                          (-2)        /* 忙 */
#endif

#endif



