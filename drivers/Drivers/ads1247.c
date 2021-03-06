/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :ads1247.c
 * Author         :Linfei
 * Date           :2012.08.22
 * Description    :ADS1247采样芯片驱动，模拟SPI方式，适用于STM32F4x系列芯片，定义
 *                 采样操作函数
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 ********************************************************************************/

#include "stdio.h"
#include "string.h"
#include "ads1247.h"
#include "stdbool.h"
#include "global.h"

//wujing 2013.02.23 使用EMBF1.1.1的模式定义进行改造

GPIO_CTL  ADS1247_CTL_CS;
GPIO_CTL  ADS1247_CTL_SCK;
GPIO_CTL  ADS1247_CTL_MOSI;
GPIO_CTL  ADS1247_CTL_RESET;
GPIO_CTL  ADS1247_CTL_START;
GPIO_CTL  ADS1247_CTL_MISO;
GPIO_CTL  ADS1247_CTL_DRDY;

 #define ADS1247_SPI_CS_LOW             (GPIO_ResetBits(ADS1247_CTL_CS.port, ADS1247_CTL_CS.pin))
 #define ADS1247_SPI_CS_HIGH            (GPIO_SetBits(ADS1247_CTL_CS.port, ADS1247_CTL_CS.pin))

 #define ADS1247_SPI_SCK_LOW            (GPIO_ResetBits(ADS1247_CTL_SCK.port, ADS1247_CTL_SCK.pin))
 #define ADS1247_SPI_SCK_HIGH           (GPIO_SetBits(ADS1247_CTL_SCK.port, ADS1247_CTL_SCK.pin))

 #define ADS1247_SPI_MOSI_LOW           (GPIO_ResetBits(ADS1247_CTL_MOSI.port, ADS1247_CTL_MOSI.pin))
 #define ADS1247_SPI_MOSI_HIGH          (GPIO_SetBits(ADS1247_CTL_MOSI.port, ADS1247_CTL_MOSI.pin))

 #define ADS1247_RESET_LOW              (GPIO_ResetBits(ADS1247_CTL_RESET.port, ADS1247_CTL_RESET.pin))
 #define ADS1247_RESET_HIGH             (GPIO_SetBits(ADS1247_CTL_RESET.port, ADS1247_CTL_RESET.pin))

 #define ADS1247_START_LOW              (GPIO_ResetBits(ADS1247_CTL_START.port, ADS1247_CTL_START.pin))
 #define ADS1247_START_HIGH             (GPIO_SetBits(ADS1247_CTL_START.port, ADS1247_CTL_START.pin))

 #define ADS1247_SPI_MISO_STATUS        (GPIO_ReadInputDataBit(ADS1247_CTL_MISO.port, ADS1247_CTL_MISO.pin))
 #define ADS1247_DRDY_STATUS            (GPIO_ReadInputDataBit(ADS1247_CTL_DRDY.port, ADS1247_CTL_DRDY.pin))
 

/* ADS1247操作命令 */
#define ADS1247_WAKEUP        0x00U
#define ADS1247_SLEEP         0x02U
#define ADS1247_SYNC          0x04U
#define ADS1247_RESET         0x06U
#define ADS1247_NOP           0xFFU
#define ADS1247_RDATA         0x12U
#define ADS1247_RDATAC        0x14U
#define ADS1247_SDATAC        0x16U
#define ADS1247_RREG          0x20U
#define ADS1247_WREG          0x40U
#define ADS1247_SYSOCAL       0x60U
#define ADS1247_SYSGCAL       0x61U
#define ADS1247_SELFOCAL      0x62U

/* ADS1247寄存器地址 */
#define ADS1247_MUX0          0x00U
#define ADS1247_VBIAS         0x01U
#define ADS1247_MUX1          0x02U
#define ADS1247_SYS0          0x03U
#define ADS1247_OFC0          0x04U
#define ADS1247_OFC1          0x05U
#define ADS1247_OFC2          0x06U
#define ADS1247_FSC0          0x07U
#define ADS1247_FSC1          0x08U
#define ADS1247_FSC2          0x09U
#define ADS1247_IDAC0         0x0AU
#define ADS1247_IDAC1         0x0BU
#define ADS1247_GPIOCFG       0x0CU
#define ADS1247_GPIODR        0x0DU
#define ADS1247_GPIODAT       0x0EU



/* 采样信号放大增益，增益越大，采样信号值越精确 */
#define ADS1247_PGA_1                0x0000U
#define ADS1247_PGA_2                0x0010U
#define ADS1247_PGA_4                0x0020U
#define ADS1247_PGA_8                0x0030U
#define ADS1247_PGA_16               0x0040U
#define ADS1247_PGA_32               0x0050U
#define ADS1247_PGA_64               0x0060U
#define ADS1247_PGA_128              0x0070U

/* 采样速率，速率越低，采样信号值越精确 */
#define ADS1247_RATE_5SPS            0x0000U
#define ADS1247_RATE_10SPS           0x0001U
#define ADS1247_RATE_20SPS           0x0002U
#define ADS1247_RATE_40SPS           0x0003U
#define ADS1247_RATE_80SPS           0x0004U
#define ADS1247_RATE_160SPS          0x0005U
#define ADS1247_RATE_320SPS          0x0006U
#define ADS1247_RATE_640SPS          0x0007U
#define ADS1247_RATE_1KSPS           0x0008U
#define ADS1247_RATE_2KSPS           0x0009U




#define ADS1247_DELAY_COUNT   0x90U  /* 0x120对应周期为 25us(10kbps) */ 
                                     //未开指令预取：ads1247_delay(0x120)：25us;
                                     //未开指令预取：ads1247_delay(0x90)：13us;开启指令预取：ads1247_delay(500)--13us

typedef struct
{
    /* 采样值缓冲区，两个通道 */
    uint32_t adbuf[2][ADS1247_ADC_TIMES];
    uint8_t  adbuf_idx[2];  /* 指示下一个采样值存放的缓冲区下标 */
    bool     adbuf_full[2]; /* 指示缓冲区是否填满，只置位1次 */
    bool     opened;  /* ADS1247打开标识 */
    uint8_t  ad_over; /* 采样结束标志: 0: 空闲, 1: 转换中, 2: 转换结束, 3: 中止 */
}ADS1247_DEV;


bool ADS1247_AD_time_flag = false;

volatile ADS1247_DEV  ads1247_dev;


void ads1247_delay(uint32_t count);
void ADS1247_init(void);
void ADS1247_DRDY_EXTI_Config(void);
void ADS1247_Reset(void);
uint8_t ADS1247_SPI_SendByte(uint8_t byte);
uint8_t ADS1247_SPI_ReadByte(void);

/********************************************************************************
 * Function       : ads1247_delay
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : 模拟SPI通信时使用的简单延时函数
 * Calls          : None
 * Input          : count:计数
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2012.08.22: 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
void ads1247_delay(uint32_t count)
{
    while(count > 0U)
    {
    	count--;
    }
}


/*******************************************************************************
 * Function       : ADS1247_init
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : ADS1247引脚初始化，模拟SPI
 * Calls          : SPI_I2S_DeInit
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS：关闭成功 
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
void ADS1247_init(void)
{    
    device_gpio_config(DEVICE_ID_ADS1247, TRUE);//初始化IO口
    
    ADS1247_CTL_CS = get_gpio_ctl_attr(DEVICE_ID_ADS1247, PIN_GPIO_CS);//获取相应操作IO口
    ADS1247_CTL_SCK = get_gpio_ctl_attr(DEVICE_ID_ADS1247, PIN_GPIO_SCK);
    ADS1247_CTL_MOSI = get_gpio_ctl_attr(DEVICE_ID_ADS1247, PIN_GPIO_MOSI);
    ADS1247_CTL_RESET = get_gpio_ctl_attr(DEVICE_ID_ADS1247, PIN_GPIO_RESET);
    ADS1247_CTL_START = get_gpio_ctl_attr(DEVICE_ID_ADS1247, PIN_GPIO_START);
    ADS1247_CTL_MISO = get_gpio_ctl_attr(DEVICE_ID_ADS1247, PIN_GPIO_MISO);
    ADS1247_CTL_DRDY = get_gpio_ctl_attr(DEVICE_ID_ADS1247, PIN_GPIO_DRDY);
    
    ADS1247_RESET_HIGH;
    ADS1247_START_HIGH;
    ADS1247_SPI_CS_HIGH;
    ADS1247_SPI_SCK_LOW;
    ADS1247_SPI_MOSI_LOW;
}

void ADS1247_DRDY_EXTI_Config(void)
{
    EXTI_InitTypeDef   EXTI_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;

    /* Connect EXTI Line to DRDY pin */
    SYSCFG_EXTILineConfig(ADS1247_DRDY_EXTI_PORT, ADS1247_DRDY_EXTI_SRC);

    /* Configure DRDY's EXTI Line */
    EXTI_InitStructure.EXTI_Line = ADS1247_DRDY_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set EXTI Line0 Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = ADS1247_DRDY_EXTI_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0FU;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0FU;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
//注：相应驱动函数对应中断应放在各个设备.c中，否则生成Lib库后会出错
void EXTI0_IRQHandler(void)     //放到ADS1247.c中去
{
	ADS1247_DRDY_EXTI_IRQHandler();	  //wujing 2012.12.13
}
void ADS1247_DRDY_EXTI_IRQHandler(void)
{
    if(EXTI_GetITStatus(ADS1247_DRDY_EXTI_LINE) != RESET)
    {
        if(ads1247_dev.ad_over == 1U) /* 在采样中 */
        {
            ads1247_dev.ad_over = 2U; /* 检测到DRDY下降沿，采样结束 */
        }

        EXTI_ClearITPendingBit(ADS1247_DRDY_EXTI_LINE);
    }
}

/*******************************************************************************
 * Function       : ADS1247_init
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : ADS1247芯片复位
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS：关闭成功 
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
void ADS1247_Reset(void)
{
    ADS1247_RESET_LOW;
    ads1247_delay(ADS1247_DELAY_COUNT);
    ADS1247_RESET_HIGH;
    ads1247_delay(0x3333U); /* 1ms 不小于0.6ms *///0x3333:未开指令预取：1080us
                          //46956：开启指令预取：1120us             
}


/*******************************************************************************
 * Function       : ADS1247_SPI_SendByte
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : 通过SPI向ADS1247发送一个字节，SPI模式：MSB First, CPLO = LOW
 *                  CPHA = 第一个边沿采样。
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : 读取字节
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
uint8_t ADS1247_SPI_SendByte(uint8_t byte)
{
    uint8_t i;
    uint8_t rdata = 0U; /* 保存接收的数据 */
    ADS1247_SPI_SCK_LOW;
    ads1247_delay(ADS1247_DELAY_COUNT);
    for (i = 0U; i < 8U; i++)
    {
        ADS1247_SPI_SCK_HIGH;
        ads1247_delay(ADS1247_DELAY_COUNT); /* 等待数据稳定 */
        
        /* 采样MISO */
        if (ADS1247_SPI_MISO_STATUS == Bit_SET)
        {
            rdata |= 0x01U << (7U - i);
        }
        
        /* 发送 */
        if((byte & 0x80U) != 0U)
        {
            ADS1247_SPI_MOSI_HIGH;
        }
        else
        {
            ADS1247_SPI_MOSI_LOW;
        }
        ads1247_delay(ADS1247_DELAY_COUNT);
        ADS1247_SPI_SCK_LOW;
        byte = (uint8_t)(byte << 1);
        ads1247_delay(ADS1247_DELAY_COUNT);
        ads1247_delay(ADS1247_DELAY_COUNT);
    }
    ADS1247_SPI_SCK_LOW;
    ADS1247_SPI_MOSI_LOW;
    return rdata;
}

/*******************************************************************************
 * Function       : ADS1247_SPI_ReadByte
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : 通过SPI从ADS1247读取一个字节
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : 读取字节
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
uint8_t ADS1247_SPI_ReadByte(void)
{
    return (ADS1247_SPI_SendByte(ADS1247_NOP));
}



/*******************************************************************************
 * Function       : ads1247_open
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : 打开并初始化ADS1247采样芯片，默认设置：内部2.048V参考电压，
 *                  AIN0+ AIN1-，偏置电压不开启，放大增益16倍，采样速率5SPS，非
 *                  周期采样
 * Calls          : None
 * Input          : 无具体意义，可都为0
 * Output         : None
 * Return         : DSUCCESS：打开成功 
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
int32_t ads1247_open(int32_t flag, int32_t mode)
{
    int32_t ret = OPFAULT;
    if(ads1247_dev.opened != true)
    {
        uint16_t adc_times = ADS1247_ADC_TIMES;
        if((adc_times > 0U) && (adc_times <= 255U))
        {
            ads1247_delay(0x40000U); /* 初次上电延时20ms *///0x40000:未开：22ms 956521:开启：22.80ms
            ADS1247_init();  /* 管脚初始化 */
            ADS1247_Reset(); /* 复位ADS1247 */

            memset(&ads1247_dev, 0, sizeof(ads1247_dev));
            ads1247_dev.ad_over = 3U; /* 周期采样函数初始默认关闭 */
        
            /* 寄存器初始化 */
            ADS1247_SPI_CS_LOW;
            (void)ADS1247_SPI_SendByte(ADS1247_WREG | ADS1247_MUX0);
            (void)ADS1247_SPI_SendByte(0x03U);
            (void)ADS1247_SPI_SendByte(0x01U);  /* MUX0: burnout off ,AIN0+   AIN1- */
            (void)ADS1247_SPI_SendByte(0x00U);  /* VBIAS: 不开启偏置电压 */
            (void)ADS1247_SPI_SendByte(0x30U);  /* MUX1: 使用内部参考电压，常开，正常操作 */
            (void)ADS1247_SPI_SendByte(ADS1247_PGA_1 | ADS1247_RATE_5SPS);  /* SYS0: 放大增益1 采样速率 5 */
            ADS1247_SPI_CS_HIGH;
            ads1247_delay(ADS1247_DELAY_COUNT);
        
            /* 关闭ADS1247 */
            ADS1247_START_LOW;
            ads1247_delay(ADS1247_DELAY_COUNT);
            
            /* 停止周期采样 */
            ADS1247_SPI_CS_LOW;
            (void)ADS1247_SPI_SendByte(ADS1247_SDATAC);
            ADS1247_SPI_CS_HIGH;
            ads1247_delay(ADS1247_DELAY_COUNT);
            ads1247_delay(ADS1247_DELAY_COUNT);

            ads1247_dev.opened = true;

            /* 开启1mA电流源 */
            if(ads1247_ioctl(ADS1247_I_1000uA, NULL) == DSUCCESS)
            {
            	/* 电流源分别输出到AIN0和AIN2 */
            	if(ads1247_ioctl(ADS1247_I12CONNECT_SET | ADS1247_I1_AIN0 | ADS1247_I2_AIN2, NULL) == DSUCCESS)
                {
                    ADS1247_DRDY_EXTI_Config(); /* 配置DRDY外部中断，下降沿触发 */
                    ads1247_dev.ad_over = 0U;    /* 开启周期采样函数 */
                    ret = DSUCCESS;
                }
                else
                {
                    ads1247_dev.opened = false;
                }
            }
            else
            {
                ads1247_dev.opened = false;
            }
        }
    }
    return ret;
}

/* ADS1247采样周期检测标志置位函数 */
void ads1247_set_flag(void)
{
    ADS1247_AD_time_flag = true;
}


/*******************************************************************************
 * Function       : ads1247_read_period_proc
 * Author         : Linfei
 * Date           : 2012.08.29
 * Description    : 周期读取一个采样值至ADS1247采样缓冲区
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.29 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
void ads1247_read_period_proc(void)
{
    if(ADS1247_AD_time_flag == true)
    {
        if((ads1247_dev.opened == true) && (ads1247_dev.ad_over != 3U))
        {
            static uint8_t ch = 0U; /* 采样通道选择 0-AIN01   1-AIN23 */
            static uint16_t time_out = 0U; /* 超时计时 */
            /* 启动一个转换 */
            if(ads1247_dev.ad_over == 0U)
            {
                int32_t result = OPFAULT;
            	if(ch == 0U)
                {
            		result = ads1247_ioctl(ADS1247_CH_SELECT01, NULL);
                }
                else
                {
                	result = ads1247_ioctl(ADS1247_CH_SELECT23, NULL);
                }
                if(result == DSUCCESS)
                {
                    /* 必须在ioctl之后置1，目的是过滤掉ioctl引起的DRDY下降沿中断触发 */
                    ads1247_dev.ad_over = 1U;
                    time_out = 0U;
                
                    /* 启动一次采样 */
					OSSchedLock();								//wujing 2012.12.07
                    ADS1247_START_HIGH;
                    ads1247_delay(ADS1247_DELAY_COUNT);
                    ADS1247_START_LOW;
  					OSSchedUnlock();						   //wujing 2012.12.07
                }
            }
            else if(ads1247_dev.ad_over == 1U)  
            {
                time_out++;
                if(time_out > 300U)
                {
                    ads1247_dev.ad_over = 0U; /* 超时未检测到转换完成，重新开始转换 */
                }
            }
            else  /* 值为2，转换结束 */
            {
                ads1247_dev.ad_over = 0U;
                uint8_t buf[4] = { 0U };
				OSSchedLock();								//wujing 2012.12.07
                ads1247_delay(ADS1247_DELAY_COUNT);
                ADS1247_SPI_CS_LOW;
                (void)ADS1247_SPI_SendByte(ADS1247_RDATA);
                buf[3] = 0U;
                buf[2] = ADS1247_SPI_ReadByte();
                buf[1] = ADS1247_SPI_ReadByte();
                buf[0] = ADS1247_SPI_ReadByte();
                memcpy(&ads1247_dev.adbuf[ch][ads1247_dev.adbuf_idx[ch]], &buf[0], 4U);
                ADS1247_SPI_CS_HIGH;
  				OSSchedUnlock();						   //wujing 2012.12.07
                ads1247_delay(ADS1247_DELAY_COUNT);

                
                if(ads1247_dev.adbuf_idx[ch] == (ADS1247_ADC_TIMES - 1U))
                {
                    ads1247_dev.adbuf_full[ch] = true;
                }
                ads1247_dev.adbuf_idx[ch] = (ads1247_dev.adbuf_idx[ch] + 1U)%ADS1247_ADC_TIMES;
                ch = (ch + 1U)%2U; /* 切换通道 */
            }
            ADS1247_AD_time_flag = false;
        }
    }
}




/*******************************************************************************
 * Function       : ads1247_read
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : 读取采样值，每个通道采样值占4个字节，两个通道8字节，采样算法
 *                  为去除极值后求平均
 * Calls          : None
 * Input          : buf: 待读取采样数据存储地址指针；count：待读取采样数据字节长度
 * Output         : None
 * Return         : EFAULT：读取失败； 3：读取到的字节数
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
int32_t ads1247_read(uint8_t buf[], uint32_t count)
{
    int32_t ret = OPFAULT;
    if(ads1247_dev.opened == true)
    {
        if((buf != NULL) && (count >= 4U))
        {
            uint32_t tmpvalue[2] = { 0U }; /* 保存两个通道过滤后的采样值 */
            uint8_t adnum = ADS1247_ADC_TIMES;
            
            for (uint8_t chn = 0U; chn < 2U; chn++)
            {
                if(ads1247_dev.adbuf_full[chn] != true)
                {
                    adnum = ads1247_dev.adbuf_idx[chn];
                }
            
                if(adnum < 3U)
                {
                    for(uint8_t i = 0U; i < adnum; i++)
                    {
                        tmpvalue[chn] += ads1247_dev.adbuf[chn][i];
                    }
                    tmpvalue[chn] = tmpvalue[chn] / adnum;
                }
                else
                {
                    /* 过滤，去除最大最小值 */
                    uint8_t maxval = 0U;
                    uint8_t minval = 0U;
                    for(uint8_t i = 0U; i < adnum; i++)
                    {
                        if (ads1247_dev.adbuf[chn][maxval] < ads1247_dev.adbuf[chn][i])
                        {
                            maxval = i;
                        }
                        if (ads1247_dev.adbuf[chn][i] < ads1247_dev.adbuf[chn][minval])
                        {
                            minval = i;
                        }
                    }
                        
                    /* 求均值 */
                    for(uint8_t i = 0U; i < adnum; i++)
                    {
                        if((i != maxval) && (i != minval))
                        {
                            tmpvalue[chn] += ads1247_dev.adbuf[chn][i];
                        }
                    }
                    if(maxval != minval)
                    {
                        adnum -= 2;
                    }
                    else
                    {
                        adnum -= 1;
                    }
                    tmpvalue[chn] = (uint32_t)(tmpvalue[chn] / (uint32_t)adnum);
                }
            }
            
            if(count > 8U)
            {
                count = 8U;
            }
            memcpy(buf, &tmpvalue[0], count);
            ret = (int32_t)count;
        }
    }
    return ret;
}

/*******************************************************************************
 * Function       : ads1247_ioctl
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : ADS配置函数，可以设置恒流源，偏置电压，采样通道，参考电压
 * Calls          : None
 * Input          : op：写保护设置参数，取值范围见ads1247.h
 *                  arg：未用
 * Output         : None
 * Return         : DSUCCESS: 设置成功；EFAULT：设置失败
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
int32_t ads1247_ioctl(uint32_t op, void *arg)
{
    int32_t ret = OPFAULT;
    arg = arg;
    if(ads1247_dev.opened == true)
    {
        uint8_t opReg = (uint8_t)((op >> 8) & 0xFFU);
        uint8_t opValue = (uint8_t)(op & 0xFFU);
        if(opReg <= 0x0EU)
        {
            /* 开启ADS1247 */
			OSSchedLock();								//wujing 2012.12.07
            ADS1247_START_HIGH;
            ads1247_delay(ADS1247_DELAY_COUNT);
            
            ADS1247_SPI_CS_LOW;
            (void)ADS1247_SPI_SendByte(ADS1247_WREG | opReg);
            (void)ADS1247_SPI_SendByte(0x00U);
            (void)ADS1247_SPI_SendByte(opValue);
            ADS1247_SPI_CS_HIGH;
            
            /* 关闭ADS1247 */
            ADS1247_START_LOW;
            (void)ADS1247_SPI_ReadByte();
            ads1247_delay(ADS1247_DELAY_COUNT);
	  		OSSchedUnlock();						   //wujing 2012.12.07
            ret = DSUCCESS;
        }
    }
    return ret;
}
/*******************************************************************************
 * Function       : ads1247_close
 * Author         : Linfei
 * Date           : 2012.08.22
 * Description    : 关闭ADS1247
 * Calls          : SPI_I2S_DeInit
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS：关闭成功 
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.22 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
int32_t ads1247_close(void)
{
    if (ads1247_dev.opened == true)
    {
        ads1247_dev.opened = false;
        memset(&ads1247_dev, 0, sizeof(ads1247_dev));
        ads1247_dev.ad_over = 3U; /* 终止周期采样函数 */
    }
    return DSUCCESS;    
}


