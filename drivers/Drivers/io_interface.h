/********************************************************************************
 * Copyright (C), 1997-2013, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :io_interface.h
 * Author         :wujing
 * Date           :2013.01.18
 * Description    :所有PCB板的端口定义
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2013-01-18 : 1.0.0 : wujing
 * Modification   : 初始代码编写。
 ********************************************************************************/
 
#ifndef IO_INTERFACE_H_
#define IO_INTERFACE_H_

#include "stdio.h"
#include "drivers.h"
#include "global.h"

/* 设备ID号 */
typedef enum
{
    DEVICE_ID_DI,
    DEVICE_ID_DO,
    DEVICE_ID_EEPROM,
    DEVICE_ID_FLASH,
    DEVICE_ID_NET,
    DEVICE_ID_ADC,
    DEVICE_ID_CAN1,
    DEVICE_ID_CAN2,
    DEVICE_ID_RTC,
    DEVICE_ID_EXTWDG,
    DEVICE_ID_LCD,
    DEVICE_ID_BUZZER,
    DEVICE_ID_KEY,   
    DEVICE_ID_RS485_1,
    DEVICE_ID_RS485_2,
    DEVICE_ID_RS485_3,
    DEVICE_ID_RS485_4,
    DEVICE_ID_RS485_5,
    DEVICE_ID_RS232_6,//USART6用以配置WIFI
    DEVICE_ID_WIFI,
    DEVICE_ID_ADS1247, //wujing 2013.02.23 add
    DEVICE_ID_ATT7022B,
    DEVICE_ID_EXT_INTERRUPT,
    
//    DEVICE_ID_NUM,
}DEVICE_ID;


/* 引脚电平状态 */
typedef enum
{
    IO_NULL,
    IO_LOW,
    IO_HIGH,
}IO_STATUS;

/* GPIO功能类型 */
typedef enum
{
    PIN_GPIO_UNCONCERN,
    PIN_GPIO_SDA,
    PIN_GPIO_SCK,
    PIN_GPIO_LCD_LED,
    PIN_GPIO_RESET,
    PIN_GPIO_485_EN,
    PIN_GPIO_LCD_DATA,
    PIN_GPIO_CS,
    PIN_GPIO_RD,
    PIN_GPIO_WR,
    PIN_GPIO_MODE,
    PIN_GPIO_MISO,//wujing 2013.02.23 add for ads1247
    PIN_GPIO_MOSI,
    PIN_GPIO_START,
    PIN_GPIO_DRDY,
    
    /* 同类型管脚定义必须连续 */
    PIN_GPIO_KEY1,
    PIN_GPIO_KEY2,
    PIN_GPIO_KEY3,
    PIN_GPIO_KEY4,
    PIN_GPIO_KEY5,
    PIN_GPIO_KEY6,
    PIN_GPIO_KEY7,
    PIN_GPIO_KEY8,
    
    PIN_GPIO_DI1,
    PIN_GPIO_DI2,
    PIN_GPIO_DI3,
    PIN_GPIO_DI4,
    PIN_GPIO_DI5,
    PIN_GPIO_DI6,
    PIN_GPIO_DI7,
    PIN_GPIO_DI8,
    PIN_GPIO_DI9,
    PIN_GPIO_DI10,
    PIN_GPIO_DI11,
    PIN_GPIO_DI12,
    PIN_GPIO_DI13,
    PIN_GPIO_DI14,
    PIN_GPIO_DO1,
    PIN_GPIO_DO2,
    PIN_GPIO_DO3,
    PIN_GPIO_DO4,
    PIN_GPIO_DO5,
    PIN_GPIO_DO6,
    PIN_GPIO_DO7,
    PIN_GPIO_DO8,
    PIN_GPIO_DO9,//wujing add 2013.02.25
    PIN_GPIO_DO10,
    
    PIN_GPIO_BUZZER,
    PIN_GPIO_EXTWDG_FEED,
    
}PIN_TYPE;


/* 总线定义 */
typedef enum
{
    PERIPH_AHB1,
    PERIPH_AHB2,
    PERIPH_APB1,
    PERIPH_APB2,
}PERIPH_BUS;


/* IO端口定义 */
typedef struct 
{
    DEVICE_ID  device_id:6;  // 所属设备ID号
    IO_STATUS  sinit:2;      // 端口初始化状态(0-无动作,1-低电平,2-高电平)
    PIN_TYPE   pin_type;     // 引脚功能类型
    uint8_t    af_periph;    // 复用端口号
    uint8_t    af_pinsource; // 复用管脚号
    GPIO_TypeDef *Base;      // 端口基地址
    GPIO_InitTypeDef io;     // IO口基本定义
}TGPIOA;

/* IO口操作结构 */
typedef struct
{
    GPIO_TypeDef *port;
    uint32_t pin;
}GPIO_CTL;

/* 设备操作结构 */
typedef struct
{
    DEVICE_ID  device_id;    // 设备ID号
    PERIPH_BUS periph_bus;    // 控制器所在总线编号
    uint32_t   periph_rccid;  // 控制器时钟编号
    void*      periph_addr;   // 控制器地址
}Device_cfg;

extern Device_cfg const device_cfg[];


//此三个函数为向驱动暴露的接口
GPIO_CTL get_gpio_ctl_attr(uint8_t device_id, PIN_TYPE pin_type);
void*    device_rcc_config(uint8_t device_id, bool action);
void     device_gpio_config(uint8_t device_id, bool is_init);


#endif
