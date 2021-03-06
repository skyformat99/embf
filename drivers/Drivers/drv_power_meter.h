#ifndef __POWER_METER_H
#define __POWER_METER_H

#include "stm32f4xx.h"
#include "fcntl.h"
#include "math.h"
#include "io_interface.h"

// 端口定义
// #define   ATT7022B_RESET_PIN    GPIO_Pin_8
// #define   ATT7022B_RESET_PORT   GPIOG    

// #define   ATT7022B_CS_PIN       GPIO_Pin_12
// #define   ATT7022B_CS_PORT      GPIOA    

// #define   ATT7022B_SCLK_PIN     GPIO_Pin_11
// #define   ATT7022B_SCLK_PORT    GPIOA
//      
// #define   ATT7022B_DIN_PIN 		GPIO_Pin_6
// #define   ATT7022B_DIN_PORT     GPIOC

// #define   ATT7022B_DOUT_PIN 	GPIO_Pin_6
// #define   ATT7022B_DOUT_PORT    GPIOG

#define  CT_RATE    4	 // 调节电流和功率的系数

//读表寄存器定义
#define  r_Pa         0x01 //A相有功功率
#define  r_Pb         0x02 //B相有功功率
#define  r_Pc         0x03 //C相有功功率
#define  r_Pt         0x04 //合相有功功率

#define  r_Qa         0x05 //A相无功功率
#define  r_Qb         0x06 //B相无功功率
#define  r_Qc         0x07 //C相无功功率
#define  r_Qt         0x08 //合相无功功率

#define  r_Sa         0x09 //A相视在功率
#define  r_Sb         0x0A //B相视在功率
#define  r_Sc         0x0B //C相视在功率
#define  r_St         0x0C //合相视在功率

#define  r_URmsa      0x0D //A相电压有效值
#define  r_URmsb      0x0E //B相电压有效值
#define  r_URmsc      0x0F //C相电压有效值

#define  r_IRmsa      0x10 //A相电流有效值
#define  r_IRmsb      0x11 //B相电流有效值
#define  r_IRmsc      0x12 //C相电流有效值
#define  r_IRmst      0x13 //ABC相电流矢量有效值

#define  r_Pfa        0x14 //A相功率因数
#define  r_Pfb        0x15 //B相功率因数
#define  r_Pfc        0x16 //C相功率因数
#define  r_Pft        0x17 //合相功率因数

#define  r_Pga        0x18 //A相电压与电流相角
#define  r_Pgb        0x19 //B相电压与电流相角
#define  r_Pgc        0x1A //C相电压与电流相角
#define  r_Pgt        0x1B //合相电压与电流相角
#define  r_Freq       0x1C //线频率
#define  r_EPa        0x1E //A相有功电能
#define  r_EPb        0x1F //B相有功电能
#define  r_EPc        0x20 //C相有功电能
#define  r_EPt        0x21 //合相有功电能
#define  r_Eqa        0x22 //A相无功电能
#define  r_Eqb        0x23 //B相无功电能
#define  r_Eqc        0x24 //C相无功电能
#define  r_Eqt        0x25 //合相无功电能
#define  r_RSPIData   0x28 //上一次SPI读出的数据
#define  r_RmsADC7    0x29 //第七路ADC输入信号的有效值
#define  r_TempD      0x2A //温度传感器的输出
#define  r_URmst      0x2B //ABC电压矢量和的有效值
#define  r_sFlag      0x2C //存放断相、相序、SIG等标志状态
#define  r_WSPIData1  0x2D //上一次SPI写入的数据
#define  r_WSPIData2  0x2E //上一次SPI写入的数据
#define  r_EFlag      0x30 //电能寄存器的工作状态
#define  r_EPa2       0x31 //A相有功电能，读后清零
#define  r_EPb2       0x32 //B相有功电能，读后清零
#define  r_EPc2       0x33 //C相有功电能，读后清零
#define  r_EPt2       0x34 //合相有功电能，读后清零
#define  r_Eqa2       0x35 //A相无功电能，读后清零
#define  r_Eqb2       0x36 //B相无功电能，读后清零
#define  r_Eqc2       0x37 //C相无功电能，读后清零
#define  r_Eqt2       0x38 //合相无功电能，读后清零
#define  r_LEFlag     0x3C //基波电能寄存器的工作状态
#define  r_PFlag      0x3D //有功和无功功率方向，正向为0，负向为1
#define  r_ChkSum1    0x3E //较表数据较验寄存器
#define  r_PosEpa     0X40 //A相正向有功电能寄存器
#define  r_PosEpb     0X41 //B相正向有功电能寄存器
#define  r_PosEpc     0X42 //C相正向有功电能寄存器
#define  r_PosEpt     0X43 //合相正向有功电能寄存器
#define  r_NegEpa     0X44 //A相反向有功电能寄存器
#define  r_NegEpb     0X45 //B相反向有功电能寄存器
#define  r_NegEpc     0X46 //C相反向有功电能寄存器
#define  r_NegEpt     0X47 //合相反向有功电能寄存器
#define  r_PosEqa     0X48 //A相正向无功电能寄存器
#define  r_PosEqb     0X49 //B相正向无功电能寄存器
#define  r_PosEqc     0X4A //C相正向无功电能寄存器
#define  r_PosEqt     0X4B //合相正向无功电能寄存器
#define  r_NegEqa     0X4C //A相反向无功电能寄存器
#define  r_NegEqb     0X4D //B相反向无功电能寄存器
#define  r_NegEqc     0X4E //C相反向无功电能寄存器
#define  r_NegEqt     0X4F //合相反向无功电能寄存器
#define  r_PosEpa2    0X60 //A相正向有功电能寄存器，读后清零
#define  r_PosEpb2    0X61 //B相正向有功电能寄存器，读后清零
#define  r_PosEpc2    0X62 //C相正向有功电能寄存器，读后清零
#define  r_PosEpt2    0X63 //合相正向有功电能寄存器，读后清零
#define  r_NegEpa2    0X64 //A相反向有功电能寄存器，读后清零
#define  r_NegEpb2    0X65 //B相反向有功电能寄存器，读后清零
#define  r_NegEpc2    0X66 //C相反向有功电能寄存器，读后清零
#define  r_NegEpt2    0X67 //合相反向有功电能寄存器，读后清零
#define  r_PosEqa2    0X68 //A相正向无功电能寄存器，读后清零
#define  r_PosEqb2    0X69 //B相正向无功电能寄存器，读后清零
#define  r_PosEqc2    0X6A //C相正向无功电能寄存器，读后清零
#define  r_PosEqt2    0X6B //合相正向无功电能寄存器，读后清零
#define  r_NegEqa2    0X6C //A相反向无功电能寄存器，读后清零
#define  r_NegEqb2    0X6D //B相反向无功电能寄存器，读后清零
#define  r_NegEqc2    0X6E //C相反向无功电能寄存器，读后清零
#define  r_NegEqt2    0X6F //合相反向无功电能寄存器，读后清零

//较表寄存器定义
#define  w_Iregion1   0x02 //相位补偿区域设置1
#define  w_Iregion2   0x03 //相位补偿区域设置2
#define  w_Iregion3   0x04 //相位补偿区域设置3
#define  w_Iregion4   0x05 //相位补偿区域设置4
#define  w_PgainA0    0x06 //A相功率增益0 
#define  w_PgainB0    0x07 //B相功率增益0
#define  w_PgainC0    0x08 //C相功率增益0
#define  w_PgainA1    0x09 //A相功率增益1
#define  w_PgainB1    0x0A //B相功率增益1
#define  w_PgainC1    0x0B //C相功率增益1
#define  w_PhsregA0   0x0C //A相区域0相位较正
#define  w_PhsregA1   0x0D //A相区域1相位较正
#define  w_PhsregA2   0x0E //A相区域2相位较正
#define  w_PhsregA3   0x0F //A相区域3相位较正
#define  w_PhsregA4   0x10 //A相区域4相位较正
#define  w_PhsregB0   0x11 //B相区域0相位较正
#define  w_PhsregB1   0x12 //B相区域1相位较正
#define  w_PhsregB2   0x13 //B相区域2相位较正
#define  w_PhsregB3   0x14 //B相区域3相位较正
#define  w_PhsregB4   0x15 //B相区域4相位较正
#define  w_PhsregC0   0x16 //C相区域0相位较正
#define  w_PhsregC1   0x17 //C相区域1相位较正
#define  w_PhsregC2   0x18 //C相区域2相位较正
#define  w_PhsregC3   0x19 //C相区域3相位较正
#define  w_PhsregC4   0x1A //C相区域4相位较正
#define  w_UgainA     0x1B //A相电压较正
#define  w_UgainB     0x1C //B相电压较正
#define  w_UgainC     0x1D //C相电压较正
#define  w_Irechg     0x1E //比差补偿区域设置
#define  w_Istartup   0x1F //启动电流阀值设置
#define  w_HFConst    0x20 //高频脉冲输出设置
#define  w_IgainA     0x26 //A相电流较正
#define  w_IgainB     0x27 //B相电流较正
#define  w_IgainC     0x28 //C相电流较正
#define  w_FailVoltage 0x29 //失压阀值设置
#define  w_EAddMode    0x2A //合向能量累加模式选择
#define  w_GainAdc7    0x2B //第七路ADC有效值较正
#define  w_GCtrlT7Adc  0x2C //温度/第七路ADC选择控制
#define  w_EnLineFreq  0x2D //基波测量使能控制
#define  w_EnUAngle    0x2E //电压夹角测量使能控制
#define  w_SelectPQSU  0x2F //基波电压功率输出选择
#define  w_EnDtIorder  0x30 //电流相序检测使能控制
#define  w_LineFreqPg  0x31 //基波功率较正
#define  w_EnHarmonic  0x3C //基波测量与谐波测量选择切换
#define  w_HFDouble    0x3E //脉冲常数加倍选择
#define  w_UADCPga     0x3F //电压通道ADC增益选择

//对外接口 2012.11.29
#define  THREE_PHASE_THREE_WIRE 0x01
#define  THREE_PHASE_FOUR_WIRE  0x02


//缓冲区
typedef struct 
{
    uint8_t   Calibration_flag;      //初始为0X55 非校表状态	 0XAA 为校表状态
    uint8_t   Read_Meter_flag;       //初始为0X55 读谐波表标志  0XAA 为读全波表标志
    uint8_t   ATT7022B_Data_Adjust_flag;    //首次上电写表
    uint8_t   Read_Meter_count;         //全波采样次数计数
    uint8_t   next_day_flag;     //天切换标志 ，=1天切换
    uint8_t   next_day_Eqtflag;             //无功天切换标志 ，=1天切换
    uint8_t   Three_or_Four_PhaseFlag;	    //1:三相四线制读数据	   2：三相三线制读数据 
    uint8_t   CorrParaPow;         //有功与无功校正切换标志，=0，校正有功，=1校正无功
    uint8_t   ACPowFlag;//读有功和无功功率的方向
	
		uint16_t  Ua_Harmonic;	  //电压畸变率
    uint16_t  Ub_Harmonic;
    uint16_t  Uc_Harmonic;
    
    uint16_t  Ua_HarmRate;	  //电压畸变率百分比
    uint16_t  Ub_HarmRate;
    uint16_t  Uc_HarmRate;
    
    uint16_t  Pa_Harmonic;	  //功率畸变率
    uint16_t  Pb_Harmonic;
    uint16_t  Pc_Harmonic;
    
    uint32_t  Sa_Harmonic;	  //视在功率畸变率
    uint32_t  Sb_Harmonic; 
    uint32_t  Sc_Harmonic;
    
    uint32_t  Ia_Harmonic;	  //电流畸变率
    uint32_t  Ib_Harmonic; 
    uint32_t  Ic_Harmonic;
    
    uint32_t  Ia_HarmRate;	  //电流畸变率百分比
    uint32_t  Ib_HarmRate;
    uint32_t  Ic_HarmRate;
    
    uint32_t  dis_Pa;     //有功功率
    uint32_t  dis_Pb;
    uint32_t  dis_Pc;
    uint32_t  dis_Pt;
    
    uint32_t  dis_Qa;     //无功功率				
    uint32_t  dis_Qb;
    uint32_t  dis_Qc;
    uint32_t  dis_Qt;
    
    uint32_t  dis_Qa_PC;  //无功功率:校表时候使用				
    uint32_t  dis_Qb_PC;
    uint32_t  dis_Qc_PC;
    uint32_t  dis_Qt_PC;
    
    uint32_t  dis_Sa;     //视在功率
    uint32_t  dis_Sb; 
    uint32_t  dis_Sc;
    uint32_t  dis_St;
    
    uint32_t  dis_Pfa;    //功率因数
    uint32_t  dis_Pfb;
    uint32_t  dis_Pfc;
    uint32_t  dis_Pft;
    
    uint32_t  dis_Va;    //电压
    uint32_t  dis_Vb;
    uint32_t  dis_Vc;
    
    uint32_t  dis_Fre;   //频率
    
    uint32_t  dis_Ia;    //电流
    uint32_t  dis_Ib;
    uint32_t  dis_Ic;
    
    uint32_t  EPa;  //能量
    uint32_t  EPb;
    uint32_t  EPc;
    
    uint32_t  Eqt;	       //合相无功电能测量值
    uint32_t  PosEqt; 	   //合相正向无功电能
    uint32_t  PosEqt2;     //合相正向无功电能,读后清零
    uint32_t  PosEqt_last; //上一次正向无功电能的大小   
    uint32_t  NegEqt; 	   //合相反向无功电能
    uint32_t  NegEqt2;	   //合相反向无功电能,读后清零

    uint32_t  EPt;         //合相有功电能测量值
    uint32_t  dis_EPt;     //合相有功电能
    uint32_t  PosEpt;      //合相正向有功电能
    uint32_t  PosEpt2;     //合相正向有功电能,读后清零
    uint32_t  PosEpt_last; //上一次正向有功电能的大小    
    uint32_t  NegEpt;      //合相反向有功电能
    uint32_t  NegEpt2;     //合相反向有功电能,读后清零
 
    uint32_t QgainA0; //增益参数
    uint32_t QgainB0;
    uint32_t QgainC0;
    
    uint32_t PgainA0; 
    uint32_t PgainB0;
    uint32_t PgainC0;
     
    uint32_t UgainA; 
    uint32_t UgainB;
    uint32_t UgainC;

    uint32_t IgainA;
    uint32_t IgainB;
    uint32_t IgainC;
}GATTPara;


// 函数声明
int32_t ATT7022B_open(int32_t flag, int32_t mode);
int32_t ATT7022B_write(const uint8_t *command, uint32_t data);
int32_t ATT7022B_read(void *arg, uint32_t count);
int32_t ATT7022B_ioctl(uint32_t cmd, void *arg);

#endif /* _SPIFLASH_H_ */

