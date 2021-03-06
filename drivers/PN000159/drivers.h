#ifndef DEVICE_H_
#define DEVICE_H_

#include "global.h"

//驱动定义
#define DRV_SYSTICK   0U
#define DRV_USARTX    1U
#define DRV_CANX      0U
#define DRV_EEPROM    1U
#define DRV_W25Qx     1U
#define DRV_ADC       0U
#define DRV_RTC       0U
#define DRV_ADS1247   0U
#define DRV_EXTWDG    1U
#define DRV_KEY       0U
#define DRV_LM240160K 0U
#define DRV_NET       1U
#define DRV_DIDO      0U
#define DRV_METER     0U

/*SysTick---------------------------------------------------------------------- */
#if DRV_SYSTICK
#include "systick.h"
#endif

#if  DRV_NET
#include "netconf.h"
#endif

/*SysTick---------------------------------------------------------------------- */
#if DRV_DIDO
#include "dido.h"
#endif

/* LCD0 -------------------------------------------------------------------------*/
#if DRV_LM240160K
#include "lm240160.h"
#endif

/*串口驱动--------------------------------------------------------------------- */
#if DRV_USARTX
#include "usartx.h"
#endif

/*can驱动---------------------------------------------------------------------- */
#if DRV_CANX
#include "canx.h"
#endif 

/*EEPROM驱动------------------------------------------------------------------- */
#if DRV_EEPROM
#include "EEPROM.h"
#endif

/*W25Qx------------------------------------------------------------------------ */
#if DRV_W25Qx
#include "W25Qx.h"
#endif

/*RTC驱动---------------------------------------------------------------------- */
#if DRV_RTC
#include "r8025.h"
#endif

/*DRV_ADC----------------------------------------------------------------------- */
#if DRV_ADC
#include "adc.h"
#endif


#if DRV_ADS1247
#include "ads1247.h"
#endif

#if DRV_EXTWDG
#include "extwdg.h"
#endif

#if DRV_KEY
#include "key.h"
#endif

#if DRV_METER
#include "drv_power_meter.h"
#endif

//IO interface
void init_interface(void);
void Ethernet_Configuration(void); /* 网络控制器初始化 */

//驱动用定时器(1ms)
//在这里加入驱动需要使用的定时器函数
//wujing 2012.11.22 for MISRA-2004
#define DRV_TIMER  \
	do{\
    UART_TIMER; \
    EEPROM_TIMER; \
}while(0) \

#endif

