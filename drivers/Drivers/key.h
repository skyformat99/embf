/****************************************************************************** 
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :usartx.h 
 * Author         :Xu Shun'an
 * Date           :2011.06.02
 * Description    :STM32F10x系列处理器的按键模块函数的头文件，定义了当前按键个数、
 *                 按键对应的IO口、指示按键状态的LED灯或者蜂鸣器的IO口、按键的状
 *                 字以及按键驱动函数
 * Others         :None
 *******************************************************************************
 *-------------------------------------------------------------------------------
 * 2011-06-02 : 1.0.1 : xusa
 * Modification   :  整理代码格式
 *-------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 ********************************************************************************/
#ifndef KEY_H
#define KEY_H

#include "stm32f4xx.h"
#include "fcntl.h"
#include "io_interface.h"


#define KEY_STABLE_DELAY       50U      /* 软件消抖动延时(ms)*/
#define LONG_PRESS_DELAY       500U     /* 按下多少ms算作长按 */
#define LONG_PRESS_DELAY_LIMIT 6000U    //wuing 2012.12.10 长按时间最大值，超过则认为按键没按

#define KEY_PULL_UP          1U        /* 开关的连接方式,如果是上拉(开关摁下时,I/O为低电平),则将此定义为1，否则定义为0*/
#define KEY_CLR_STATE        0x0100U    /* ioctl操作的种类，清除按键的状态*/
#define BUZZER_OPEN          0x0200U
#define BUZZER_CLOSE         0x0300U

/*注意:将此宏加入systick 毫秒中断处理函数中!*/     
#define KEY_TIMER                 \
   do                             \
   {                              \
       key_timer();               \
   }while(0)


void key_timer(void);    /*注意:将此函数加入systick 毫秒中断处理函数中!*/
int32_t key_open(int32_t flag, int32_t mode);
int32_t key_close(void);
int32_t key_read(uint8_t buf[], uint32_t count);
int32_t key_ioctl(uint32_t op, void* arg);
int32_t key_poll(void *oe);

#endif
