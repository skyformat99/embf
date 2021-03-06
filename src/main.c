/****************************************************************************** 
* Copyright (C), 1997-2010, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :main.c
* Author         :llemmx
* Date           :2011-05-03
* Description    :工程的入口文件，主要负责初始化，消息调度等工作。
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include "drivers.h"
#include "Includes.h"
#include "config.h"
#include "assage.h"
#include "lcd_module.h"
#include "stm32f4x7_eth.h"
#include "utils.h"
#include "netconf.h"

volatile t_gparam gt_glp;

#if (defined P_P_000071) || (defined P_P_000069)
#define  APP_TASK_INIT_STK_SIZE 512
#define  APP_TASK_UI_STK_SIZE   1024
#define  APP_TASK_NET_STK_SIZE  1024
#define  APP_TASK_MAIN_STK_SIZE  1024
    
#define  APP_TASK_UI_PRIO        2
#define  APP_TASK_MAIN_PRIO      3
#define  APP_TASK_NET_PRIO       5
    
__align(8) static OS_STK Task_InitStk[APP_TASK_INIT_STK_SIZE];         // 初始化任务进程堆栈
__align(8) static OS_STK Task_UIStk[APP_TASK_UI_STK_SIZE];             // UI进程堆栈
__align(8) static OS_STK Task_NETStk[APP_TASK_NET_STK_SIZE];           // 网络进程堆栈
__align(8) static OS_STK Task_MainStk[APP_TASK_MAIN_STK_SIZE];         // 主进程堆栈

//serial port buffer
uint8_t m_serial1_rbuf[256];
uint8_t m_serial6_rbuf[256];//串口6用来配置WIFI模块
uint8_t m_serial3_rbuf[256];
uint8_t m_serial1_sbuf[256];
uint8_t m_serial6_sbuf[256];
uint8_t m_serial3_sbuf[256];

//看门狗实验变量定义_start
//定义在global.h
twatch_dog_ver task_m = {0};
twatch_dog_ver task_u = {0};
twatch_dog_ver task_n = {0};
extern void RTC_Config(void); //为了低功耗时关掉内部RTC时钟
#endif
#ifdef P_A_000092
#define  APP_TASK_INIT_STK_SIZE 512
#define  APP_TASK_NET_STK_SIZE  1024
#define  APP_TASK_MAIN_STK_SIZE 2048
#define  APP_TASK_TIME_STK_SIZE  1024   //wujing 2012.12.11 增加定时刷新任务，刷新RTC时间

#define  APP_TASK_MAIN_PRIO      3
#define  APP_TASK_NET_PRIO       5
#define  APP_TASK_TIME_PRIO      6		//wujing 2012.12.11 增加定时刷新任务，刷新RTC时间

__align(8) static OS_STK Task_InitStk[APP_TASK_INIT_STK_SIZE];         // 初始化任务进程堆栈
__align(8) static OS_STK Task_NETStk[APP_TASK_NET_STK_SIZE];           // 网络进程堆栈
__align(8) static OS_STK Task_MainStk[APP_TASK_MAIN_STK_SIZE];         // 主进程堆栈
__align(8) static OS_STK Task_TimeStk[APP_TASK_TIME_STK_SIZE];         // 定时任务进程堆栈

//serial port buffer
uint8_t m_serial1_rbuf[256];
uint8_t m_serial2_rbuf[256];
uint8_t m_serial3_rbuf[256];
uint8_t m_serial1_sbuf[256];
uint8_t m_serial2_sbuf[256];
uint8_t m_serial3_sbuf[256];

//看门狗实验变量定义_start
//定义在global.h
twatch_dog_ver task_m = {0};  //主任务
twatch_dog_ver task_n = {0};  //网络任务

uint8_t netline_config_flag = false; //网线配置标志，上电连接网线的话只执行一次
extern bool netline_connected(void);

/******************************************************************************
* Function       :task_timer
* Author         :wujing	
* Date           :2012.12.12
* Description    :周期处理函数,判断网线是否连接
* Calls          :None
* Input          :None
* Output         :None
* Return         :None
*******************************************************************************
* History:
*------------------------------------------------------------------------------
* 2012.09.18 : 1.0.0 : Linfei
* Modification   : 初始代码编写
*------------------------------------------------------------------------------
******************************************************************************/
void task_timer(void *p_arg)
{
    while(1)
    {  
        if (true == netline_connected())  //网线连接上	 //wujing 2012.12.11
        {
            if (false == netline_config_flag)
            {
                netline_config_flag = true;    //上电只执行一次
                OSTaskCreateExt((void (*)(void *)) task_net,             // 创建网络进程
                    (void          * ) 0,
                    (OS_STK        * )&Task_NETStk[APP_TASK_NET_STK_SIZE - 1],
                    (INT8U           ) APP_TASK_NET_PRIO,
                    (INT16U          ) APP_TASK_NET_PRIO,
                    (OS_STK        * )&Task_NETStk[0],
                    (INT32U          ) APP_TASK_NET_STK_SIZE,
                    (void          * )0,
                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
                 OSTaskDel(OS_PRIO_SELF);  //创建网络任务后任务完成，删除自身 
            }
        }
        OSTimeDly(250);		  //500ms执行一次 ，待测试，周期准确性
    }
}
#endif

/******************************************************************************
* Function       :OSTimeTickHook
* Author         :llemmx
* Date           :2011-05-03
* Description    :ucos定时器回调，主要可以再这里处理一些用户自定义的时间功能。
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
void OSTimeTickHook(void){
    DRV_TIMER;
}

/******************************************************************************
* Function       :fputc
* Author         :llemmx
* Date           :2011-05-03
* Description    :调试信息打印
* Calls          :无 
* Table Accessed :无
* Table Updated  :无
* Input          :ch - 具体要发送的数值
                  f - 文件接口
* Output         :向串口发送数据
* Return         :返回要发送的数据
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int fputc(int ch, FILE *f)
{
#if G_DEBUG
    
#endif
    return ch;
}
#if (defined P_P_000071) || (defined P_P_000069)
void init_run(void)
{    
    uint8_t time_buf[7]={0};
    GPIO_SetBits(GPIOA,GPIO_Pin_3);
    OSSchedLock();
    drv_close(gt_glp.fdido);
    drv_close(gt_glp.fkey);
    dev_init();
      
    SysTick_Config(21000U);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);//系统时钟8分频
    RTC_Config();//执行RTC配置
    OSTimeSet(0);
    while(OSTimeGet()<1000)//上电后延时5S 2013.03.08    
    {
        drv_ioctl(gt_glp.fextwdg, FEED_EXTWDG, NULL); 		    /* 外部看门狗喂狗 */
    }

   // OSTimeDly(1000); //此处的延时可以保证LCD设备的正确启动，即等待外设上电稳定后才进行打开LCD设备的操作//关调度情况下OSTimeDly()不可用
    //initization all drivers
    //gt_glp.fsystick    = drv_open("SYSTICK0" , 0 , 0);
    gt_glp.frtc        = drv_open("RTC0", 0, 0);  /* 打开RTC */
    gt_glp.fdcan1      = drv_open("CAN1",0, 3);
    gt_glp.flcd        = drv_open("LCD0", 0, 0);
    gt_glp.feeprom1    = drv_open("FM31XX0", 0, 0);
    gt_glp.fkey        = drv_open("KEY0", 0, 0);
    gt_glp.fflash1     = drv_open("W25QX0", 0, 0);
    gt_glp.fnet        = drv_open("NET0", 0, 5);  /* 打开网卡设备 */
    gt_glp.fdido       = drv_open("DIDO", 0, 0);  /* 打开DI和DO */
    
    if(6 == drv_read(gt_glp.frtc,time_buf,6U))//上电需要重新对时
    {
        embf_set_time(time_buf, 6);
    }
    
    device_gpio_config(DEVICE_ID_WIFI, TRUE);
    
    //uint8_t buf[]={0x12,0x10,0x15,0x19,0x57,0x30};
    //drv_write(gt_glp.frtc,buf,6);
    
    TUsartX_ioctl uio;
    gt_glp.fserial1=drv_open("USART1",O_NONBLOCK,0x0000);// 串口1，用于和PC后台通讯
    uio.rbuf    =m_serial1_rbuf;
    uio.rbuflen =256;
    uio.sbuf    =m_serial1_sbuf;
    uio.sbuflen =256;
    drv_ioctl(gt_glp.fserial1,USART_SET_BUF,&uio);
    drv_ioctl(gt_glp.fserial1,USART_SET_485E,&uio);

    gt_glp.fserial6=drv_open("USART6",O_NONBLOCK,0x0001);// 串口6，用于配置WIFI模块
    uio.rbuf    =m_serial6_rbuf;
    uio.rbuflen =256;
    uio.sbuf    =m_serial6_sbuf;
    uio.sbuflen =256;
    drv_ioctl(gt_glp.fserial6,USART_SET_BUF,&uio);

   
		
		
    gt_glp.pwr_mode=0;
    OSSchedUnlock();
}

void enter_low_power(void)
{
    gt_glp.pwr_mode=1;
    
    GPIO_ResetBits(GPIOA,GPIO_Pin_3);
    
    drv_close(gt_glp.frtc);
    drv_close(gt_glp.fdcan1);
    drv_close(gt_glp.flcd);
    drv_close(gt_glp.feeprom1);
//  drv_close(gt_glp.fkey);  //wujing 2013.1.5 用户可以通过按键进行低功耗唤醒 
    drv_close(gt_glp.fflash1);
    drv_close(gt_glp.fnet);
//  drv_close(gt_glp.fdido); //低功耗需要
    drv_close(gt_glp.fserial1);
    drv_close(gt_glp.fserial2);
    
    
    GPIO_DeInit(GPIOC);
    GPIO_DeInit(GPIOD);
    GPIO_DeInit(GPIOE);  
    GPIO_DeInit(GPIOF);
    GPIO_DeInit(GPIOG);
    GPIO_DeInit(GPIOH);
    GPIO_DeInit(GPIOI);
		
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | \
                           RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | \
                           RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH | \
                           RCC_AHB1Periph_GPIOI, DISABLE);
													 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, DISABLE);
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_CRYP, DISABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, DISABLE);
    
    /* Disable the PWR clock */                          //RTC 打开的时钟均关掉
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, DISABLE);   
    /* The RTC Clock may varies due to LSI frequency dispersion. */   
    /* Disable the LSI OSC */ 
    RCC_LSICmd(DISABLE);
    /* Disable the RTC Clock */
    RCC_RTCCLKCmd(DISABLE);
													 
    //2013.1.5将GPIOA的不用管脚全部初始化为浮空
    GPIO_InitTypeDef GPIO_InitStructure;
				
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);
		
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
    GPIO_Init(GPIOB, &GPIO_InitStructure);
		
    
    RCC_DeInit();
    
    SysTick_Config(2000);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
}
#endif
#ifdef P_A_000092
void init_run(void)
{
 //   dev_init();

    OSTimeDly(500);
    
    SysTick_Config(21000U);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);//系统时钟8分频

//	net_init();
    //initization all drivers
    //gt_glp.fsystick    = drv_open("SYSTICK0" , 0 , 0);
    gt_glp.frtc        = drv_open("RTC0", 0, 0);  /* 打开RTC */
    gt_glp.fdcan1      = drv_open("CAN1",0, 3);
    gt_glp.fdcan2      = drv_open("CAN2",0, 3);	 //wujing 2012.12.13 add for xu
    gt_glp.feeprom1    = drv_open("FM31XX0", 0, 0);
    gt_glp.fflash1     = drv_open("W25QX0", 0, 0);
    gt_glp.fdido       = drv_open("DIDO", 0, 0);  /* 打开DI和DO */
	gt_glp.fAtt7022B   = drv_open("ATT7022B", 0, 0);  /* 打开POWER METER */
    gt_glp.fadc1       = drv_open("ADC3", 0, 0xC7B0); //wujing 2012.11.26 add for xu,打开10个通道
    gt_glp.fads1247    = drv_open("ADS1247", 0, 0);  
    gt_glp.fnet        = drv_open("NET0", 0, 5);  /* 打开网卡设备 */ //wujing 2013.03.1兼容71与92
//     gt_glp.fextwdg     = drv_open("EXTWDG0", 0, 0);    //开启外部看门狗
    
    
    //uint8_t buf[]={0x12,0x10,0x15,0x19,0x57,0x30};
    //drv_write(gt_glp.frtc,buf,6);
    TUsartX_ioctl uio;
    gt_glp.fserial1=drv_open("USART1",O_NONBLOCK,0x0000);// 串口1，用于和PC后台通讯
    uio.rbuf    =m_serial1_rbuf;
    uio.rbuflen =256;
    uio.sbuf    =m_serial1_sbuf;
    uio.sbuflen =256;
    drv_ioctl(gt_glp.fserial1,USART_SET_BUF,&uio);
    drv_ioctl(gt_glp.fserial1,USART_SET_485E,&uio);
    
    gt_glp.fserial2=drv_open("USART2",O_NONBLOCK,0x0001); // 串口2，用于和PC后台通讯
    uio.rbuf    =m_serial2_rbuf;
    uio.rbuflen =256;
    uio.sbuf    =m_serial2_sbuf;
    uio.sbuflen =256;
    drv_ioctl(gt_glp.fserial2, USART_SET_BUF, &uio);
    drv_ioctl(gt_glp.fserial2, USART_SET_485E, &uio);
    
    gt_glp.fserial3=drv_open("USART3",O_NONBLOCK,0x0002); // 串口3，用于和PC后台通讯
    uio.rbuf    =m_serial3_rbuf;
    uio.rbuflen =256;
    uio.sbuf    =m_serial3_sbuf;
    uio.sbuflen =256;
    drv_ioctl(gt_glp.fserial3, USART_SET_BUF, &uio);
    drv_ioctl(gt_glp.fserial3, USART_SET_485E, &uio);

}
#endif
#if (defined P_P_000071) || (defined P_P_000069)
/******************************************************************************
* Function       :Task_init
* Author         :llemmx
* Date           :2011-05-03
* Description    :主进程初始化函数，主要负责主进程初始化，各种设备的初始化，时钟，全局变
                  量初始化等工作。
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :p_arg - 进程启动时所附带的参数
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
static void task_init(void *p_arg)
{
    //INT8U ret;
    uint16_t time_regulate_count = 0;//校正计数
    uint8_t time_buf[7];//用来保存时间数据
    SysTick_Config(21000U);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);//系统时钟8分频
    OSStatInit();

    //initization global variant
    memset((void *)&gt_glp,0,sizeof(t_gparam));
    assage_init();
    
    gt_glp.fextwdg     = drv_open("EXTWDG0", 0, 0);
    
    OSTimeSet(0);
    while(OSTimeGet()<4000)//上电后延时5S 2013.03.08    
    {
        drv_ioctl(gt_glp.fextwdg, FEED_EXTWDG, NULL); 		    /* 外部看门狗喂狗 */
    }
    init_run();
    
    //create tasks
    OSTaskCreateExt((void (*)(void *)) task_main,             // 创建主进程
                        (void          * ) 0,
                        (OS_STK        * )&Task_MainStk[APP_TASK_MAIN_STK_SIZE - 1],
                        (INT8U           ) APP_TASK_MAIN_PRIO,
                        (INT16U          ) APP_TASK_MAIN_PRIO,
                        (OS_STK        * )&Task_MainStk[0],
                        (INT32U          ) APP_TASK_MAIN_STK_SIZE,
                        (void          * )0,
                        (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
    
    OSTaskCreateExt((void (*)(void *)) task_ui,             // 创建UI进程
                    (void          * ) 0,
                    (OS_STK        * )&Task_UIStk[APP_TASK_UI_STK_SIZE - 1],
                    (INT8U           ) APP_TASK_UI_PRIO,
                    (INT16U          ) APP_TASK_UI_PRIO,
                    (OS_STK        * )&Task_UIStk[0],
                    (INT32U          ) APP_TASK_UI_STK_SIZE,
                    (void          * )0,
                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
    
    OSTaskCreateExt((void (*)(void *)) task_net,             // 创建网络进程
                    (void          * ) 0,
                    (OS_STK        * )&Task_NETStk[APP_TASK_NET_STK_SIZE - 1],
                    (INT8U           ) APP_TASK_NET_PRIO,
                    (INT16U          ) APP_TASK_NET_PRIO,
                    (OS_STK        * )&Task_NETStk[0],
                    (INT32U          ) APP_TASK_NET_STK_SIZE,
                    (void          * )0,
                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

//看门狗进程实验开始
	while(1)
	{
    IWDG_ReloadCounter();//内部看门狗喂狗
    drv_ioctl(gt_glp.fextwdg, FEED_EXTWDG, NULL); 		    /* 外部看门狗喂狗 */

		if(task_m.err_flag != 0)		   //之前发生了异常情况
		{
			if(task_m.delay_flag_500 >0)	 //500ms计数开始，同样，仅捕捉从1减到0这个状态。
			{
				task_m.delay_flag_500 --;
				if(0 == task_m.delay_flag_500)	//500ms延时到再进行任务重启是否成功的判断。
				{
					if(task_m.time_cnt == 0)	   //重启后500ms，任务依然没有喂软件狗
					{
						task_m.err_flag ++;
						task_m.delay_flag_500 = 30;	  				  //用以在重启任务后延时500ms，再判断是否重启成功，可靠性增加。
						OSTaskDel(3);		//重启任务task_main			  //用删除任务再重新建立任务的方式重启任务。 
						OSTaskCreateExt((void (*)(void *)) task_main,             // 创建主进程
                        (void          * ) 0,
                        (OS_STK        * )&Task_MainStk[APP_TASK_MAIN_STK_SIZE - 1],
                        (INT8U           ) APP_TASK_MAIN_PRIO,
                        (INT16U          ) APP_TASK_MAIN_PRIO,
                        (OS_STK        * )&Task_MainStk[0],
                        (INT32U          ) APP_TASK_MAIN_STK_SIZE,
                        (void          * )0,
                        (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
					}
					else if(task_m.time_cnt >0)	//任务重启成功
					{
						task_m.err_flag = 0;
					}					
					if(task_m.err_flag >= 3)		    //重启任务3次依然没有成功，重启CPU
					{
						NVIC_SystemReset();     //重启CPU;
					}		
				}
			}
		}
		else if(0 == task_m.err_flag)	   //之前没有发生异常情况，
		{
			if(task_m.time_cnt > 0) //只有值大于0时才进行操作，用于解决初始化时值为0的问题，这里只捕捉从1变为0的变化。
			{
				task_m.time_cnt --;
				if(0 == task_m.time_cnt) //底层任务3S都没有喂狗，说明任务已经挂，
				{
					task_m.err_flag ++;		 					  //任务故障计数，用以达到限定次数后重启CPU
					task_m.delay_flag_500 = 30;	  				  //用以在重启任务后延时500ms，再判断是否重启成功，可靠性增加。
					OSTaskDel(3);//重启任务task_main			  //用删除任务再重新建立任务的方式重启任务。 
					OSTaskCreateExt((void (*)(void *)) task_main,             // 创建主进程
                        (void          * ) 0,
                        (OS_STK        * )&Task_MainStk[APP_TASK_MAIN_STK_SIZE - 1],
                        (INT8U           ) APP_TASK_MAIN_PRIO,
                        (INT16U          ) APP_TASK_MAIN_PRIO,
                        (OS_STK        * )&Task_MainStk[0],
                        (INT32U          ) APP_TASK_MAIN_STK_SIZE,
                        (void          * )0,
                        (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
				}
			}
		
		}
		if(task_u.err_flag != 0)		   //之前发生了异常情况
		{
			if(task_u.delay_flag_500 >0)	 //500ms计数开始，同样，仅捕捉从1减到0这个状态。
			{
				task_u.delay_flag_500 --;
				if(0 == task_u.delay_flag_500)	//500ms延时到再进行任务重启是否成功的判断。
				{
					if(task_u.time_cnt == 0)	   //重启后500ms，任务依然没有喂软件狗
					{
						task_u.err_flag ++;
						task_u.delay_flag_500 = 30;
						OSTaskDel(2);//重启任务task_ui		  //用删除任务再重新建立任务的方式重启任务。
						OSTaskCreateExt((void (*)(void *)) task_ui,             // 创建UI进程
	                    (void          * ) 0,
	                    (OS_STK        * )&Task_UIStk[APP_TASK_UI_STK_SIZE - 1],
	                    (INT8U           ) APP_TASK_UI_PRIO,
	                    (INT16U          ) APP_TASK_UI_PRIO,
	                    (OS_STK        * )&Task_UIStk[0],
	                    (INT32U          ) APP_TASK_UI_STK_SIZE,
	                    (void          * )0,
	                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));	
					}
					else if(task_u.time_cnt >0)	//任务重启成功
					{
						task_u.err_flag = 0;
					}
					
					if(task_u.err_flag >= 3)		    //重启任务3次依然没有成功，重启CPU
					{
						NVIC_SystemReset();     //重启CPU;
					}		
				}
			}
		}
		else if(0 == task_u.err_flag)	   //之前没有发生异常情况，
		{
			if(task_u.time_cnt > 0) //只有值大于0时才进行操作，用于解决初始化时值为0的问题，这里只捕捉从1变为0的变化。
			{
				task_u.time_cnt --;
				if(0 == task_u.time_cnt) //底层任务3S都没有喂狗，说明任务已经挂，
				{
					task_u.err_flag ++;		  //任务故障计数，用以达到限定次数后重启CPU
					task_u.delay_flag_500 = 30;	  //用以在重启任务后延时500ms，再判断是否重启成功，可靠性增加。
					OSTaskDel(2);//重启任务task_ui		  //用删除任务再重新建立任务的方式重启任务。
			   	  OSTaskCreateExt((void (*)(void *)) task_ui,             // 创建UI进程
	                    (void          * ) 0,
	                    (OS_STK        * )&Task_UIStk[APP_TASK_UI_STK_SIZE - 1],
	                    (INT8U           ) APP_TASK_UI_PRIO,
	                    (INT16U          ) APP_TASK_UI_PRIO,
	                    (OS_STK        * )&Task_UIStk[0],
	                    (INT32U          ) APP_TASK_UI_STK_SIZE,
	                    (void          * )0,
	                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));		 
				}
			}
		
		}
		if(task_n.err_flag != 0)		   //之前发生了异常情况
		{
			if(task_n.delay_flag_500 >0)	 //500ms计数开始，同样，仅捕捉从1减到0这个状态。
			{
				task_n.delay_flag_500 --;
				if(0 == task_n.delay_flag_500)	//500ms延时到再进行任务重启是否成功的判断。
				{
					if(task_n.time_cnt == 0)	   //重启后500ms，任务依然没有喂软件狗
					{
						task_n.err_flag ++;
						task_n.delay_flag_500 = 30;
						OSTaskDel(5);//重启任务task_net			  //用删除任务再重新建立任务的方式重启任务。
						OSTaskCreateExt((void (*)(void *)) task_net,             // 创建网络进程
	                    (void          * ) 0,
	                    (OS_STK        * )&Task_NETStk[APP_TASK_NET_STK_SIZE - 1],
	                    (INT8U           ) APP_TASK_NET_PRIO,
	                    (INT16U          ) APP_TASK_NET_PRIO,
	                    (OS_STK        * )&Task_NETStk[0],
	                    (INT32U          ) APP_TASK_NET_STK_SIZE,
	                    (void          * )0,
	                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
						
					}
					else if(task_n.time_cnt >0)	//任务重启成功
					{
						task_n.err_flag = 0;
					}
					
					if(task_n.err_flag >= 3)		    //重启任务3次依然没有成功，重启CPU
					{
						NVIC_SystemReset();     //重启CPU;
					}		
				}
			}
		}
		else if(0 == task_n.err_flag)	   //之前没有发生异常情况，
		{
			if(task_n.time_cnt > 0) //只有值大于0时才进行操作，用于解决初始化时值为0的问题，这里只捕捉从1变为0的变化。
			{
				task_n.time_cnt --;
				if(0 == task_n.time_cnt) //底层任务3S都没有喂狗，说明任务已经挂，
				{
					task_n.err_flag ++;		  //任务故障计数，用以达到限定次数后重启CPU
					task_n.delay_flag_500 = 30;	  //用以在重启任务后延时300ms*30=9S，再判断是否重启成功，可靠性增加,这里改为30，防止网络任务执行不到软件喂狗
					OSTaskDel(5);//重启任务task_net			  //用删除任务再重新建立任务的方式重启任务。
					OSTaskCreateExt((void (*)(void *)) task_net,             // 创建网络进程
	                    (void          * ) 0,
	                    (OS_STK        * )&Task_NETStk[APP_TASK_NET_STK_SIZE - 1],
	                    (INT8U           ) APP_TASK_NET_PRIO,
	                    (INT16U          ) APP_TASK_NET_PRIO,
	                    (OS_STK        * )&Task_NETStk[0],
	                    (INT32U          ) APP_TASK_NET_STK_SIZE,
	                    (void          * )0,
	                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK)); 
				}
			}
		
		}
        //对时功能,暂定半小时对下时
        if(time_regulate_count<6000)
        {
            time_regulate_count++;
        }
        else
        {
            time_regulate_count = 0;
            if(6 == drv_read(gt_glp.frtc,time_buf,6U))//读取成功且是合法数据,失败则不进行对时
            {
                 embf_set_time(time_buf, 6);
            }
        }
		OSTimeDly(300); 						//延时300ms	，挂起自己，让其他任务执行	
	}
//看门狗进程实验结束
}
#endif
#ifdef P_A_000092
/******************************************************************************
* Function       :Task_init
* Author         :llemmx
* Date           :2011-05-03
* Description    :主进程初始化函数，主要负责主进程初始化，各种设备的初始化，时钟，全局变
                  量初始化等工作。
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :p_arg - 进程启动时所附带的参数
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
static void task_init(void *p_arg)
{
    //INT8U ret;

    SysTick_Config(21000U);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);//系统时钟8分频
    OSStatInit();


    //initization global variant
    memset((void *)&gt_glp,0,sizeof(t_gparam));
    assage_init();
    
    init_run();
    
    //create tasks
    OSTaskCreateExt((void (*)(void *)) task_main,             // 创建主进程
                    (void          * ) 0,
                    (OS_STK        * )&Task_MainStk[APP_TASK_MAIN_STK_SIZE - 1],
                    (INT8U           ) APP_TASK_MAIN_PRIO,
                    (INT16U          ) APP_TASK_MAIN_PRIO,
                    (OS_STK        * )&Task_MainStk[0],
                    (INT32U          ) APP_TASK_MAIN_STK_SIZE,
                    (void          * )0,
                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
   

    OSTaskCreateExt((void (*)(void *)) task_timer,             // 创建定时进程
                    (void          * ) 0,
                    (OS_STK        * )&Task_TimeStk[APP_TASK_TIME_STK_SIZE - 1],
                    (INT8U           ) APP_TASK_TIME_PRIO,
                    (INT16U          ) APP_TASK_TIME_PRIO,
                    (OS_STK        * )&Task_TimeStk[0],
                    (INT32U          ) APP_TASK_TIME_STK_SIZE,
                    (void          * )0,
                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

//看门狗进程实验开始
	while(1)
	{
    	IWDG_ReloadCounter();//内部看门狗喂狗
        drv_ioctl(gt_glp.fextwdg, FEED_EXTWDG, NULL); /* 外部看门狗喂狗 */


		if(task_m.err_flag != 0)		   //之前发生了异常情况
		{
			if(task_m.delay_flag_500 >0)	 //500ms计数开始，同样，仅捕捉从1减到0这个状态。
			{
				task_m.delay_flag_500 --;
				if(0 == task_m.delay_flag_500)	//500ms延时到再进行任务重启是否成功的判断。
				{
					if(task_m.time_cnt == 0)	   //重启后500ms，任务依然没有喂软件狗
					{
						task_m.err_flag ++;
						task_m.delay_flag_500 = 30;	  				  //用以在重启任务后延时500ms，再判断是否重启成功，可靠性增加。
						OSTaskDel(3);		//重启任务task_main			  //用删除任务再重新建立任务的方式重启任务。 
						OSTaskCreateExt((void (*)(void *)) task_main,             // 创建主进程
                        (void          * ) 0,
                        (OS_STK        * )&Task_MainStk[APP_TASK_MAIN_STK_SIZE - 1],
                        (INT8U           ) APP_TASK_MAIN_PRIO,
                        (INT16U          ) APP_TASK_MAIN_PRIO,
                        (OS_STK        * )&Task_MainStk[0],
                        (INT32U          ) APP_TASK_MAIN_STK_SIZE,
                        (void          * )0,
                        (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
					}
					else if(task_m.time_cnt >0)	//任务重启成功
					{
						task_m.err_flag = 0;
					}					
					if( task_m.err_flag >= 3)		    //重启任务3次依然没有成功，重启CPU
					{
						NVIC_SystemReset();     //重启CPU;
					}		
				}
			}
		}
		else if(0 == task_m.err_flag)	   //之前没有发生异常情况，
		{
			if(task_m.time_cnt > 0) //只有值大于0时才进行操作，用于解决初始化时值为0的问题，这里只捕捉从1变为0的变化。
			{
				task_m.time_cnt --;
				if(0 == task_m.time_cnt) //底层任务3S都没有喂狗，说明任务已经挂，
				{
					task_m.err_flag ++;		 					  //任务故障计数，用以达到限定次数后重启CPU
					task_m.delay_flag_500 = 30;	  				  //用以在重启任务后延时5*300ms，再判断是否重启成功，可靠性增加。
					OSTaskDel(3);//重启任务task_main			  //用删除任务再重新建立任务的方式重启任务。 
					OSTaskCreateExt((void (*)(void *)) task_main,             // 创建主进程
                        (void          * ) 0,
                        (OS_STK        * )&Task_MainStk[APP_TASK_MAIN_STK_SIZE - 1],
                        (INT8U           ) APP_TASK_MAIN_PRIO,
                        (INT16U          ) APP_TASK_MAIN_PRIO,
                        (OS_STK        * )&Task_MainStk[0],
                        (INT32U          ) APP_TASK_MAIN_STK_SIZE,
                        (void          * )0,
                        (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
				}
			}
		
		}
		if(task_n.err_flag != 0)		   //之前发生了异常情况
		{
			if(task_n.delay_flag_500 >0)	 //500ms计数开始，同样，仅捕捉从1减到0这个状态。
			{
				task_n.delay_flag_500 --;
				if(0 == task_n.delay_flag_500)	//500ms延时到再进行任务重启是否成功的判断。
				{
					if(task_n.time_cnt == 0)	   //重启后500ms，任务依然没有喂软件狗
					{
						task_n.err_flag ++;
						task_n.delay_flag_500 = 30;
						OSTaskDel(5);//重启任务task_net			  //用删除任务再重新建立任务的方式重启任务。
						OSTaskCreateExt((void (*)(void *)) task_net,             // 创建网络进程
	                    (void          * ) 0,
	                    (OS_STK        * )&Task_NETStk[APP_TASK_NET_STK_SIZE - 1],
	                    (INT8U           ) APP_TASK_NET_PRIO,
	                    (INT16U          ) APP_TASK_NET_PRIO,
	                    (OS_STK        * )&Task_NETStk[0],
	                    (INT32U          ) APP_TASK_NET_STK_SIZE,
	                    (void          * )0,
	                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
					}
					else if(task_n.time_cnt >0)	//任务重启成功
					{
						task_n.err_flag = 0;
					}
					
					if(task_n.err_flag >= 3)		    //重启任务3次依然没有成功，重启CPU
					{
						NVIC_SystemReset();     //重启CPU;
					}		
				}
			}
		}
		else if(0 == task_n.err_flag)	   //之前没有发生异常情况，
		{
			if(task_n.time_cnt > 0) //只有值大于0时才进行操作，用于解决初始化时值为0的问题，这里只捕捉从1变为0的变化。
			{
				task_n.time_cnt --;
				if(0 == task_n.time_cnt) //底层任务3S都没有喂狗，说明任务已经挂，
				{
					task_n.err_flag ++;		  //任务故障计数，用以达到限定次数后重启CPU
					task_n.delay_flag_500 = 30;	  //用以在重启任务后延时500ms，再判断是否重启成功，可靠性增加。
					OSTaskDel(5);//重启任务task_net			  //用删除任务再重新建立任务的方式重启任务。
					OSTaskCreateExt((void (*)(void *)) task_net,             // 创建网络进程
	                    (void          * ) 0,
	                    (OS_STK        * )&Task_NETStk[APP_TASK_NET_STK_SIZE - 1],
	                    (INT8U           ) APP_TASK_NET_PRIO,
	                    (INT16U          ) APP_TASK_NET_PRIO,
	                    (OS_STK        * )&Task_NETStk[0],
	                    (INT32U          ) APP_TASK_NET_STK_SIZE,
	                    (void          * )0,
	                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK)); 
					}
			}
		
		}

		OSTimeDly(300); 						//延时300ms	，挂起自己，让其他任务执行	
	}
//看门狗进程实验结束
}
#endif
/******************************************************************************
* Function       :OS_CPU_SysTickClkFreq
* Author         :llemmx
* Date           :2011-05-03
* Description    :系统计时点计算，ucos的MCU频率计算。
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :INT32U - 返回MCU的工作频率
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
INT32U OS_CPU_SysTickClkFreq(void)
{
    RCC_ClocksTypeDef  rcc_clocks;
    RCC_GetClocksFreq(&rcc_clocks);

    return ((INT32U)rcc_clocks.HCLK_Frequency);
}

/******************************************************************************
* Function       :main
* Author         :llemmx
* Date           :2011-05-03
* Description    :工程入口函数，负责MCU上电初始化与创建第一个主进程。
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
int main()
{
	dev_init();  //这里必须得加上，不然系统不能正常启动
    OSInit();//初始化OS
    OSTaskCreateExt((void (*)(void *)) task_init,  // Create the start task.
                    (void          * ) 0,
                    (OS_STK        * )&Task_InitStk[APP_TASK_INIT_STK_SIZE - 1],
                    (INT8U           ) APP_TASK_INIT_PRIO,
                    (INT16U          ) APP_TASK_INIT_PRIO,
                    (OS_STK        * )&Task_InitStk[0],
                    (INT32U          ) APP_TASK_INIT_STK_SIZE,
                    (void          * )0,
                    (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
    OSStart();
}

