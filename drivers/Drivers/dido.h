/******************************************************************************
 * Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD.
 * File name      :dido.h
 * Author         :wujing
 * Date           :2012.09.20
 * Description    :STM32F10x系列处理器的DIDO模块函数，包括打开DIDO、读取DI状态、
 *                 控制输出DO状态、关闭DIDO
 *
 * Others         :None
 *******************************************************************************

 * 2012-09-20 : 1.0.0 : wujing
 * Modification   : 初始代码编写。
 *******************************************************************************/
 
#ifndef DIDO_H
#define DIDO_H

#include "stm32f4xx.h"
#include "fcntl.h"
#include "io_interface.h"

#define DI_TIME_DELAY        1000U        /*软件输入防抖*/

#define DO_STATE_HIGH           0x0100U    /* ioctl操作的种类，置位DO相应端口*/
#define DO_STATE_LOW            0x0200U    /* ioctl操作的种类，将DO相应端口置0*/
#define DO_STATE_GET            0x0300U    // 获取DO输出管脚的电平状态

#define DI_MAX_NUM          16
#define DO_MAX_NUM          16


/*注意:将此宏加入systick 毫秒中断处理函数中!*/     
#define DIDO_TIMER                 \
   do                             \
   {                              \
       dido_timer();               \
   }while(0)

void dido_timer(void);    

int32_t dido_open(int32_t flag, int32_t mode);
int32_t dido_close(void);
int32_t dido_read(uint8_t buf[], uint32_t count);
int32_t dido_ioctl(uint32_t op, void* arg);
   

   
#endif
