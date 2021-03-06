/******************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
 * File name      : canx.h
 * Author         : Liulei
 *                  Linfei
 * Date           : 2011.07.18
 * Description    : STM32F10x系列处理器的CAN通信模块函数的头文件，包括CAN1和CAN2
 *                  打开、关闭、接收、发送、配置函数的声明。最大支持的CAN口数，
 *                  CAN外设列表，ioctl的可用命令以及可设定的波特率。
 *
 * Others         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2012.07.28 : 1.0.2 : Linfei
 * Modification   : 增加对STM32F4xx芯片支持
 *------------------------------------------------------------------------------
 * 2011.07.18 : 1.0.1 : Linfei
 * Modification   : CAN2支持
 *------------------------------------------------------------------------------
 * 2010.08.03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 *------------------------------------------------------------------------------
 ******************************************************************************/

#ifndef CANX_H_
#define CANX_H_
#include <stdint.h>
#include <string.h>
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_can.h"
#include "io_interface.h"

/* 最大支持的CAN口数目，取值为1或2 */
#define MAX_CAN_NUM     2U

/* 接收缓冲队列大小 */
#define MAX_CAN_BUF     32U



#ifndef  DSUCCESS
 #define  DSUCCESS                      0           /* 操作成功 */
#endif

#ifndef  OPFAULT
 #define  OPFAULT                       (-1)        /* 操作出错 */
#endif

#ifndef DEVBUSY
 #define DEVBUSY                        (-2)        /* 忙 */
#endif



/* 错误状态结构体 */
typedef struct
{
    uint32_t rec:8;           /* 接收错误计数 */
    uint32_t tec:8;           /* 发送错误计数 */
    uint32_t lec:4;        /* 上次错误的类型 */
    uint32_t boff:1;       /* 离线标志 */
    uint32_t epvf:1;       /* 错误被动标志 */
    uint32_t ewgf:1;       /* 错误警告标志 */
    uint32_t buserr:1;     /* 出现以上三个错误中的任意一个，该标志置位 */
} CAN_ErrorStateTypeDef;

/* ioctl操作命令 */
#define CAN_SET_PORT        0X01000000U
#define CAN_SET_FILTER      0X02000000U
#define CAN_GET_ERR         0X03000000U
#define CAN_SET_BAUD        0X04000000U

/* 可设定的波特率值 */
#if defined(STM32F4XX)
    #define CAN_BAUD_1Mbps      0x00000006U
    #define CAN_BAUD_500Kbps    0x0000000CU
	#define CAN_BAUD_250Kbps    0x00000018U //wujing 2012.11.26 add for xu
    #define CAN_BAUD_200Kbps    0x0000001EU
    #define CAN_BAUD_100Kbps    0x0000003CU
    #define CAN_BAUD_80Kbps     0x0000004BU
    #define CAN_BAUD_30Kbps     0x000000C8U
    #define CAN_BAUD_20Kbps     0x0000012CU
#else
    #define CAN_BAUD_1Mbps      0x00000004U
    #define CAN_BAUD_500Kbps    0x00000008U
    #define CAN_BAUD_200Kbps    0x00000014U
    #define CAN_BAUD_100Kbps    0x00000028U
    #define CAN_BAUD_80Kbps     0x00000032U
    #define CAN_BAUD_30Kbps     0x00000085U
    #define CAN_BAUD_20Kbps     0x000000c8U
#endif



/* 接口函数定义 */
int32_t can1_open(int32_t flag, int32_t prio);
int32_t can1_release(void);
int32_t can1_read(uint8_t buf[], uint32_t count);
int32_t can1_write(const uint8_t buf[], uint32_t count);
int32_t can1_ioctl(uint32_t op, void *arg);

int32_t can2_open(int32_t flag, int32_t prio);
int32_t can2_release(void);
int32_t can2_read(uint8_t buf[], uint32_t count);
int32_t can2_write(const uint8_t buf[], uint32_t count);
int32_t can2_ioctl(uint32_t op, void *arg);

#if SYS_OS_SUPPORT

int32_t can1_poll(void *ev);
int32_t can2_poll(void *ev);

#endif


#endif

