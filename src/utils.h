/****************************************************************************** 
* Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :utils.h
* Author         :llemmx
* Date           :2012-11-02
* Description    :操作系统函数的封装、常用函数的封装
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2012-11-2 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#ifndef UTILS_H_
#define UTILS_H_

#include "global.h"

typedef struct
{
    uint32_t StdId;
    uint32_t ExtId;
    uint8_t IDE;
    uint8_t RTR;
    uint8_t DLC;
    uint8_t Data[8];
    uint8_t FMI;
} t_embf_canmsgrx;

typedef struct
{
    uint32_t StdId;
    uint32_t ExtId;
    uint8_t IDE;
    uint8_t RTR;
    uint8_t DLC;
    uint8_t Data[8];
} t_embf_canmsgtx;

typedef enum
{
    EMBF_CRC8= 1,
    EMBF_CRC16 = 2,    
}E_CRC_TYPE;  //双备份\双校验所用

int32_t embf_get_stksize_used(uint8_t prio);
uint32_t embf_get_tick(void);
void embf_delay(uint32_t p_time);
uint8_t embf_get_usage(void);
void embf_get_current_ver(char *buf,uint8_t size);
void init_run(void);
void enter_low_power(void);
int32_t task_prio_prompt(void);
int32_t task_prio_recover(void);
int32_t write_eeprom(uint32_t addr, const void* buf, uint32_t count);//wujing 2012.12.18
int32_t read_eeprom(uint32_t addr, void* buf, uint32_t count);
int32_t read_flash(uint32_t addr, void* buf, uint32_t count);
int32_t write_flash(uint32_t addr, const void* buf, uint32_t count);
int32_t clear_flash(uint32_t opcode, void *para);
void watch_dog_inter_open(void);//打开内部看门狗

uint8_t embf_cryp_aes_ecb(uint8_t mode, uint8_t* key, uint16_t keysize, uint8_t* input, uint32_t ilength, uint8_t* output);//AES加密封装
uint8_t embf_bcd_to_dec(uint8_t bcd);//BCD码转换为十进制数
uint8_t embf_dec_to_bcd(uint8_t dec);//十进制数转换为BCD码
uint8_t embf_get_crc8(uint8_t *id, uint8_t len);//生成8位CRC校验码
uint32_t embf_get_crc32(volatile uint8_t *ptr, uint16_t len);//生成32位CRC检验码
uint16_t embf_get_crc16(volatile uint8_t *ptr, uint16_t len);//生成16位CRC检验码
uint16_t CRC_CCITT(uint8_t *pcData, uint8_t dwSize);//
uint8_t embf_crccheck(uint8_t* pbuf, uint16_t buflen, uint8_t crctype);//判断CRC校验是否正确
void embf_setu16_bigendian(void* value, uint16_t fact);//将16位数据转为大端模式存储
uint16_t embf_getu16_bigendian(void* value);//将数据转换为大端模式存储
int32_t embf_write_bytes(uint16_t data_address,uint16_t data_address_bak,uint8_t *iuputdata, uint8_t length, uint16_t crc_address, uint16_t crc_address_bak,uint8_t crc_type, uint8_t rewrite_nub);
int32_t embf_read_bytes (uint16_t data_address,uint16_t data_address_bak,uint16_t crc_address, uint16_t crc_address_bak,uint8_t *outputdata, uint8_t length,uint8_t *data_def, uint8_t crc_type,uint8_t reread_nub);
int32_t embf_get_time(uint8_t* buf, uint8_t count);
int32_t embf_set_time(uint8_t* buf, uint8_t count);
#if (defined P_P_000071) || (defined P_P_000069)
#define UTILS_IO_LEDRED   0x00000001u
#define UTILS_IO_LEDGREEN 0x00000002u
#define UTILS_IO_WAKE_DSP_OUT  0x00000004u     //唤醒DSP，为输出引脚   HMI_A
#define UTILS_IO_PWRIO    0x00000010u
#define UTILS_IO_WIFI_RESET 0x00000020u
#define UTILS_IO_WAKE_HMI_IN 0x00000040u       //唤醒液晶，输入引脚  DSP_A
#define UTILS_IO_WIFI_PROG   0x00000080u       //WIFI恢复出厂值管脚
#define WIFI_LINK_STA        0x00000100u       //WIFI nlink脚
#define WIFI_READY_STA       0x00000200u       //WIFI nReady管脚 
#endif
#ifdef P_A_000092
#define UTILS_IO_LED_RUN   0x00000001U
#endif
#define UTILS_IO_HIGH 0x01u
#define UTILS_IO_LOW  0x00u
void io_ctrl(uint32_t io,uint8_t flag);

#if (defined P_P_000071) || (defined P_P_000069)
uint8_t io_get(uint32_t io);
#endif

#define MODE_ENCRYPT             ((uint8_t)0x01) //AES宏添加 
#define MODE_DECRYPT             ((uint8_t)0x00)

#define DSUCCESS     0
#define OPFAULT      (-1)
#define DEVBUSY      (-2)

#endif
