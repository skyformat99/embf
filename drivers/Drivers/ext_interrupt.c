/********************************************************************************
 * Copyright (C), 1997-2013, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :ext_interrupt.c
 * Author         :wujing   
 * Date           :2013.02.26
 * Description    :用来配置系统所用外部中断引脚，及放置中断处理函数               
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2013.02.26 : 1.0.0 : wujing
 * Modification   : 初始代码编写。
 ********************************************************************************/
 #include "ext_interrupt.h"
 
 /*******************************************************************************
 * Function       : PA000092_EXTI_Config
 * Author         : wujing
 * Date           : 2013.02.26
 * Description    : PA000092的外部中断管脚配置，掉电检测管脚为PB14，中断触发方式为上升沿触发
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2013.02.26 : 1.0.0 : wujing
 * Modification   : 初始代码编写
 ********************************************************************************/
 void PA000092_EXTI_Config(void)
{
    device_gpio_config(DEVICE_ID_EXT_INTERRUPT, TRUE);//初始化IO口
    
    EXTI_InitTypeDef   EXTI_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;

    /* Connect EXTI Line to DRDY pin */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource14); //PA000092掉电检测管脚为PB14

    /* Configure DRDY's EXTI Line */
    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//上升沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set EXTI Line0 Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void EXTI15_10_IRQHandler(void)    
{
	//PA0092_EXTI_IRQHandler();	  //这个应该开放给用户来定义 
}