/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :EEPROM.h
 * Author         :Linfei
 * Date           :2012.08.11
 * Description    :EEPROM驱动，适用于STM32F4x系列芯片，支持页写和页等待且可配置，
 *                 定义EEPROM使用的通信管脚、操作函数声明
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2013.02.25 : 1.0.1 : Linfei
 * Modification   : 保留最高64B。
 *-------------------------------------------------------------------------------
 * 2012.08.11 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 ********************************************************************************/
#ifndef SPI_EEPROM_H_
#define SPI_EEPROM_H_

#include "stdio.h"
#include "stm32f4xx.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_gpio.h"
#include "io_interface.h"

#ifndef  EEPROM_FM25W256  //FM25W256不需要写等待
    #define  EEPROM_WRITE_WAIT
    #define  EEPROM_WRITE_BYPAGE
#endif



/* 写等待开关
 * 若EEPROM需等待，请定义EEPROM_WRITE_WAIT宏
 * 如需写等待，需设置等待时间，且将EEPROM_TIMER;放入ms中断中 */
#ifdef  EEPROM_WRITE_WAIT
/* 写等待时间，单位ms */
#define EEPROM_WAIT_TIME                5U  /* ms */
#endif


/* 页写开关
 * 若要开启EEPROM页写特性，则须定义EEPROM_WRITE_WAIT宏，否则去除该宏的注释
 * 并设定页大小SPI_EEPROM_PageSize  */
#ifdef  EEPROM_WRITE_BYPAGE
/* 页大小 */
#define SPI_EEPROM_PageSize             64U  /* 字节 */
#endif

/* EEPROM大小，需要指定 */
#define  EEPROM_SIZE                    0x8000








#ifndef  DSUCCESS
 #define  DSUCCESS                      0           /* 操作成功 */
#endif

#ifndef  OPFAULT
 #define  OPFAULT                       (-1)        /* 操作出错 */
#endif

#ifndef DEVBUSY
 #define DEVBUSY                        (-2)        /* 忙 */
#endif



/* 存储区保护 */
#define EEPROM_PRO_NONE                 0x00U    /* 不保护 */
#define EEPROM_PRO_UPER_QUARTER         0x04U    /* 保护高地址端1/4存储区 */
#define EEPROM_PRO_UPER_HALF            0x08U    /* 保护高地址端1/2存储区 */
#define EEPROM_PRO_ALL                  0x0CU    /* 保护所有存储区 */




/* 调用周期1ms，写等待倒计时用 */
#define EEPROM_TIMER              \
   do                             \
   {                              \
       eeprom_write_wait_count(); \
   }while(0)                      


void eeprom_write_wait_count(void);
   
int32_t EEPROM_open(int32_t flag, int32_t mode);
int32_t EEPROM_write(const uint8_t buf[], uint32_t count);
int32_t EEPROM_read(uint8_t buf[], uint32_t count);
int32_t EEPROM_lseek(int64_t offset, int32_t where);
int32_t EEPROM_ioctl(uint32_t op, void *arg);
int32_t EEPROM_close(void);

#endif
