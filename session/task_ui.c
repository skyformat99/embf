/****************************************************************************** 
* Copyright (C), 1997-2010, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :main.c
* Author         :llemmx
* Date           :2012-09-29
* Description    :UI进程
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include "global.h"
#include "config.h"
#include "message.h"
#include "lcd_module.h"
#include "assage.h"
#include "utils.h"
#include "menu.h"
#include "key.h"
#include "lm240160.h"


#define BACKLIGHT_COUNT 20 //背光时间(s)


t_task_ui  m_task_ui;
TLCDModule m_lcd;
TProcBuf m_MsgQ;      //message queueu

//button analyst
bool key_press(uint32_t state, uint8_t i)
{
    bool ret;
    if(((state&(1U << (i * 4U))) != 0U)&&((state&(2U << (i * 4U))) == 0U))//wujing 2012.11.3 add to distinguish state press from state long press  
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    return ret;
}
bool key_long_press(uint32_t state, uint8_t i)
{
    bool ret;
    if((state&(2U << (i * 4U))) != 0U)
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    return ret;
}
bool key_release(uint32_t state, uint8_t i)
{
    bool ret;
    if((state&(4U << (i * 4U))) != 0U)
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    return ret;
}

int find_msg(PMsg msg)
{
    int ret=0;
    uint32_t tseed;
    
    if (gt_glp.pwr_mode){
        embf_delay(1000);
        return 1;
    }
    
    if (MSG_GET_MSG(m_MsgQ,MSG_TYPE_TIMER)){
        tseed=embf_get_tick();
        if (tseed-m_task_ui.ui_time>1000){
            m_task_ui.ui_time=tseed;
            PostMessage(&m_MsgQ,MSG_TYPE_TIMER,0,0);
        }
    }
    if (MSG_GET_MSG(m_MsgQ,MSG_TYPE_BUTTON))
    {
        //按键捕获
        uint32_t ukey=0u;
        drv_read(gt_glp.fkey,&ukey,4u);
        
        if (key_release(ukey, 0))//上
        {
            if (key_press(ukey,0))
            {
                drv_ioctl(gt_glp.fkey, KEY_CLR_STATE | 0, NULL);
                PostMessage(&m_MsgQ,MSG_TYPE_BUTTON,MKEY_BTN_UP,MKEY_UP);
                
                ret=MSG_TYPE_BUTTON;
            }
            if(key_long_press(ukey, 0))
            {
                drv_ioctl(gt_glp.fkey, KEY_CLR_STATE | 0, NULL);
                PostMessage(&m_MsgQ,MSG_TYPE_BUTTON,MKEY_BTN_UP,MKEY_ESC);
                ret=MSG_TYPE_BUTTON;
            }
        }
        
        if (key_release(ukey, 1))//下
        {
            if (key_press(ukey,1))
            {
                drv_ioctl(gt_glp.fkey, KEY_CLR_STATE | 1, NULL);
                PostMessage(&m_MsgQ,MSG_TYPE_BUTTON,MKEY_BTN_UP,MKEY_RIGHT);
                ret=MSG_TYPE_BUTTON;
            }
            
            if(key_long_press(ukey, 1))
            {
                drv_ioctl(gt_glp.fkey, KEY_CLR_STATE | 1, NULL);
                PostMessage(&m_MsgQ,MSG_TYPE_BUTTON,MKEY_BTN_UP,MKEY_ENTER);
                ret=MSG_TYPE_BUTTON;
            }
        }
        
        if (ret)
        {
            //按键背光
            drv_ioctl(gt_glp.flcd,LM_LED_ON,NULL);
            m_task_ui.back_light_count=BACKLIGHT_COUNT;
            return ret;
        }
    }

    embf_delay(4);


    return 1;
}

void task_ui(void *p_arg)
{
//	uint8_t buf1[10]={1,2,3,4,5,6,7,8,9,10};		 //任务堆栈变化测试
//	uint8_t buf2[10]= {10,11,12,13,14,15,16,17,18,19};
    LCD_open(&m_lcd);                  /* 初始化UI */
    widget_setlcd(&m_MsgQ,&m_lcd);           /* UI控件初始化 */
        
    m_lcd.setbacklight(20);            /* 设置背光 */
    m_lcd.clear();                     /* 清屏 */
    m_task_ui.back_light_count = BACKLIGHT_COUNT; /* 设置背光亮持续时间 */ 
    
    //设置消息循环和类型
    InitMsg(&m_MsgQ,find_msg);
    EnableMsgType(&m_MsgQ,MSG_TYPE_TIMER, 1);
    EnableMsgType(&m_MsgQ,MSG_TYPE_BUTTON, 1);
    
    //注册消息
    RegUIMsg(&m_MsgQ,&m_lcd);
    DoMsgProcess(&m_MsgQ,NULL, News_Exit_Program);  /* 启动UI */
}
