#ifndef _MODBUS_H_
#define _MODBUS_H_ 

#include "global.h"

//MODBUS 命令
#define MODBUS_CMD_02 0x02 //读离散量输入
#define MODBUS_CMD_03 0x03 //读保持寄存器
#define MODBUS_CMD_04 0x04 //读输入寄存器
#define MODBUS_CMD_05 0x05 //写单个线圈
#define MODBUS_CMD_06 0x06 //写单个寄存器
#define MODBUS_CMD_15 0x0F //写多个线圈
#define MODBUS_CMD_16 0x10 //写多个寄存器
#define MODBUS_CMD_20 0x14 //读文件记录
#define MODBUS_CMD_21 0x15 //写文件记录
#define MODBUS_CMD_22 0x16 //屏蔽写寄存器
#define MODBUS_CMD_23 0x17 //读/写多个寄存器
#define MODBUS_CMD_43 0x2B //读设备识别码

//modbus 差错码
#define MODBUS_ERROR  0x80
//MODBUS 异常码
#define MODBUS_UNUSED 0x01  //非法功能
#define MODBUS_ABADDR 0x02  //非法数据地址
#define MODBUS_DATAER 0x03  //非法数据值
#define MODBUS_DEVICE 0x04  //从站设备故障
#define MODBUS_SURE   0x05  //确认
#define MODBUS_BUSY   0x06  //从属设备忙
#define MODBUS_FLASH  0x07  //存储奇偶性差错

//Options
#define MODBUS_RTU    0x01 //set modbus rtu mode
#define MODBUS_TCP    0x02 //set modbus tcp mode

typedef void (*ModbusMCallBack)(uint16_t addr,uint8_t *buf);

typedef struct {
    uint8_t type;       //modbus type
    uint8_t dev_addruc; //device address
    uint8_t cmd;        //modbus command
    uint8_t *buf;       //send buf
    uint8_t len;        //buf size
    uint16_t start_addr;//start address
    uint16_t end_addr;  //end address
    ModbusMCallBack mcb;
}TPackageParam;

//interface
int Modbus_M_Package(TPackageParam *pp);

typedef void (*ModbusMRCallBack)(uint8_t addr,uint8_t cmd,uint8_t *data,uint8_t len);
typedef void (*ModbusMECallback)(uint8_t addr,uint8_t cmd,uint8_t ercode);
void Modbus_M_Analyst(uint8_t type,uint8_t *data,uint8_t len,ModbusMRCallBack msb,ModbusMECallback mec);

typedef struct {
    uint8_t typeuc;//type
	uint8_t addr; //devices address
	uint8_t cmd;  //modbus command
	uint8_t len;  //modbus data length
	uint8_t slen; //send buffer size
	uint8_t *data;//modbus data
	uint8_t *sbuf;//send buffer
    uint8_t *tcp_headp;
}TAnalystParam;
typedef int (*ModbusSRCallBack)(TAnalystParam *ap,uint8_t *err);
int Modbus_S_Analyst(uint8_t type,uint8_t *data,uint8_t len,uint8_t *sbuf,uint8_t slen,ModbusSRCallBack msb);

typedef struct {
    uint8_t typeuc;
    uint8_t addruc;
    uint8_t cmduc;
    uint8_t erruc;
    uint8_t lenuc;
    uint8_t slenuc;
    uint8_t *datauc;
    uint8_t *sbufuc;
}TSPackageParam;
int Modbus_S_Package(TSPackageParam *spp);

uint16_t getCRC16(uint8_t *ptr,uint8_t len);
int Modbus_S_get_address(uint8_t type,uint8_t *data,uint8_t len);
int Modbus_get_id(uint8_t type,uint8_t *data,uint8_t len);
#endif

