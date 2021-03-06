/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :ads1247.h
 * Author         :Linfei
 * Date           :2012.08.22
 * Description    :ADS1247采样芯片驱动，模拟SPI方式，适用于STM32F4x系列芯片，定义
 *                 模拟SPI通信管脚，以及操作接口声明，ADS1247默认配置：采样值放大
 *                 倍数PGA = 1、Vref = 2.048V、采样速率=5SPS.（PGA越大，速率越小
 *                 则采样精度越高，但是PGA越大，测量范围=(-Vref/PGA ~ +Vref/PGA)
 *                 越小，实际范围比该公式还要小）。默认设置可以用于PT100、PT1000，
 *                 默认已开启1000uA电流源，且开启两组通道扫描采样:(AIN0+, AIN1-)
 *                 和(AIN2+, AIN3-).调用open后需经过205ms才可读取到通道1的正确采样
 *                 值，再经过205ms可以读取到通道2的正确采样值，在此之前read读取的
 *                 采样值为0.
 *                 采样值转换公式：(采样值 / 8388608 / PGA) * Vref. (单位：V)
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 ********************************************************************************/

#ifndef ADS1247_H_
#define ADS1247_H_
#include "stm32f4xx.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_gpio.h"

/* 通道采样次数设定, 3 <= ADS1247_ADC_TIMES <= 255，该值越大，采样响应速度越慢 */
#define ADS1247_ADC_TIMES     5U

/* 驱动用，定时调用函数，调用周期(1ms)，建议不要放在定时器硬件中断中
 * 而是采用在硬件中断中设置标志位的方式，在外部检测该标志位并调用，
 * 调用后清除该标志位 */
#define ADS1247_PERIOD_PROC  (ads1247_set_flag())



/* ADS1247 引脚配置 --------------------------------------------------------------------- */
#define ADS1247_SPI_PORT                 (SPI1)
#define ADS1247_SPI_AF_PORT              (GPIO_AF_SPI1)

#define ADS1247_NSS_GROUP                (GPIOE)
#define ADS1247_NSS_Pin                  (GPIO_Pin_3)

#define ADS1247_MISO_GROUP               (GPIOE)
#define ADS1247_MISO_Pin                 (GPIO_Pin_6)

#define ADS1247_MOSI_GROUP               (GPIOE)
#define ADS1247_MOSI_Pin                 (GPIO_Pin_5)

#define ADS1247_SCK_GROUP                (GPIOE)
#define ADS1247_SCK_Pin                  (GPIO_Pin_4)

#define ADS1247_RESET_GROUP              (GPIOF)
#define ADS1247_RESET_Pin                (GPIO_Pin_2)

#define ADS1247_START_GROUP              (GPIOF)
#define ADS1247_START_Pin                (GPIO_Pin_1)

#define ADS1247_DRDY_GROUP               (GPIOF)
#define ADS1247_DRDY_Pin                 (GPIO_Pin_0)
#define ADS1247_DRDY_EXTI_PORT           (EXTI_PortSourceGPIOF)
#define ADS1247_DRDY_EXTI_SRC            (EXTI_PinSource0)
#define ADS1247_DRDY_EXTI_LINE           (EXTI_Line0)
#define ADS1247_DRDY_EXTI_IRQ            (EXTI0_IRQn)




#define ADS1247_SPI_CS_LOW             (GPIO_ResetBits(ADS1247_NSS_GROUP, ADS1247_NSS_Pin))
#define ADS1247_SPI_CS_HIGH            (GPIO_SetBits(ADS1247_NSS_GROUP, ADS1247_NSS_Pin))

#define ADS1247_SPI_SCK_LOW            (GPIO_ResetBits(ADS1247_SCK_GROUP, ADS1247_SCK_Pin))
#define ADS1247_SPI_SCK_HIGH           (GPIO_SetBits(ADS1247_SCK_GROUP, ADS1247_SCK_Pin))

#define ADS1247_SPI_MOSI_LOW           (GPIO_ResetBits(ADS1247_MOSI_GROUP, ADS1247_MOSI_Pin))
#define ADS1247_SPI_MOSI_HIGH          (GPIO_SetBits(ADS1247_MOSI_GROUP, ADS1247_MOSI_Pin))

#define ADS1247_RESET_LOW              (GPIO_ResetBits(ADS1247_RESET_GROUP, ADS1247_RESET_Pin))
#define ADS1247_RESET_HIGH             (GPIO_SetBits(ADS1247_RESET_GROUP, ADS1247_RESET_Pin))

#define ADS1247_START_LOW              (GPIO_ResetBits(ADS1247_START_GROUP, ADS1247_START_Pin))
#define ADS1247_START_HIGH             (GPIO_SetBits(ADS1247_START_GROUP, ADS1247_START_Pin))

#define ADS1247_SPI_MISO_STATUS        (GPIO_ReadInputDataBit(ADS1247_MISO_GROUP, ADS1247_MISO_Pin))
#define ADS1247_DRDY_STATUS            (GPIO_ReadInputDataBit(ADS1247_DRDY_GROUP, ADS1247_DRDY_Pin))



/* ioctl ----------------------------------------------------------------------------- */

/* 恒流源大小设定 ------------------------------------------------------- */
#define ADS1247_I_OFF                0x0A00U   /* ioctl操作码 恒流源关 */
#define ADS1247_I_50uA               0x0A01U   /* ioctl操作码 恒流源50uA */
#define ADS1247_I_100uA              0x0A02U   /* ioctl操作码 恒流源100uA */
#define ADS1247_I_250uA              0x0A03U   /* ioctl操作码 恒流源250uA */
#define ADS1247_I_500uA              0x0A04U   /* ioctl操作码 恒流源500uA */
#define ADS1247_I_750uA              0x0A05U   /* ioctl操作码 恒流源750uA */
#define ADS1247_I_1000uA             0x0A06U   /* ioctl操作码 恒流源1000uA */
#define ADS1247_I_1500uA             0x0A07U   /* ioctl操作码 恒流源1500uA */
/* ---------------------------------------------------------------------- */

/* 恒流源接入到模拟采样输入脚配置 ------------------------------------------- */
#define ADS1247_I12CONNECT_SET       0x0B00U   /* ioctl操作码 | 子参数1 | 子参数2 */
#define ADS1247_I1_AIN0              0x0000U   /* 子参数1 I1接入到AIN0 */
#define ADS1247_I1_AIN1              0x0010U   /* 子参数1 I1接入到AIN1 */
#define ADS1247_I1_AIN2              0x0020U   /* 子参数1 I1接入到AIN2 */
#define ADS1247_I1_AIN3              0x0030U   /* 子参数1 I1接入到AIN3 */
#define ADS1247_I1_DISCON            0x00F0U   /* 子参数1 I1不接入到模拟采样口 */

#define ADS1247_I2_AIN0              0x0000U   /* 子参数2 I2接入到AIN0 */
#define ADS1247_I2_AIN1              0x0001U   /* 子参数2 I2接入到AIN1 */
#define ADS1247_I2_AIN2              0x0002U   /* 子参数2 I2接入到AIN2 */
#define ADS1247_I2_AIN3              0x0003U   /* 子参数2 I2接入到AIN3 */
#define ADS1247_I2_DISCON            0x000FU   /* 子参数2 I2不接入到模拟采样口 */
/* --------------------------------------------------------------------------- */

/* 偏置电压设置
 * ADS1247_VBIAS_SET | 通道
 * 通道按位设，ADS1247一共四位，例如开启0和2的偏置电压：ADS1247_VBIAS_SET | 0x05 */
#define ADS1247_VBIAS_SET            0x0100U   /* ioctl操作码 */

/* 通道选择 ------------------------------------------------------------- */
#define ADS1247_CH_SELECT01          0x0001U   /* ioctl操作码 AIN0+ AIN1- */
#define ADS1247_CH_SELECT23          0x0013U   /* ioctl操作码 AIN2+ AIN3- */
/* ---------------------------------------------------------------------- */

/* 参考电压设置 --------------------------------------------------------- */
#define ADS1247_EXT_VREF             0x0200U   /* ioctl操作码 使用外部参考电压 */
#define ADS1247_INT_VREF             0x0230U   /* ioctl操作码 使用内部参考电压:2.048V */
/* ---------------------------------------------------------------------- */

#ifndef  DSUCCESS
 #define  DSUCCESS                      0           /* 操作成功 */
#endif

#ifndef  OPFAULT
 #define  OPFAULT                       (-1)        /* 操作出错 */
#endif

#ifndef DEVBUSY
 #define DEVBUSY                        (-2)        /* 忙 */
#endif

/* 周期读取采样值，需周期调用，与ADS1247_PERIOD_PROC关联 */
void ads1247_read_period_proc(void);

void ads1247_set_flag(void);

/* ADS1247的DRDY外部中断，需放入对应的EXTI中断函数中 */
void ADS1247_DRDY_EXTI_IRQHandler(void);
int32_t ads1247_open(int32_t flag, int32_t mode);
int32_t ads1247_read(uint8_t buf[], uint32_t count);
int32_t ads1247_ioctl(uint32_t op, void *arg);
int32_t ads1247_close(void);
#endif
