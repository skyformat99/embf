/******************************************************************************
* Copyright (C), 1997-2010, SUNGROW POWER SUPPLY CO., LTD.
* File name      :device.c
* Author         :llemmx
* Date           :2011-05-03
* Description    :实现设备文件操作接口，与硬件初始化接口。
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include "drivers.h"
#include "fcntl.h"
#include "stm32f4xx.h"
#include "utils.h"

//驱动接口
typedef struct {
    int32_t (*lseek)   (int64_t, int32_t);
    int32_t (*read)    (uint8_t *, uint32_t);
    int32_t (*write)   (const uint8_t *, uint32_t);
    int32_t (*ioctl)   (uint32_t, void *);
    int32_t (*open)    (int32_t, int32_t);
    int32_t (*release) (void);
#if SYS_OS_SUPPORT
    int32_t (*poll)    (void *);
#endif
    char *drvname;
}file_operations;

//全局设备结构体(静态)
file_operations const g_drv[]={
#ifdef DRV_LM240160K    
#if DRV_LM240160K
    {.open=lm_open,.release=NULL,.read=NULL,.poll=NULL,
     .write=lm_write,.lseek=NULL,.ioctl=lm_ioctl,.drvname="LCD0"},
#endif 
#endif
     
#ifdef DRV_SYSTICK
#if DRV_SYSTICK
    {
        .open = dSysTick_open, .release = dSysTick_release, .read = dSysTick_read,
        .write = NULL, .lseek = NULL, .ioctl = dSysTick_ioctl, .drvname = "SYSTICK0"
    },      /* SysTick */
#endif
#endif
    
    
#ifdef DRV_DIDO
#if DRV_DIDO
    {.open=dido_open,.release=dido_close,.read=dido_read,.poll=NULL,
     .write=NULL,.lseek=NULL,.ioctl=dido_ioctl,.drvname="DIDO"},
#endif 
#endif

#ifdef DRV_NET
#if DRV_NET
    {
        .open = net_open, .release = net_close, .read = NULL,.poll=net_poll,
        .write = NULL, .lseek = NULL, .ioctl = net_ioctl, .drvname = "NET0"
    },      /* SysTick */
#endif
#endif

#ifdef DRV_USARTX
#if DRV_USARTX
    {
        .open = usart1_open, .release = usart1_close, .read = usart1_read,.poll=usart1_poll,
        .write = usart1_write, .lseek = NULL, .ioctl = usart1_ioctl, .drvname = "USART1"
    },/* usart1 */
    {
        .open = usart2_open, .release = usart2_close, .read = usart2_read,.poll=usart2_poll,
        .write = usart2_write, .lseek = NULL, .ioctl = usart2_ioctl, .drvname = "USART2"
    },/* usart2 */
    {
        .open = usart3_open, .release = usart3_close, .read = usart3_read,.poll=usart3_poll,
        .write = usart3_write, .lseek = NULL, .ioctl = usart3_ioctl, .drvname = "USART3"
    },/* usart3 */
    {
        .open = usart4_open, .release = usart4_close, .read = usart4_read,.poll=usart4_poll,
        .write = usart4_write, .lseek = NULL, .ioctl = usart4_ioctl, .drvname = "USART4"
    },/* usart4 */
    {
        .open = usart5_open, .release = usart5_close, .read = usart5_read,.poll=usart5_poll,
        .write = usart5_write, .lseek = NULL, .ioctl = usart5_ioctl, .drvname = "USART5"
    },/* usart5 */
#endif
#endif

#ifdef DRV_CANX
#if DRV_CANX
    {
        .open = can1_open, .release = can1_release, .read = can1_read,.poll=can1_poll,
        .write = can1_write, .lseek = NULL, .ioctl = can1_ioctl, .drvname = "CAN1"
    },     /* CAN1 */
    {
        .open = can2_open, .release = can2_release, .read = can2_read,.poll=can2_poll,
        .write = can2_write, .lseek = NULL, .ioctl = can2_ioctl, .drvname = "CAN2"
    },     /* CAN2 */
#endif
#endif

#ifdef DRV_EEPROM
#if DRV_EEPROM
    {
        .open = EEPROM_open, .release = EEPROM_close, .read = EEPROM_read,
        .write = EEPROM_write, .lseek = EEPROM_lseek, .ioctl = EEPROM_ioctl, .drvname = "FM31XX0"
    },/* fm31xx */
#endif
#endif

#ifdef DRV_W25Qx
#if DRV_W25Qx
    {
        .open = w25qx_open, .release = w25qx_close, .read = w25qx_read,.poll=NULL,
        .write = w25qx_write, .lseek = w25qx_lseek, .ioctl = w25qx_ioctl, .drvname = "W25QX0"
    },/* W25q16bv */
#endif
#endif

#ifdef DRV_ADC
#if DRV_ADC
    {
        .open = adc3_open, .release = adc3_release, .read = adc3_read,
        .write = NULL, .lseek = NULL, .ioctl = adc3_ioctl, .drvname = "ADC3"
    },/* ADC3 */
#endif
#endif

#ifdef DRV_RTC
#if DRV_RTC
    {
        .open = r8025_open, .release = r8025_release, .read = r8025_read,.poll=NULL,
        .write = r8025_write, .lseek = NULL, .ioctl = NULL, .drvname = "RTC0"
    },/* RTC */
#endif
#endif

#ifdef DRV_ADS1247
#if DRV_ADS1247
    {
        .open = ads1247_open, .release = ads1247_close, .read = ads1247_read,
        .write = NULL, .lseek = NULL, .ioctl = ads1247_ioctl, .drvname = "ADS1247"
    },/* ADS1247 */
#endif
#endif

#ifdef DRV_EXTWDG
#if DRV_EXTWDG
    {
        .open = extwdg_open, .release = extwdg_close, .read = NULL,.poll=NULL,
        .write = NULL, .lseek = NULL, .ioctl = extwdg_ioctl, .drvname = "EXTWDG0"
    },/* EXTWDG */
#endif
#endif
    
#ifdef DRV_KEY
#if DRV_KEY
    {
        .open = key_open, .release = key_close, .read = key_read,.poll=key_poll,
        .write = NULL, .lseek = NULL, .ioctl = key_ioctl, .drvname = "KEY0"
    },/* KEY */
#endif
#endif

#ifdef DRV_METER
#if DRV_METER
    {
        .open = ATT7022B_open, .release = NULL, .read = ATT7022B_read,.poll=NULL,
        .write = ATT7022B_write, .lseek = NULL, .ioctl = ATT7022B_ioctl, .drvname = "ATT7022B"
    },/* KEY */
#endif
#endif
};

static uint8_t g_DrvMaxNum;
/******************************************************************************
* Function       :RCC_Configuration
* Author         :llemmx
* Date           :2011-05-03
* Description    :配置系统时钟
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
ErrorStatus HSEStartUpStatus;
void RCC_Configuration(void)
{
    /* RCC system reset(for debug purpose) */
    RCC_DeInit();

#ifdef DATA_IN_ExtSRAM
    SystemInit_ExtMemCtl();
#endif /* DATA_IN_ExtSRAM */

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Wait till HSE is ready */
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if(HSEStartUpStatus == SUCCESS)
    {
        /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
        FLASH_SetLatency(FLASH_Latency_5); /* VCC = 3.3V, HCLK = 168MHz */

        /* HCLK = SYSCLK */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        /* PCLK2 = HCLK / 2 */
        RCC_PCLK2Config(RCC_HCLK_Div2);

        /* PCLK1 = HCLK / 4 */
        RCC_PCLK1Config(RCC_HCLK_Div4);

        /* VCO input Clock: 25MHz / PLLM(25) = 1MHz */
        /* VCO output Clock: 1MHz * PLLN(336) = 336MHz */
        /* PLLCLK: 336MHz / PLLP(2) = 168MHz */
        /* USB SDIO RNG: 336MHz / PLLQ(7) = 48MHz */
        RCC_PLLConfig(RCC_PLLSource_HSE, 25U, 336U, 2U, 7U);

        /* Enable PLL */
        RCC_PLLCmd(ENABLE);

        /* Wait till PLL is ready */
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        {
        }



        /* Select PLL as system clock source */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        /* Wait till PLL is used as system clock source */
        while(RCC_GetSYSCLKSource() != 0x08U)
        {
        }
    }

    /* GPIO Enable */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | \
                           RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | \
                           RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | \
                           RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH | \
                           RCC_AHB1Periph_GPIOI, ENABLE);
    /* Periph Enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_USART3 | RCC_APB1Periph_SPI3, ENABLE);

    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_CRYP, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
	                           RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
}

/******************************************************************************
* Function       :NVIC_Configuration
* Author         :llemmx
* Date           :2011-05-03
* Description    :Configures the nested vectored interrupt controller.
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */ 
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0); 
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08004000 */ 
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x08000000);   
#endif
    /* 2 bit for pre-emption priority, 2 bits for subpriority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	NVIC_InitTypeDef NVIC_InitStructure;               //wujing 2012.12.18 for PN000159

    NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/******************************************************************************
* Function       :dev_init
* Author         :llemmx
* Date           :2011-05-03
* Description    :设备初始化
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :初始化成功返回TRUE，初始化失败返回FALSE
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
bool dev_init(void)
{
    //初始化系统时钟
    RCC_Configuration();
    //remap jtag
    //GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    //中断向量
    NVIC_Configuration();
    //init io interface
    init_interface();
    g_DrvMaxNum=(sizeof(g_drv)/sizeof(file_operations));

    //open iwdg
    //iwdg_init();
    return true;
}

/******************************************************************************
* Function       :open
* Author         :llemmx
* Date           :2011-05-03
* Description    :打开驱动文件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :pathname - 文件名称
                  flags - 打开文件标志
                  mode - 模式
* Output         :无
* Return         :打开成功返回文件句柄，失败返回EFAULT
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t drv_open(const char* pathname, int32_t flags, int32_t mode)
{
    uint8_t i;
    int32_t ret = OPFAULT;
	if(NULL != pathname)
	{
	    for (i=0;i<g_DrvMaxNum;++i){
	        if (strcmp(pathname,g_drv[i].drvname)==0){
	            ret=g_drv[i].open(flags,mode);
	            if (DSUCCESS!=ret){
	                return ret;
	            }
	            return i+1;
	        }
	    }
	}
    return ret;
}

/******************************************************************************
* Function       :close
* Author         :llemmx
* Date           :2011-05-06
* Description    :关闭文件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :fd - 文件句柄
* Output         :无
* Return         :成功返回DSUCCESS，失败返回EFAULT
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t drv_close(int32_t fd)
{
    int32_t ret = OPFAULT;
    if (((uint32_t)fd<=g_DrvMaxNum)&&(fd != 0))
	{
	    if (NULL!=g_drv[fd-1].release)
		{
	    	ret=g_drv[fd-1].release();
		}
	}
    return ret;
}

/******************************************************************************
* Function       :read
* Author         :llemmx
* Date           :2011-05-06
* Description    :关闭文件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :fd - 文件句柄
                  count - 缓冲区长度
* Output         :buf - 缓冲区指针
* Return         :成功返回实际读取数据长度，失败返回EFAULT
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t drv_read(int32_t fd, void* buf,uint32_t count)
{
    int32_t ret = OPFAULT;
    //系统计数驱动
    if (((uint32_t)fd<=g_DrvMaxNum)&&(fd != 0))
	{
	    if (NULL!=g_drv[fd-1].read)
		{
	    	ret=g_drv[fd-1].read(buf,count);
		}
	}
    return ret;
}

/******************************************************************************
* Function       :write
* Author         :llemmx
* Date           :2011-05-06
* Description    :写数据
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :fd - 文件句柄
                  count - 缓冲区长度
* Output         :buf - 缓冲区指针
* Return         :成功返回实际写入数据长度，失败返回EFAULT
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t drv_write(int32_t fd, const void* buf, uint32_t count)
{
    int32_t ret = OPFAULT;
    //系统计数驱动
    if (((uint32_t)fd<=g_DrvMaxNum)&&(fd != 0))
	{
	    if (NULL!=g_drv[fd-1].write)
		{    
	   		 ret=g_drv[fd-1].write(buf,count);
	    }
	}
	return ret;
}

/******************************************************************************
* Function       :ioctl
* Author         :llemmx
* Date           :2011-05-06
* Description    :驱动的特殊操作
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :fildes - 文件句柄
                  request - 用户自定义的操作命令
                  arg - 传入的用户参数指针
* Output         :无
* Return         :成功返回DSUCCESS，失败返回EFAULT
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t drv_ioctl(int32_t fildes, int32_t request,void* arg)
{
    int32_t ret = OPFAULT;
    //系统计数驱动
	if (((uint32_t)fildes<=g_DrvMaxNum)&&(fildes != 0))
	{
	    if (NULL!=g_drv[fildes-1].ioctl)
		{    
	   		 ret=g_drv[fildes-1].ioctl(request,arg);
	    }
	}
	return ret;
}

/******************************************************************************
* Function       :lseek
* Author         :llemmx
* Date           :2011-05-06
* Description    :跳转读写指针
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :fd - 文件句柄
                  offset - 跳转的相对位置
                  whence - 跳转的起始点
* Output         :无
* Return         :成功返回当前指针位置，失败返回EFAULT
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t drv_lseek(int32_t fd, int32_t offset, int32_t whence)
{
    int32_t ret = OPFAULT;
    //系统计数驱动
    if (((uint32_t)fd<=g_DrvMaxNum)&&(fd != 0))
	{
	    if (NULL!=g_drv[fd-1].lseek)
		{
	   		 ret=g_drv[fd-1].lseek(offset,whence);
		}
	}
    return ret;
}

int32_t drv_poll(int32_t fd,void *ev)
{
    int32_t ret = OPFAULT;
    //系统计数驱动
    if (((uint32_t)fd<=g_DrvMaxNum)&&(fd != 0))
	{
	    if (NULL!=g_drv[fd-1].poll)
		{
	   		 ret=g_drv[fd-1].poll(ev);
		}
	}
    return ret;
}
