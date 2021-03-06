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

volatile uint16_t counter_di[DI_MAX_NUM];    /*该数组用于判断软件去抖时间是否足够*/

/* 用一个32位数表示DI状态，最多可表示32个DI，
*  每位表示IO口的状态，所有状态的更改都经过
*  1S的软件去抖
*/
volatile uint32_t di_state = 0U;
uint8_t di_num = 0;
uint8_t do_num = 0;


/*dido_open所用端口和管脚定义*/
GPIO_CTL di_port[DI_MAX_NUM]; 
GPIO_CTL do_port[DO_MAX_NUM];

/********************************************************************************
 * Function       : dido_timer
 * Author         : wujing
 * Date           : 2012.09.20
 * Description    : 判断di_state的状态，在毫秒定时器中使用，di_state的状态改变需
 *                  经过软件去抖，时间为1000ms,用DI_TIME_DELAY来定义.
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
    if(dido_opened == true)
    {
        for(i=0U ; i< di_num; ++i)
        {
            ret = GPIO_ReadInputDataBit(di_port[i].port,di_port[i].pin); /*轮询读各个IO口*/
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
        /* 从GPIO配置表中检索DI信息 */
        for(i = 0; i < DI_MAX_NUM; i++)
        {
            di_port[i] = get_gpio_ctl_attr(DEVICE_ID_DI, (PIN_TYPE)(PIN_GPIO_DI1 + i));
            if(di_port[i].port != 0)
            {
                di_num++;
                dido_opened = true;
            }
            else
            {
                break;
            }
        }
        
        for(i = 0; i < DO_MAX_NUM; i++)
        {
            do_port[i] = get_gpio_ctl_attr(DEVICE_ID_DO, (PIN_TYPE)(PIN_GPIO_DO1 + i));
            if(do_port[i].port != 0)
            {
                do_num++;
                dido_opened = true;
            }
            else
            {
                break;
            }
        }
        device_gpio_config(DEVICE_ID_DI, TRUE);
        device_gpio_config(DEVICE_ID_DO, TRUE);
        
        uint32_t tmp;
        for(i = 0U; i< di_num; i++)                                      /*初始化di_state，实时读IO状态*/
        {
            tmp = GPIO_ReadInputDataBit(di_port[i].port,di_port[i].pin); /*轮询读各个IO口*/
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
 *                                DO_STATE_GET 为获取输出脚电平状态。
 * Calls          : GPIO_SetBits() GPIO_ResetBits()GPIO_ReadOutputDataBit()
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS: DIDO输出配置成功
 *                  OPFAULT ：DIDO输出配置失败
 *                  0       : DO输出为0
 *                  1       : DO输出为1
 *********************************************************************************/
int32_t dido_ioctl(uint32_t op, void *arg)
{
    arg = arg;
    int32_t ret = OPFAULT;
    
    uint32_t cmd    = op & 0xffffff00U;
    uint32_t do_id  = op & 0x000000ffU;
    
    if((dido_opened) && (do_id < do_num))
    {
        if(DO_STATE_HIGH == cmd)
        {
            GPIO_SetBits(do_port[do_id].port,do_port[do_id].pin);
            ret = DSUCCESS;
        }
        else if(DO_STATE_LOW == cmd)
        {
            GPIO_ResetBits(do_port[do_id].port,do_port[do_id].pin);
            ret = DSUCCESS;
        }
        if(DO_STATE_GET == cmd)
        {
           ret = GPIO_ReadOutputDataBit(do_port[do_id].port,do_port[do_id].pin);
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
    int32_t ret;
    if(false == dido_opened)
    {
        ret = DSUCCESS;
    }
    else
    {
        di_num = 0;
        do_num = 0;
        device_gpio_config(DEVICE_ID_DI, FALSE);
        device_gpio_config(DEVICE_ID_DO, FALSE);
        dido_opened = false;
        ret = DSUCCESS;
    }
    return ret;
}




