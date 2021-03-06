#ifndef DEVICE_CONF_H__
#define DEVICE_CONF_H__

//dido的板级不同
#if (defined P_P_000071) || (defined P_P_000069)
    #define DIG_OUT1     0x00      //继电器输出1
    #define DIG_OUT2     0x01      //继电器输出2
    #define LED_RED      0x02      //红灯输出
    #define LED_GREEN    0x03      //绿灯输出
    #define POWER_IO_5V  0x04      //外设电源控制输出
    #define HMI_A        0x05      //液晶唤醒DSP输出
    #define WIFI_RESET   0x06      //WIFI复位输出
    #define WIFI_PROG    0x07      //WIFI恢复出厂值控制
#endif

#ifdef  P_A_000092
    #define DIG_OUT1 0x00
    #define DIG_OUT2 0x01
    #define DIG_OUT3 0x02
    #define DIG_OUT4 0x03
    #define DIG_OUT5 0x04
    #define FAN1_OUT 0x05
    #define FAN2_OUT 0x06
    #define FAN3_OUT 0x07
    #define DO_P3    0x08
    #define RUN_LED  0x09
    #define DO_STATE_HIGH   0x0100U    //蔡工处使用了原来dido.h中的定义，此处保持应用程序的一致性
    #define DO_STATE_LOW    0x0200U    //
    
    /*主板数字输入，DI输入数组为buf[]
   DIG_IN1 - buf[0]
   DIG_IN2 - buf[1]
   DIG_IN3 - buf[2]
   DIG_IN4 - buf[3]
   DIG_IN5 - buf[4]
   DIG_IN6 - buf[5]
   扩展板数字输入:  
   DIG_IN1_C - buf[6]
   DIG_IN2_C - buf[7]
   DIG_IN3_C - buf[8]
   DIG_IN4_C - buf[9]
   DIG_IN5_C - buf[10]
   DIG_IN6_C - buf[11]
   DIG_IN7_C - buf[12]
   DIG_IN8_C - buf[13] */
    
#endif
//驱动定义
#ifdef  P_A_000092
	#define DRV_SYSTICK   0U
	#define DRV_USARTX    1U
	#define DRV_CANX      1U
	#define DRV_EEPROM    1U
	#define DRV_W25Qx     1U
	#define DRV_ADC       1U
	#define DRV_RTC       1U
	#define DRV_ADS1247   1U
	#define DRV_EXTWDG    1U
	#define DRV_KEY       0U
	#define DRV_LM240160K 0U
	#define DRV_NET       1U
	#define DRV_DIDO      1U
	#define DRV_METER     1U
    #define NIC_DP83848
    #define EEPROM_FM25W256 //IO-D的EEPROM使用FM25W256不需页等待，速度快
#endif


#ifdef  P_P_000069
	#define DRV_SYSTICK   0U
	#define DRV_USARTX    1U
	#define DRV_CANX      1U
	#define DRV_EEPROM    1U
	#define DRV_W25Qx     1U
	#define DRV_ADC       0U
	#define DRV_RTC       1U
	#define DRV_ADS1247   0U
	#define DRV_EXTWDG    1U
	#define DRV_KEY       1U
	#define DRV_LM240160K 1U
	#define DRV_NET       1U
	#define DRV_DIDO      1U
    #define NIC_LAN9303
  //  #define EEPROM_FM25W256 71的EEPROM需要页等待，为BRSW256
#endif

#ifdef  P_P_000071
	#define DRV_SYSTICK   0U
	#define DRV_USARTX    1U
	#define DRV_CANX      1U
	#define DRV_EEPROM    1U
	#define DRV_W25Qx     1U
	#define DRV_ADC       0U
	#define DRV_RTC       1U
	#define DRV_ADS1247   0U
	#define DRV_EXTWDG    1U
	#define DRV_KEY       1U
	#define DRV_LM240160K 1U
	#define DRV_NET       1U
	#define DRV_DIDO      1U
    #define NIC_LAN9303
  //  #define EEPROM_FM25W256 71的EEPROM需要页等待，为BRSW256
#endif


#ifdef P_N_000159
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
#endif

#endif
