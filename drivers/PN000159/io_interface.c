/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :io_interface.c
 * Author         :Xu Shun'an
 * Date           :2011.08.04
 * Description    :IO控制器的端口初始化
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2011-08-04 : 1.0.0 : xusa
 * Modification   : 初始代码编写。
 ********************************************************************************/
#include "drivers.h"
extern void ETH_GPIO_Conf(void); //用于实现检测到网线再打开网络设备和创建网络任务的功能

// IO端口定义
typedef struct 
{
    GPIO_TypeDef *Base;     // 端口基地址
    GPIO_InitTypeDef io;    // IO口基本定义
    uint8_t       sinit;    // 端口初始化状态(0-无动作,1-低电平,2-高电平)
}TGPIOA;

TGPIOA const gGPIOAll[] = 
{
	{.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_15, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 2},     //  run led     //P-N-000159

//	{.Base = GPIOA, {.GPIO_Pin = GPIO_Pin_11, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},    // ATT7022B_SCK  //来自IO-D板，用来参考
//    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_6, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},                                   // ATT7022B_MISO
//    {.Base = GPIOC, {.GPIO_Pin = GPIO_Pin_6, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // ATT7022B_MOSI
//    {.Base = GPIOA, {.GPIO_Pin = GPIO_Pin_12, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},    // ATT7022B_NSS
//    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_8, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 2},     // ATT7022B_RESET
//    {.Base = GPIOG, {.GPIO_Pin = GPIO_Pin_3, .GPIO_Mode = GPIO_Mode_IN, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},                                   // ATT7022B_REVP
//
//    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_4, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},      // SPI1_SCK
//    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_6, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},      // SPI1_MISO
//    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_5, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},      // SPI1_MOSI
//    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_3, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // SPI1_NSS


//    {.Base = GPIOC, {.GPIO_Pin = GPIO_Pin_10, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // SPI3_SCK
//    {.Base = GPIOC, {.GPIO_Pin = GPIO_Pin_11, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // SPI3_MISO
//    {.Base = GPIOC, {.GPIO_Pin = GPIO_Pin_12, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // SPI3_MOSI
//    {.Base = GPIOA, {.GPIO_Pin = GPIO_Pin_15, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},    // SPI3_NSS

    {.Base = GPIOB, {.GPIO_Pin = GPIO_Pin_10, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // SPI2_SCK  //PB10  //P-N-000159 用于控制EEPROM、FLASH
    {.Base = GPIOB, {.GPIO_Pin = GPIO_Pin_14, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // SPI2_MISO //PB14
    {.Base = GPIOB, {.GPIO_Pin = GPIO_Pin_15, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},     // SPI2_MOSI //PB15

    {.Base = GPIOE, {.GPIO_Pin = GPIO_Pin_0, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},    // W25Q64BV 片选
    {.Base = GPIOB, {.GPIO_Pin = GPIO_Pin_9, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},    // FM25W256 片选

    {.Base = GPIOD, {.GPIO_Pin = GPIO_Pin_12, .GPIO_Mode = GPIO_Mode_OUT, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 1},    // 485_EN3        //P-N-000159
    {.Base = GPIOD, {.GPIO_Pin = GPIO_Pin_8, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},      // UART3_TX
    {.Base = GPIOD, {.GPIO_Pin = GPIO_Pin_9, .GPIO_Mode = GPIO_Mode_AF, .GPIO_OType = GPIO_OType_PP, .GPIO_PuPd = GPIO_PuPd_UP, .GPIO_Speed = GPIO_Speed_50MHz}, .sinit = 0},      // UART3_RX
    


};


/***********************************************************************************
 * Function       : init_interface
 * Author         : Xu Shun'an
 * Date           : 2011.11.01
 * Description    : 端口初始化函数  
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None 
 ***********************************************************************************/ 
void init_interface(void)
{
    uint8_t tmp = sizeof(gGPIOAll)/sizeof(TGPIOA);

    for (uint8_t i = 0;i<tmp;++i)
    {
        GPIO_Init(gGPIOAll[i].Base,(GPIO_InitTypeDef*)&gGPIOAll[i].io);
        if (1 == gGPIOAll[i].sinit)
        {
            GPIO_ResetBits(gGPIOAll[i].Base,gGPIOAll[i].io.GPIO_Pin);
        }
        else if (2 == gGPIOAll[i].sinit)
        {
            GPIO_SetBits(gGPIOAll[i].Base,gGPIOAll[i].io.GPIO_Pin);
        }
    }
    
    // 管脚映射
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);

   
//    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
//    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);
//    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);
//
//	  GPIO_PinAFConfig(GPIOE, GPIO_PinSource4, GPIO_AF_SPI1); //wujing 2012.11.26 add for xu
//    GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_SPI1);
//    GPIO_PinAFConfig(GPIOE, GPIO_PinSource6, GPIO_AF_SPI1);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);  //wujing 2012.12.18 for PN000159
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);
	ETH_GPIO_Conf();                                         //wujing 2012.12.18 for PN000159
}


