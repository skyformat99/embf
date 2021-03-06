/******************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
 * File name      : adc.c
 * Author         : Liulei
 *                  Linfei
 * Date           : 2012.08.14
 * Description    : STM32F4xx系列处理器的ADC模块函数的头文件，其中定义了ADC1请求
 *                  DMA操作时的DMA外设地址，ADC采样时间配置相关结构及宏
 * Others         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2012.08.14 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 ******************************************************************************/
#ifndef ADC_H_
#define ADC_H_


#include "stdint.h"
#include "string.h"
#include "stm32f4xx.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_rcc.h"
#include "io_interface.h"




/* 有效采样阀值，根据ADC的有效位数设定，此ADC驱动限定为12位分辨率 */
#define MIN_ERR_SAMPLE 0xFFFU

/* ADC采样时间配置命令，作为adc1_ioctl的cmd输入参数 */
#define ADC_SPEED_CFG  1U

/* 通道采样时间配置结构，作为adc1_ioctl的arg输入参数 */
typedef struct CH_CFG
{
    uint8_t  ch_id; /* 要配置的通道编号，合法取值范围为0~15 */
    uint8_t  sample_time; /* 要配置的通道的采样时间，合法取值为ADC_SampleTime系列宏 */
} Channel_cfg;

/* ADC_SampleTime系列宏，表示ADC通道的采样时间，作为Channel_cfg中sample_time的合法取值
 * 例如：ADC_SampleTime_3T中的3T表示3个ADC时钟周期;
 * 单次数据采样的总转换时间Tconv=采样时间+12个ADC时钟周期.
 * 例: Tconv=3+12=15个ADC时钟周期。若此时ADCCLK为15MHZ，则Tconv=（1/15MHZ）*15=1us。*/
#define ADC_SampleTime_3T                     ((uint8_t)0x00U)
#define ADC_SampleTime_15T                    ((uint8_t)0x01U)
#define ADC_SampleTime_28T                    ((uint8_t)0x02U)
#define ADC_SampleTime_56T                    ((uint8_t)0x03U)
#define ADC_SampleTime_84T                    ((uint8_t)0x04U)
#define ADC_SampleTime_112T                   ((uint8_t)0x05U)
#define ADC_SampleTime_144T                   ((uint8_t)0x06U)
#define ADC_SampleTime_480T                   ((uint8_t)0x07U)



#ifndef  DSUCCESS
 #define  DSUCCESS                      0           /* 操作成功 */
#endif

#ifndef  OPFAULT
 #define  OPFAULT                       (-1)        /* 操作出错 */
#endif

#ifndef DEVBUSY
 #define DEVBUSY                        (-2)        /* 忙 */
#endif

/* ADC3驱动接口 */
int32_t adc3_open(int32_t flags, int32_t adchannel);
int32_t adc3_release(void);
int32_t adc3_read(uint8_t buf[], uint32_t count);
int32_t adc3_ioctl(uint32_t cmd, void *arg);



#endif
