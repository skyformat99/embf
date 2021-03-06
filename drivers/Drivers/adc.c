/******************************************************************************
* Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
* File name      : adc.c
* Author         : Linfei
* Date           : 2012.08.14
* Description    : STM32F4xx系列处理器的ADC模块函数，包括打开ADC、关闭ADC、
*                  读ADC数据和配置ADC采样周期
* Others         : None
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2012.08.14 : 1.0.0 : Linfei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
******************************************************************************/
#include "adc.h"
#include "stdbool.h"

/* 每个通道重复采样次数，之后计算平均值 */
#define  ADC_TRYTIMES    8U


/* ADC设备配置结构 */
typedef struct DEV_ADC
{
    bool            opened;                   /* ADC是否打开的标志 */
    uint8_t         channel_num;			  /* ADC开启的通道数目 */
    uint16_t        channel[16];              /* ADC开启的通道的编号 */
    uint32_t        dr_address;               /* ADC数据寄存器地址 */
    __IO uint16_t   ConvertedBuf[ADC_TRYTIMES << 4];	      /* ADC 16个通道采集数据的缓冲区，首地址作为所请求DMA的内存地址 */
    uint8_t         buf_len;				  /* 缓冲区长度,单位：半字 */
} Dev_adc;

/*ADC所请求DMA的外设地址 */
#define ADC3_DR_Address ((uint32_t)(&(ADC3->DR)))

/*ADC支持的通道数 */
#define CHANNEL_NUM  16U



/*ADC3设备配置结构 */
Dev_adc dev_adc3;

/*******************************************************************************
* Function       : adc3_open
* Author         : Linfei
* Date           : 2012.08.14
* Description    : 打开并初始化ADC3设备，工作模式设置为：独立模式、扫描和连续转换，
*                  转换完成后产生DMA请求，根据channel参数开启相应通道，使用软件触发
*                  方式启动数据转换
* Calls          : RCC_ADCCLKConfig, RCC_AHBPeriphClockCmd, RCC_APB2PeriphClockCmd
*                  ADC_DeInit, ADC_RegularChannelConfig, memset, DMA_Init, DMA_Cmd,
*                  malloc, DMA_DeInit, ADC_Init, ADC_DMACmd, ADC_Cmd, ADC_ResetCalibration,
*                  ADC_GetResetCalibrationStatus, ADC_StartCalibration, ADC_GetCalibrationStatus
*                  ADC_SoftwareStartConvCmd
* Table Accessed : None
* Table Updated  : None
* Input          : flags，无具体含义，可为0
*                  channel，有效位：0~15，每一位表示是否使用对应通道，如0x00000003,
*                  表示打开并使用通道0和1，通道的开启只能在adc3_open中配置一次.
* Output         : None
* Return         : DSUCCESS：开启ADC成功；OPFAULT，开启ADC失败
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2012.08.14 : 1.0.1 : Linfei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t adc3_open(int32_t flags, int32_t adchannel)
{
    ADC_InitTypeDef       ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    DMA_InitTypeDef       DMA_InitStructure;
    uint8_t ch;
    int32_t ret = OPFAULT;
    uint16_t adc_ch = (uint16_t)adchannel;

    adc_ch &= 0xFFFFU;
    if((dev_adc3.opened != true) && (adc_ch != 0U))
    {
        device_gpio_config(DEVICE_ID_ADC, TRUE);//wujing 2013.02.25 EMBF1.1.1移植
        memset(&dev_adc3, 0, sizeof(dev_adc3));
        dev_adc3.opened = true;

        /* 开启ADC3和DMA2的时钟 */
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

        /* ADC寄存器复位 */
        ADC_DeInit();

        /* ADC3通道及采样周期配置 */
        for(ch = 0U; ch < 16U; ch++)
        {
            if(((adc_ch >> ch) & 0x1U) != 0U)
            {
                dev_adc3.channel[dev_adc3.channel_num] = ch;
                dev_adc3.channel_num += 1U;
                ADC_RegularChannelConfig(ADC3, ch, dev_adc3.channel_num, ADC_SampleTime_56Cycles);
            }
        }

        /* ADC的DMA缓冲区有效长度设置 */
        dev_adc3.buf_len = (dev_adc3.channel_num) * ADC_TRYTIMES;
        dev_adc3.dr_address = ADC3_DR_Address;

        /* DMA通道配置 */
        DMA_DeInit(DMA2_Stream0);
        DMA_InitStructure.DMA_Channel = DMA_Channel_2;
        DMA_InitStructure.DMA_PeripheralBaseAddr    = dev_adc3.dr_address;
        DMA_InitStructure.DMA_Memory0BaseAddr       = (uint32_t)(dev_adc3.ConvertedBuf);
        DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralToMemory;
        DMA_InitStructure.DMA_BufferSize            = dev_adc3.buf_len;
        DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
        DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_HalfWord;
        DMA_InitStructure.DMA_Mode                  = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
        DMA_InitStructure.DMA_FIFOMode              = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold         = DMA_FIFOThreshold_HalfFull;
        DMA_InitStructure.DMA_MemoryBurst           = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst       = DMA_PeripheralBurst_Single;
        DMA_Init(DMA2_Stream0, &DMA_InitStructure);

        /* 使能DMA通道 */
        DMA_Cmd(DMA2_Stream0, ENABLE);
        
        
        /* ADC Common Init **********************************************************/
        ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
        ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;  /* ADCCLK = PCLK2 / 4 = 21MHz */
        ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
        ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
        ADC_CommonInit(&ADC_CommonInitStructure);

        /* ADC初始化配置 */
        ADC_InitStructure.ADC_Resolution            = ADC_Resolution_12b;  /* 12位分辨率 */
        ADC_InitStructure.ADC_ScanConvMode          = ENABLE;  /* 扫描模式 */
        ADC_InitStructure.ADC_ContinuousConvMode    = ENABLE;  /* 连续转换模式 */
        ADC_InitStructure.ADC_ExternalTrigConvEdge  = ADC_ExternalTrigConvEdge_None;
        ADC_InitStructure.ADC_ExternalTrigConv      = ADC_ExternalTrigConv_T1_CC1;
        ADC_InitStructure.ADC_DataAlign             = ADC_DataAlign_Right;
        ADC_InitStructure.ADC_NbrOfConversion       = dev_adc3.channel_num;
        ADC_Init(ADC3, &ADC_InitStructure);

        /* AD转换完成后DMA传输 */
        ADC_DMARequestAfterLastTransferCmd(ADC3, ENABLE);
        
        /* 使能ADC的DMA请求 */
        ADC_DMACmd(ADC3, ENABLE);

        /* 使能ADC */
        ADC_Cmd(ADC3, ENABLE);

        /* 开启ADC数据采集，软件触发方式 */
        ADC_SoftwareStartConv(ADC3);
        ret = DSUCCESS;
    }
    return ret;
}

/**********************************************************************************
* Function       : adc3_release
* Author         : Liulei
* Date           : 2011.07.18
* Description    : 关闭ADC设备
* Calls          : DMA_DeInit, ADC_DMACmd, ADC_SoftwareStartConvCmd, free
* Table Accessed : None
* Table Updated  : None
* Input          : None
* Output         : None
* Return         : DSUCCESS：ADC关闭
**********************************************************************************
* History        :
*---------------------------------------------------------------------------------
* 2011-07-18 : 1.0.1 : Linfei
* Modification   : 设备结构dev_adc3恢复默认值
*---------------------------------------------------------------------------------
* 2010-08-03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*---------------------------------------------------------------------------------
***********************************************************************************/
int32_t adc3_release(void)
{
    if(dev_adc3.opened == true)
    {
        DMA_DeInit(DMA2_Stream0);
        ADC_DMACmd(ADC3, DISABLE);
        ADC_Cmd(ADC3, DISABLE);
        memset(&dev_adc3, 0, sizeof(dev_adc3));
        dev_adc3.opened = false;
        dev_adc3.dr_address = NULL;
    }
    return DSUCCESS;
}

/********************************************************************************
* Function       : adc3_read
* Author         : Liulei
*                  Linfei
* Date           : 2011.07.18
* Description    : 读取ADC采样数据，采样数据是3次采样所得数据的均值,
*                  读取的采样值按打开的通道的编号顺序保存在buf中
* Calls          : memcpy
* Table Accessed : None
* Table Updated  : None
* Input          : buf：待读取的ADC采样信息的数据指针，保存读取的采样数据，单个采
*                  样数据占2个字节存储空间；
*                  count：待读取的采样数据的字节数，该值必须大于等于2
* Output         : None
* Return         : OPFAULT：读取失败；count：实际读取到的采集数据字节数
*********************************************************************************
* History        :
*--------------------------------------------------------------------------------
* 2011-07-18 : 1.0.1 : Linfei
* Modification   : 添加参数检查和有效数据判断处理，将保存单个采样数据的缓冲区由
*                  32位改为16位
*--------------------------------------------------------------------------------
* 2010-08-03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*---------------------------------------------------------------------------------
*********************************************************************************/
int32_t adc3_read(uint8_t buf[], uint32_t count)
{
    int32_t ret = OPFAULT;
    if((dev_adc3.opened == true) && (buf != NULL) && (count >= 2U))
    {
        uint16_t adcbuf[CHANNEL_NUM] = { 0U };
        uint16_t tmpcnt[CHANNEL_NUM] = { 0U };
        uint8_t i;

        /* 提取有效采样数据，剔除无效值 */
        for(i = 0U; i < dev_adc3.buf_len; i++)
        {
            if(dev_adc3.ConvertedBuf[i] <= MIN_ERR_SAMPLE)
            {
                adcbuf[i%dev_adc3.channel_num] += dev_adc3.ConvertedBuf[i];
                tmpcnt[i%dev_adc3.channel_num]++;
            }
        }

        /* 求采样数据均值 */
        for(i = 0U; i < dev_adc3.channel_num; i++)
        {
            if(tmpcnt[i] != 0U)
            {
                adcbuf[i] = adcbuf[i] / tmpcnt[i];
            }
        }

        if(count > ((uint32_t)dev_adc3.channel_num << 1))
        {
            count = (uint32_t)dev_adc3.channel_num << 1;
        }

        if(adcbuf != NULL)
        {
            memcpy(buf, (uint8_t *)adcbuf, count);
            ret = (int32_t)count;
        }
        else
        {
        	ret = OPFAULT;
        }
    }
    return ret;
}

/**********************************************************************************
* Function       : adc3_ioctl
* Author         : Liulei
*                  Linfei
* Date           : 2011.07.18
* Description    : 设置指定通道的采样时间
* Calls          : ADC_SoftwareStartConvCmd, ADC_RegularChannelConfig,
*                  ADC_SoftwareStartConvCmd
* Table Accessed : None
* Table Updated  : None
* Input          : cmd：控制命令，目前仅支持ADC_SPEED_CFG命令 ；arg：要配置的通道
*                  及其采样时间
* Output         : None
* Return         : DSUCCESS：配置成功 ; OPFAULT: 配置失败，cmd不是支持的控制命令或者
*                  arg不合法
**********************************************************************************
* History        :
*---------------------------------------------------------------------------------
* 2011-07-18 : 1.0.1 : Linfei
* Modification   : 修复不能正确配置采样时间的bug，添加16个通道采样时间的独立配置功能。
*---------------------------------------------------------------------------------
* 2010-08-03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*----------------------------------------------------------------------------------
***********************************************************************************/
int32_t adc3_ioctl(uint32_t cmd, void *arg)
{
    Channel_cfg *ch_cfg = (Channel_cfg *)arg;
    uint8_t seq;
    int32_t ret = OPFAULT;
    if(cmd == ADC_SPEED_CFG)
    {
        /* arg参数错误，返回OPFAULT */
        if(ch_cfg != NULL)
        {
            /* 检查要设置的通道采样转换时间是否非法 */
            if(IS_ADC_SAMPLE_TIME(ch_cfg->sample_time))
            {
                /* 判断要配置的通道是否已开启，若已开启，seq=规则通道序号，未开启则出错，返回OPFAULT */
                for(seq = 1U; seq <= dev_adc3.channel_num; seq++)
                {
                    if(dev_adc3.channel[seq-1U] == ch_cfg->ch_id)
                    {
                        break;
                    }
                }
                if(seq <= dev_adc3.channel_num)
                {
                    ADC_RegularChannelConfig(ADC3, ch_cfg->ch_id, seq, ch_cfg->sample_time);
                    ret = DSUCCESS;
                }
            }
        }
    }
    return ret;
}
