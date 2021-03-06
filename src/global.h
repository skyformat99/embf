#ifndef GLOBALA_H_
#define GLOBALA_H_

//sys module include files
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
//UCOS
#include "includes.h"
#include "os_cpu.h"


#define TRUE  true
#define FALSE false
#define NULL  0
#define __IO  volatile                  /*!< defines 'read / write' permissions   */

#define SYS_OS_SUPPORT  1 //OS support
#define SYS_TEST        1 //open test
#define G_DEBUG         1 //open debug
#define USE_FULL_ASSERT 1 //open assert
#ifdef P_P_000071
	#define EMBF_VER "EMBF-V1-A(1.0.8)(P-P-000071)"
#endif
#ifdef P_P_000069
	#define EMBF_VER "EMBF-V1-A(1.0.8)(P-P-000069)"
#endif
#ifdef P_A_000092
	#define EMBF_VER "EMBF-V1-A(1.0.8)(P-A-000092)"
#endif


#if G_DEBUG
#define DBPRINTF(format, arg...) { printf("%s:%d::",__FILE__,__LINE__);printf(format, ## arg); }
#else
#define DBPRINTF(format, arg...)
#endif

#if (defined P_P_000071) || (defined P_P_000069)
typedef struct {
    uint8_t back_light_count;
    uint32_t ui_time;
}t_task_ui;
#endif

// typedef __packed struct ot{
//     uint8_t tm_year;
//     uint8_t tm_mon;
//     uint8_t tm_mday;
//     uint8_t tm_wday;
//     uint8_t tm_hour;
//     uint8_t tm_min;
//     uint8_t tm_sec;
//     uint8_t tm_usec;
// }t_time;
typedef struct {
    //t_time sys_time;       //system time 剔除
    uint32_t timercnt;     //tickcount
    uint32_t pwr_mode;     //power mode,1=low power,0=normal
    //drivers handle
    int32_t fserial1; // serial port1
    int32_t fserial2; // serial port2
    int32_t fserial3; // serial port3
    int32_t fserial4; // serial port4
    int32_t fserial5; // serial port5
    int32_t fserial6; //serial port6
    int32_t fsystick; // timer
    int32_t fflash1;  // spi flash
    int32_t feeprom1; // eeprom
    int32_t flcd;     // LCD
    int32_t fkey;     // key
    int32_t fadc1;    // ADC
    int32_t fads1247; // ext ADC
    int32_t fdcan1;   // CAN1
    int32_t fdcan2;   // CAN2
    int32_t frtc;     // RTC
    int32_t fextwdg;  // ext wdg
    int32_t fnet;     // net work
    int32_t fdido;    // DIDO
	int32_t fAtt7022B; //POWER METER //wujing 2012.12.11
}t_gparam,*p_gparam;

//看门狗实验变量定义_start
typedef struct
{
    uint32_t err_flag;      //异常标志
	uint32_t time_cnt;	    //计数变量
	uint32_t delay_flag_500;//500ms延时标志
} twatch_dog_ver;

extern twatch_dog_ver task_m ;
extern twatch_dog_ver task_n ;

#if (defined P_P_000071) || (defined P_P_000069)
	extern twatch_dog_ver task_u ;
#endif

extern volatile t_gparam gt_glp;
extern OS_EVENT *g_drv_events[];//the array length must more than max drivers
// extern volatile t_time gt_time;


#endif

