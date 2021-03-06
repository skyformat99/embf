/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :extwdg.c
 * Author         :Linfei
 * Date           :2012.08.30
 * Description    :外部看门狗TPS3823驱动，喂狗脉冲2us，超时复位时间固定为1.6s
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2012.08.30 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 ********************************************************************************/
#include <stdbool.h>
#include "extwdg.h"

bool extwdg_opened = false;

GPIO_CTL EXTWDG_FEED;


/* 内部函数声明 */
void extwdg_delay(uint32_t nCount);



/*******************************************************************************
* Function Name  : Delay
* Description    : Inserts a delay time.
* Input          : nCount: specifies the delay time length.
* Output         : None
* Return         : None
*******************************************************************************/
void extwdg_delay(uint32_t nCount)
{
    while(nCount > 0U)
    {
    	nCount--;
    }
}


/*******************************************************************************
 * Function       : extwdg_open
 * Author         : Linfei
 * Date           : 2012.08.30
 * Description    : 打开外部看门狗
 * Calls          : None
 * Input          : 无具体意义，可都为0
 * Output         : None
 * Return         : DSUCCESS：打开成功 
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.30 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
int32_t extwdg_open(int32_t flag, int32_t mode)
{
    int32_t ret = OPFAULT;
    if(extwdg_opened != true)
    {
        device_gpio_config(DEVICE_ID_EXTWDG, TRUE);
        EXTWDG_FEED = get_gpio_ctl_attr(DEVICE_ID_EXTWDG, PIN_GPIO_EXTWDG_FEED);

        GPIO_ResetBits(EXTWDG_FEED.port, EXTWDG_FEED.pin);
        
        extwdg_opened = true;
        ret = DSUCCESS;
    }
    return ret;
}



/*******************************************************************************
 * Function       : extwdg_ioctl
 * Author         : Linfei
 * Date           : 2012.08.30
 * Description    : 喂狗
 * Calls          : None
 * Input          : op：操作码：喂狗
 *                  arg：未用
 * Output         : None
 * Return         : DSUCCESS: 设置成功；EFAULT：设置失败
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.30 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
int32_t extwdg_ioctl(uint32_t op, void *arg)
{
    int32_t ret = OPFAULT;
    arg = arg;
    if((extwdg_opened == true) && (op == FEED_EXTWDG))
    {
        GPIO_SetBits(EXTWDG_FEED.port, EXTWDG_FEED.pin);
        extwdg_delay(80U);//未开指令预取：7.8us；开指令预取：2.8us
        GPIO_ResetBits(EXTWDG_FEED.port, EXTWDG_FEED.pin);
        extwdg_delay(80U);
        ret = DSUCCESS;
    }
    return ret;
}

/*******************************************************************************
 * Function       : extwdg_close
 * Author         : Linfei
 * Date           : 2012.08.30
 * Description    : 关闭外部看门狗
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS：关闭成功 
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.08.30 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
int32_t extwdg_close(void)
{
    if (extwdg_opened == true)
    {
        device_gpio_config(DEVICE_ID_EXTWDG, FALSE); 
        extwdg_opened = false;
    }
    return DSUCCESS;    
}

