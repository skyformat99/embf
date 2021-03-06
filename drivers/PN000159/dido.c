/******************************************************************************
 * Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD.
 * File name      :dido.c
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
#include "dido.h"



bool dido_opened = false;         /* DIDO端口打开标志 */

volatile uint16_t counter_di[DI_NUM];    /*该数组用于判断软件去抖时间是否足够*/

/* 用一个32位数表示DI状态，最多可表示32个DI，
*  每位表示IO口的状态，所有状态的更改都经过
*  1S的软件去抖
*/
volatile uint32_t di_state = 0U;  

/*DIDO端口与引脚定义*/
typedef struct
{
    GPIO_TypeDef *Base;     /* 端口基地址  */
    GPIO_InitTypeDef io;    /* IO口基本定义*/
    uint8_t sinit;    // 端口初始化状态(0-无动作,1-低电平,2-高电平)
} TDIDO;

/*dido_open所用端口和管脚定义*/
TDIDO di_port[] = 
{
    {.Base = GPIOD, {.GPIO_Pin = GPIO_Pin_7,  .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},        // DIG_IN1-主板数字输入
    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_11, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},        // DIG_IN2
    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_12, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},        // DIG_IN3
    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_13, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},        // DIG_IN4
    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_15, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},        // DIG_IN5
    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_0,  .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},        // DIG_IN6
    
    {.Base = GPIOF, {.GPIO_Pin = GPIO_Pin_11, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN1_C-扩展板数字输入
    {.Base = GPIOF, {.GPIO_Pin = GPIO_Pin_13, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN2_C
    {.Base = GPIOF, {.GPIO_Pin = GPIO_Pin_15, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN3_C
    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_1,  .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN4_C
    {.Base = GPIOB, {.GPIO_Pin = GPIO_Pin_1,  .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN5_C
    {.Base = GPIOF, {.GPIO_Pin = GPIO_Pin_12, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN6_C
    {.Base = GPIOF, {.GPIO_Pin = GPIO_Pin_14, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN7_C
    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_0,  .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},         // DIG_IN8_C
};
TDIDO do_port[] =
{
    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_12, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // DIG_OUT1-预留第1路继电器输出
    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_13, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // DIG_OUT2-预留第2路继电器输出
    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_14, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // DIG_OUT3-预留第3路继电器输出
	{.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_1,  .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // DIG_OUT4
	{.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_2,  .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // DIG_OUT5
    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_7,  .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // FAN1_OUT-控制风扇继电器输出
    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_8,  .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // FAN2_OUT-控制风扇继电器输出
    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_9,  .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},     // FAN3_OUT-控制风扇继电器输出
    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_14, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 2},     // DO_P3	光耦输出
};

/********************************************************************************
 * Function       : dido_timer
 * Author         : wujing
 * Date           : 2012.09.20
 * Description    : 判断di_state的状态，在毫秒定时器中使用，di_state的状态改变需
 *                  经过软件去抖，时间为50ms,用DI_TIME_DELAY来定义.
 * Calls          : GPIO_ReadInputDataBit() 
 * Input          : None
 * Output         : None
 * Return         : None 
 *********************************************************************************/
void dido_timer(void)
{
    uint32_t ret;
    uint32_t tmp;
    uint8_t i;
    if(dido_opened)
    {
        for(i=0U ; i<DI_NUM ; ++i)
        {
            ret = GPIO_ReadInputDataBit(di_port[i].Base,di_port[i].io.GPIO_Pin); /*轮询读各个IO口*/
            tmp = di_state&(1U<<i);          /*提取di_state相应位*/   
            if((tmp^(ret<<i))!= 0U)           /*此次di_state的值与上次不一致*/
            {
                if(counter_di[i] == 0U)
                {    
                    counter_di[i] = 1U;         /*开始计数*/
                }
                if(counter_di[i] > DI_TIME_DELAY)
                {
                    di_state ^= 1U<<i;        /*对di_state相应位赋新值-用取反实现*/
                    counter_di[i] = 0U;        /*对计数值清零*/
                }
            }
            if ((counter_di[i] > 0U) && (counter_di[i] < (DI_TIME_DELAY + 2U)))
            {
                ++counter_di[i];
            }
        }
    }
}

/********************************************************************************
 * Function       : dido_open
 * Author         : wujing
 * Date           : 2012.09.20
 * Description    : 打开DIDO,初始化DIDO所用IO端口、初始化di_state为实时IO口状态
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS: DIDO端口打开 
 *********************************************************************************/
int32_t dido_open(int32_t flag, int32_t mode)
{
    uint8_t i;
    int32_t  ret;
    if(dido_opened)
    {
        ret = DSUCCESS;
    }
    else
    {
        uint32_t tmp = sizeof(di_port)/sizeof(TDIDO);                          /*初始化DI所用IO口*/
        for(i = 0U ; i<tmp ; ++i)
        {
            GPIO_Init(di_port[i].Base,(GPIO_InitTypeDef*)&di_port[i].io);
        }
        tmp = sizeof(do_port)/sizeof(TDIDO);                                   /*初始化DO所用IO口*/
        for(i = 0U ; i<tmp ; ++i)
        {
            GPIO_Init(do_port[i].Base,(GPIO_InitTypeDef*)&do_port[i].io);
        }
        for(i = 0U ; i<DI_NUM ; i++)                                      /*初始化di_state，实时读IO状态*/
        {
            tmp = GPIO_ReadInputDataBit(di_port[i].Base,di_port[i].io.GPIO_Pin); /*轮询读各个IO口*/
            di_state |= tmp<<i;                                       /*对di_state进行赋值*/
        }
        
        dido_opened = true;
        ret = DSUCCESS;       
    }
    return ret;
}


/********************************************************************************
 * Function       : dido_read
 * Author         : wujing
 * Date           : 2012.09.20
 * Description    : 读DI的状态，通过DI实现，读di_state的状态至buf
 * Calls          : memcpy()
 * Input          : None
 * Output         : None
 * Return         : id:       DI读取成功，返回读取状态信息字节数
 *                : OPFAULT : DI读取失败                   
 *********************************************************************************/
int32_t dido_read(uint8_t buf[], uint32_t count)  /*read是通过DI实现*/
{   
    int32_t ret = OPFAULT;
	uint8_t i = 0;											//将每位提取出来存入数组	
    if((dido_opened)&&(count<=32))   /*DI最大支持32个*/
    {
        uint32_t tmp=di_state;
		for(i=0;i<count;i++)
		{
			if(0U!=(di_state&(1U<<i)))
			{
				buf[i] = 1U;
			}
			else
			{
				buf[i] = 0U;
			}	
		}
        ret = (int32_t)count;
    }
    return ret;
}

/********************************************************************************
 * Function       : dido_ioctl
 * Author         : wujing
 * Date           : 2012.09.20
 * Description    : 控制DO的输出，DO_STATE_HIGH为输出高电平，
 *                                DO_STATE_LOW 为输出低电平，  
 * Calls          : GPIO_SetBits() GPIO_ResetBits()
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS: DIDO输出配置成功
 *                  OPFAULT ：DIDO输出配置失败
 *********************************************************************************/
int32_t dido_ioctl(uint32_t op, void *arg)
{
    arg = arg;
    int32_t ret = OPFAULT;
    
    uint32_t cmd    = op & 0xffffff00U;
    uint32_t do_id  = op & 0x000000ffU;
    
    if((dido_opened) && (do_id < DO_NUM))
    {
        if(DO_STATE_HIGH == cmd)
        {
            GPIO_SetBits(do_port[do_id].Base,do_port[do_id].io.GPIO_Pin);
            ret = DSUCCESS;
        }
        else if(DO_STATE_LOW == cmd)
        {
            GPIO_ResetBits(do_port[do_id].Base,do_port[do_id].io.GPIO_Pin);
            ret = DSUCCESS;
        }
        else
        {
            /*do nothing for MISRA-2004*/
        }
    }
    return ret;


}
/********************************************************************************
 * Function       : dido_close
 * Author         : wujing
 * Date           : 2012.09.20
 * Description    : 关闭DIDO所用IO口,将DIDO所用管脚设置为浮空状态。
 *                  
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS：DIDO端口关闭 
 *********************************************************************************/
int32_t dido_close(void)
{
    uint8_t i;
    int32_t ret;
    if(!dido_opened)
    {
        ret = DSUCCESS;
    }
    else
    {
        uint8_t tmp = sizeof(di_port)/sizeof(TDIDO);                          /*初始化DI所用IO为悬浮输入模式  */
        for(i = 0U ; i<tmp ; ++i)
        {
            di_port[i].io.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_Init(di_port[i].Base,(GPIO_InitTypeDef*)&di_port[i].io);
        }
        tmp = sizeof(do_port)/sizeof(TDIDO);                                  /*初始化DI所用IO为悬浮输入模式  */
        for(i = 0U ; i<tmp ; ++i)
        {
            do_port[i].io.GPIO_Mode = GPIO_Mode_IN;            
            do_port[i].io.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_Init(do_port[i].Base,(GPIO_InitTypeDef*)&do_port[i].io);
        }
        
        dido_opened = false;
        ret = DSUCCESS;
    }
    return ret;
}




