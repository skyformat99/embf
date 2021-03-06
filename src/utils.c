/****************************************************************************** 
* Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :utils.c
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

#include "includes.h"
#include "utils.h"
#include "stm32f4xx_gpio.h"
#include "fcntl.h"
#include "device_conf.h"
#include "dido.h"
#include "stm32f4xx_crc.h"//硬件CRC
#include "stm32f4xx_cryp.h"//AES加密

#define  E2_MAX_ADDR   (0x8000 - 63) //EEPROM保留最高64B不用
RTC_TimeTypeDef RTC_TimeStructure;
RTC_DateTypeDef RTC_DateStructure;

uint8_t g_prio = 0; //用来保存提升前的任务优先级，用以恢复优先级实现
uint8_t g_prio_open_flag = false;//确保一次只有一个任务使用提升优先级,初始化为false
uint8_t dsp_wake_buf[11] = {0}; //用来获取DSP唤醒信号的状态,新添71WIFI管脚nlink\nready的信息
uint8_t tmp_2crc_2bak[256] = {0}; //双备份双校验的数据缓冲区，最大支持256个字节的输入\读出
void iwdg_init(void);//声明，去除warning
/******************************************************************************
* Function       :embf_get_time
* Author         :wujing	
* Date           :2013-04-13
* Description    :读取时间接口,从内部RTC读取,耗时12us,时间由外部RTC同步,可以节省直接从外部RTC读取的8ms时间
*                :读出的数据类型为BCD码,buf[0~5]:年月日时分秒
* Calls          :RTC_GetTime()、RTC_GetDate()
* Table Accessed :无
* Table Updated  :无
* Input          :buf:指定读的地址
*                :count:要写入数据的个数,要求为6
* Output         :无
* Return         :成功返回true,失败返回false
*******************************************************************************
* History:        2013-04-13 
*------------------------------------------------------------------------------
* 2013-04-13 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t embf_get_time(uint8_t* buf, uint8_t count)  
{   
	int32_t ret = false;       
    if(6 == count)
    {
        RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);  //要先读时间 再读日期
        RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
        
        buf[0] = RTC_DateStructure.RTC_Year;
        buf[1] = RTC_DateStructure.RTC_Month;
        buf[2] = RTC_DateStructure.RTC_Date;

        buf[3] = RTC_TimeStructure.RTC_Hours;
        buf[4] = RTC_TimeStructure.RTC_Minutes;
        buf[5] = RTC_TimeStructure.RTC_Seconds;

        ret = true;
    }
    return ret;
}
/******************************************************************************
* Function       :embf_set_time
* Author         :wujing	
* Date           :2013-04-13
* Description    :设置时间接口,设置时同时设置内部RTC与外部RTC
*                :传入的数据类型为BCD码,buf[0~5]:年月日时分秒
* Calls          :RTC_SetTime()、RTC_SetDate()
* Table Accessed :无
* Table Updated  :无
* Input          :buf:指定写的地址
*                :count:要写入数据的个数,要求为6
* Output         :无
* Return         :成功返回true,失败返回false
*******************************************************************************
* History:        2013-04-13 
*------------------------------------------------------------------------------
* 2013-04-13 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t embf_set_time(uint8_t* buf, uint8_t count)  
{   
	int32_t ret = false;   
    int32_t tmp,tmp1= 0;
    if(6 == count)
    {
        if(drv_write(gt_glp.frtc, buf, 6) == 6) //外部RTC写成功,说明时间合法,可以写内部RTC
        {
            RTC_DateStructure.RTC_Year = buf[0];
            RTC_DateStructure.RTC_Month= buf[1];
            RTC_DateStructure.RTC_Date = buf[2];
            
            RTC_TimeStructure.RTC_Hours = buf[3];
            RTC_TimeStructure.RTC_Minutes = buf[4];
            RTC_TimeStructure.RTC_Seconds = buf[5];

            tmp = RTC_SetTime(RTC_Format_BCD,&RTC_TimeStructure); //写RTC寄存器有返回值
            tmp1 = RTC_SetDate(RTC_Format_BCD,&RTC_DateStructure);
            if((tmp == 1)&&(tmp1 == 1))
            {
                ret = true;
            }
        }
    }      
    return ret;
}
/****************************************************************************************
 ** 函数名称: embf_write_bytes
 ** 功能描述: 数组的带校验写入,写入后读出比较,如果不一致则重写,重写达到一定次数后失败退出 
 ** 参    数: data_address-数组存放地址
 **           data_address_bak-数组备份存放地�
 **           inputdata-待写入数组
 **           length-写入数值的字节数:1-8
 **           crc_address-crc校验存放地址
 **           crc_address_bak-校验备份存放地址
 **           crc_type-crc类型:crc8或者crc16
 **           rewrite_nub:读写不一致时重试次数
 ** 返 回 值: 操作状态
 **           PARAMETER_ERROR -输入参数错误
 **           SUCCEE 操作成功
 **           FAIL   操作失败
 ** 作　  者: maxm
 ** 日  　期: 2012年3月13日
 ** 版    本: V1.0
 **---------------------------------------------------------------------------------------
 ** 修 改 人:wujing 
 ** 日　  期:2013.03.28
 ** 修改内容:将其移入平台代码,去除最多8个字节的限制
 **--------------------------------------------------------------------------------------
 ****************************************************************************************/

int32_t embf_write_bytes(uint16_t data_address,
           uint16_t data_address_bak,
           uint8_t *iuputdata, 
           uint8_t length, 
           uint16_t crc_address, 
           uint16_t crc_address_bak,
           uint8_t crc_type, 
           uint8_t rewrite_nub)
{
    uint16_t  tmp = 0;
  //  uint8_t  tmp_arr_bak[8] = {7,8,9,10,11,12,15,16};  //write&read共用缓冲区 tmp_2crc_2bak[256]
    static uint8_t  tmp_crc_arr[4] = {1,2,3,4};     //初始化为不同值,防止写入读出出错时变量值没有改变导致比较通过
    static uint8_t  tmp_crc_arr_bak[4] = {7,8,9,10};
    uint8_t  i =0;
    uint8_t  state = 0;
    uint8_t  state1 = 0;
    uint8_t  count = 0;
    //fenglk 20120811　增加喂狗
    //extern void IWDG_ReloadCounter(void);
//     if(length>8||length==0)  
//     {
//         return (u8)-3;    //参数错误,超出函数处理能力
//     }

    if((data_address>E2_MAX_ADDR-length+1)||   //最大地址是0x8000 - 32
       (data_address_bak>E2_MAX_ADDR-length+1)||
       (crc_address>E2_MAX_ADDR-crc_type+1)||
       (crc_address_bak>E2_MAX_ADDR-crc_type+1)    
       )
    {
        return OPFAULT;   //参数错误 地址错误
    }

    if(crc_type == EMBF_CRC16)            //crc校验选择
    {
        tmp = embf_get_crc16(iuputdata , length);   
    }

    else if(crc_type == EMBF_CRC8)
    {
        tmp = embf_get_crc8(iuputdata , length);
    }

    else
    {
        return OPFAULT;   //参数错误 校验类型写错
    }

    embf_setu16_bigendian(tmp_crc_arr, (uint16_t)tmp);//0低1高 注意存储的高低字节顺序

    count = rewrite_nub;
    
    do         //写读比较,错误重写
    {
        state = 0;
        //fenglk 20120811　增加喂狗
        //IWDG_ReloadCounter();
        
        write_eeprom(data_address, iuputdata, length);
        write_eeprom(crc_address, tmp_crc_arr, crc_type);
        read_eeprom(data_address, tmp_2crc_2bak, length);
        read_eeprom(crc_address, tmp_crc_arr_bak, crc_type);
        
        for(i=0;i<length;i++)
        {
            if(iuputdata[i] != tmp_2crc_2bak[i])
            {
                state = 1;
            }
        }

        for(i=0;i<crc_type;i++)
        {
            if(tmp_crc_arr[i] != tmp_crc_arr_bak[i])
            {
                state = 1;
            }
        }

        count--;
    }
    while(1==state && count>0 ); //有出错且容错次数不为0  state=1；

    count = rewrite_nub;

    do         //写读比较,错误重写
    {
        //fenglk 20120811　增加喂狗
        //IWDG_ReloadCounter(); 
        state1 = 0;
        write_eeprom(data_address_bak, iuputdata, length);
        write_eeprom(crc_address_bak, tmp_crc_arr, crc_type);
        read_eeprom(data_address_bak, tmp_2crc_2bak, length);
        read_eeprom(crc_address_bak, tmp_crc_arr_bak, crc_type);
        
        for(i=0;i<length;i++)
        {
            if(iuputdata[i] != tmp_2crc_2bak[i])
            {
                state1 = 1;
            }
        }

        for(i=0;i<crc_type;i++)
        {
            if(tmp_crc_arr[i] != tmp_crc_arr_bak[i])
            {
                state1 = 1;
            }
        }

        count--;
    }
    while(1==state1 && count>0 );

    if(1==state)
    {
        return OPFAULT;  //写失败退出
    }
    else if(1==state1)
    {
        return OPFAULT;  //写备份失败退出
    }
    else
    {
        return DSUCCESS;    //成功
    }

}
/****************************************************************************************
 ** 函数名称: embf_read_bytes
 ** 功能描述: 数组的带校验读出,读出进行crc校验,如果不一致则重读,重读达到一定次数后写入默认值
 ** 参    数: outputdata-读出数组
 **           data_address-数组存放地址
 **           data_address_bak-数组备份存放地址
 **           length-写入数值的字节数:1-8
 **           crc_address-crc校验存放地址
 **           crc_address_bak-校验备份存放地址
 **           crc_type-crc类型:crc8或者crc16
 **           data_def-默认数组
 ** 返 回 值: 操作状态
 **           PARAMETER_ERROR -输入参数错误
 **           SUCCEE 操作成功
 **           LOAD_DEF 载入默认值
 ** 作　  者: maxm
 ** 日  　期: 2012年3月13日
 ** 版    本: V1.0
 **---------------------------------------------------------------------------------------
 ** 修 改 人:wujing 
 ** 日　  期:2013.03.28
 ** 修改内容:去除只能有8字节输入的限制
 **--------------------------------------------------------------------------------------
 ****************************************************************************************/
int32_t  embf_read_bytes (uint16_t data_address,
               uint16_t data_address_bak,
               uint16_t crc_address, 
               uint16_t crc_address_bak,
               uint8_t *outputdata, 
               uint8_t length,
               uint8_t *data_def, 
               uint8_t crc_type,
               uint8_t reread_nub)
{
    uint16_t  tmp = 0,tmp_bak=0,tmpcrc = 0;
   // uint8_t  tmp_arr[8] = {7,8,9,10,11,12,15,16};
    static uint8_t  tmp_crc_arr[4] = {1,2,3,4};     //初始化为不同值,防止写入读出出错时变量值没有改变导致比较通过
    static uint8_t  tmp_crc_arr_bak[4] = {7,8,9,10};
    uint8_t  i =0;
//  u8  state = 0;
    uint8_t  count = 0;

    //fenglk 20120811　增加喂狗
   // extern void IWDG_ReloadCounter(void);

//     if((length>8)||(length==0))
//     {
//         return (u8)-3;    //参数错误,超出函数处理能力
//     }

    if((data_address>E2_MAX_ADDR-length+1)||   //最大地址是512
       (data_address_bak>E2_MAX_ADDR-length+1)||
       (crc_address>E2_MAX_ADDR-crc_type+1)||
       (crc_address_bak>E2_MAX_ADDR-crc_type+1)    
       )
    {
        return OPFAULT;   //参数错误 地址错误
    }

    if((crc_type!=EMBF_CRC16)&&(crc_type!=EMBF_CRC8))
    {
        return OPFAULT;   //参数错误 校验类型写错
    }

    count = reread_nub;

    do           //读错误重读
    {
        //fenglk 20120811　增加喂狗
        //IWDG_ReloadCounter();
        
        read_eeprom(data_address, tmp_2crc_2bak, length);
        read_eeprom(crc_address, tmp_crc_arr, crc_type);
        read_eeprom(crc_address_bak, tmp_crc_arr_bak, crc_type);
        if(crc_type == EMBF_CRC8)
        {
          tmp_crc_arr[1]=0;
          tmp_crc_arr_bak[1]=0;
        }

        tmp = embf_getu16_bigendian(tmp_crc_arr);     //读出的CRC   注意存储的高低字节顺序
        tmp_bak = embf_getu16_bigendian(tmp_crc_arr_bak);
        
        if(crc_type == EMBF_CRC16)            //crc校验选择
        {
            tmpcrc = embf_get_crc16(tmp_2crc_2bak , length);      //计算CRC
        }
    
        else
        {
            tmpcrc = embf_get_crc8(tmp_2crc_2bak , length);
        }

        count--;

    }while((tmp!=tmpcrc)&&(count>0));   //达到重读次数或者读校验正确退出

    if(tmp==tmpcrc)
    {       
        if(tmp!=tmp_bak)     //备份数据区crc如果和数据区crc不一致则对备份数据区数据及crc重写
        {
            write_eeprom(data_address_bak, tmp_2crc_2bak, length);
            write_eeprom(crc_address_bak, tmp_crc_arr, crc_type);    
        }

        for(i=0;i<length;i++)
        {
            *(outputdata+i) = tmp_2crc_2bak[i];
        }
        return DSUCCESS;    //成功
    }
    else
    {   
        count = reread_nub;
        
        do                               //备份数据区读错误重读
        {
            //fenglk 20120811　增加喂狗
            //IWDG_ReloadCounter();
            
            read_eeprom(data_address_bak, tmp_2crc_2bak, length);
            read_eeprom(crc_address_bak, tmp_crc_arr_bak, crc_type);
            if(crc_type == EMBF_CRC8)
            {
              tmp_crc_arr[1]=0;
              tmp_crc_arr_bak[1]=0;
            }
            tmp = embf_getu16_bigendian(tmp_crc_arr);           //注意存储的高低字节顺序
            tmp_bak = embf_getu16_bigendian(tmp_crc_arr_bak);
            
            if(crc_type == EMBF_CRC16)            //crc校验选择
            {
                tmpcrc = embf_get_crc16(tmp_2crc_2bak , length );      
            }
        
            else
            {
                tmpcrc = embf_get_crc8(tmp_2crc_2bak , length);
            }
    
            count--;
        }
    
        while((tmp_bak!=tmpcrc)&&(count>0));        //达到重读次数或者读校验正确退出
            
        if(tmp_bak==tmpcrc)
        {
            write_eeprom(data_address, tmp_2crc_2bak, length);
            write_eeprom(crc_address, tmp_crc_arr_bak, crc_type);

            for(i=0;i<length;i++)
            {
                *(outputdata+i) = tmp_2crc_2bak[i];
            }
            return DSUCCESS;    //成功                 
        }

        else        //数据区和备份数据区数据均读失败则对数据区和备份数据区写入默认值及crc
        {
            if(NULL != data_def)
            {
                for(i=0;i<length;i++)
                {
                    *(outputdata+i) = data_def[i];  //此处有风险
                }

                if(crc_type == EMBF_CRC16)            //crc校验选择
                {
                    tmp = embf_get_crc16(data_def , length);    
                }
            
                else
                {
                    tmp = embf_get_crc8(data_def , length);
                }
            
                embf_setu16_bigendian(tmp_crc_arr, (u16)tmp);//0低1高
                //fenglk 20120811　增加喂狗
                //IWDG_ReloadCounter();     
                write_eeprom(data_address, data_def, length);
                write_eeprom(crc_address, tmp_crc_arr, crc_type);
                write_eeprom(data_address_bak, data_def, length);
                write_eeprom(crc_address_bak, tmp_crc_arr, crc_type);

                return OPFAULT;    //载入默认值
            }
            return OPFAULT;
                                
        }
        
    }

}

/******************************************************************************
* Function       :embf_cryp_aes_ecb
* Author         :wujing
* Date           :2013.03.12
* Description    :AES加密函数封装
* Calls          :CRYP_AES_ECB()
* Input          :mode:encryption or decryption Mode.
*                      This parameter can be one of the following values:
*                      MODE_ENCRYPT: Encryption
*                      MODE_DECRYPT: Decryption
*                :key:Key used for AES algorithm
*                :keysize: length of the Key, must be a 128, 192 or 256.
*                :input: pointer to the Input buffer.
*                :ilength: length of the Input buffer, must be a multiple of 16.
*                :output: pointer to the returned buffer.               
* Output         :None
* Return         :1: Operation done
*                :0: Operation failed
*******************************************************************************
* History:
*------------------------------------------------------------------------------
* 2013.03.12 : 1.0.0 : wujing
* Modification   : 初始代码编写
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t embf_cryp_aes_ecb(uint8_t mode, uint8_t* key, uint16_t keysize, uint8_t* input, uint32_t ilength, uint8_t* output)
{
    uint8_t ret = 0;
    ret = CRYP_AES_ECB(mode, key, keysize, input, ilength, output);
    return ret;
}
/******************************************************************************
* Function       :embf_bcd_to_bin
* Author         :Linfei
* Date           :2012.09.21
* Description    :BCD码转换成十进制值
* Calls          :None
* Input          :bcd：待转换的BCD码
* Output         :None
* Return         :转换结果：十进制值
*******************************************************************************
* History:
*------------------------------------------------------------------------------
* 2012.09.21 : 1.0.0 : Linfei
* Modification   : 初始代码编写
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t embf_bcd_to_dec(uint8_t bcd)		
{
    return ((((bcd >> 4) & 0x0F) * 10) + (bcd & 0x0F));
}
/******************************************************************************
* Function       :embf_bin_to_bcd
* Author         :Linfei
* Date           :2012.09.21
* Description    :十进制值转换成BCD码
* Calls          :None
* Input          :None
* Output         :None
* Return         :转换结果：BCD码
*******************************************************************************
* History:
*------------------------------------------------------------------------------
* 2012.09.21 : 1.0.0 : Linfei
* Modification   : 初始代码编写
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t embf_dec_to_bcd(uint8_t dec)	    
{
    return (((dec / 10) << 4) | (dec % 10));
}
/******************************************************************************
* Function       :embf_setu16_bigendian
* Author         :wujing    
* Date           :2013.03.28
* Description    :将16位的数据转换为大端存储模式
* Calls          :None
* Input          :None
* Output         :None
* Return         :转换结果：大端模式
*******************************************************************************
* History:
*------------------------------------------------------------------------------
* 2013.03.28 : 1.0.0 : wujing
* Modification   : 初始代码编写
*------------------------------------------------------------------------------
******************************************************************************/
void embf_setu16_bigendian(void* value, uint16_t fact)   
{
    ((u8*)value)[0] = (fact>>8)&0xFF;
    ((u8*)value)[1] = (fact)&0xFF;
}
/******************************************************************************
* Function       :embf_getu16_bigendian
* Author         :wujing    
* Date           :2013.03.28
* Description    :将传入的大端数据转换成小端模式
* Calls          :None
* Input          :None
* Output         :None
* Return         :转换结果：小端模式数据
*******************************************************************************
* History:
*------------------------------------------------------------------------------
* 2013.03.28 : 1.0.0 : wujing
* Modification   : 初始代码编写
*------------------------------------------------------------------------------
******************************************************************************/
uint16_t embf_getu16_bigendian(void* value)   
{
    return (((u8*)value)[0]<<8)|
           (((u8*)value)[1]);
}
/***********************************************************************
 * Function       : embf_get_crc8
 * Author         : Xu Shun'an
 * Date           : 2011.06.27
 * Description    : 生成8位的CRC校验码
 * Calls          : None
 * Input          : id: 待校验的数组指针，len: 待校验的数组长度
 * Output         : None
 * Return         : crc: 8位校验码
************************************************************************/
uint8_t embf_get_crc8(uint8_t *id, uint8_t len)
{
  uint8_t i = 0;
  uint8_t crc = 0;
  while (len-- != 0)
  {
    for (i = 1; i != 0; i *= 2)
    {
      if ((crc &1) != 0)
      {
        crc /= 2;
        crc ^= 0x8c;
      }
      else
      {
        crc /= 2;
      }
      if ((*id &i) != 0)
      {
        crc ^= 0x8c;
      }
    }
    id++;
  }

  return crc;
}
/***********************************************************************
 * Function       : embf_get_crc32
 * Author         : linfei
 * Date           : 2011.10.31
 * Description    : 生成32位的CRC校验码，目前为ARM平台的硬件校验（非主流，
 *                  左移实现），若更换平台须修改该函数
 * Calls          : None
 * Input          : ptr: 待生成校验的数组指针
 *                  len: 待生成校验的数组长度
 * Output         : None
 * Return         : crc: 32位校验码，出错返回0
************************************************************************/
uint32_t embf_get_crc32(volatile uint8_t *ptr, uint16_t len)
{

    if(ptr == NULL || len < 1)
    {
        return 0xFFFFFFFF;
    }
    uint8_t rem = len%4;

    CRC_ResetDR();
    len -= rem;
    CRC_CalcBlockCRC((uint32_t *)ptr, len>>2);
    switch(rem)
    {
    case 1:
        CRC->DR = ptr[len];
        break;
    case 2:
        CRC->DR = ptr[len] + (ptr[len + 1]<<8);
        break;
    case 3:
        CRC->DR = ptr[len] + (ptr[len + 1]<<8) + (ptr[len + 2]<<16);
        break;
    default:
        break;
    }
    return CRC->DR;

//    return 0xFFFFFFFF;
}
/***********************************************************************
 * Function       : embf_get_crc16
 * Author         : Xu Shun'an
 * Date           : 2011.06.27
 * Description    : 生成16位的CRC校验码
 * Calls          : None
 * Input          : ptr: 待校验的数组指针，len: 待校验的数组长度
 * Output         : None
 * Return         : crc: 16位校验码
************************************************************************/
uint16_t embf_get_crc16(volatile uint8_t *ptr, uint16_t len) 
{ 
    uint8_t i; 
    uint16_t crc=0xFFFF; 
    if(len==0) 
    {
        len=1;
    } 
    while (len--)  
    {   
        crc ^= *ptr; 
        for (i=0; i<8; i++)  
        { 
            if (crc&1) 
            { 
                crc >>= 1;  
                crc ^= 0xA001; 
            }  
            else 
            {
                crc >>= 1;
            } 
        }         
        ptr++; 
    } 
    return (crc); 
} 


/*************************************************************************
 * Function       : CRC_CCITT
 * Author         : Xu Shun'an
 * Date           : 2011.06.27
 * Description    : 钒电池CRC校验函数
 * Calls          : None
 * Input          : pcData: 待校验的数组指针，dwSize: 待校验的数组长度
 * Output         : None
 * Return         : crc: 校验结果
**************************************************************************/
uint16_t CRC_CCITT(uint8_t *pcData, uint8_t dwSize)
{
      uint8_t i,j;
    uint16_t crc=0xffff;
    uint16_t Use;
    
    for (i=0; i<dwSize; i++)
    {
         crc = crc^pcData[i] ;
         for (j=0; j<8; j++)
         {
             Use=crc;
             if ((Use&0x01) == 0x01)
             {
               crc=(crc>>1)^0x8408;
             }
             else
             {
               crc=crc>>1;
             }
         }           
     }
    crc^=0xffff;
    return crc;
}

/*************************************************************************
 * Function       : embf_crccheck
 * Author         : Xu Shun'an
 * Date           : 2011.06.27
 * Description    : CRC校验是否正确的判断
 * Calls          : getCRC16
 * Input          : pbuf: 待判断校验的数组指针，len: 待判断校验的数组长度
 *                  crctype: CRC校验类型，=0 为CRC16，=1 为钒电池CRC校验
 * Output         : None
 * Return         : 0: 校验错误；1：校验成功
**************************************************************************/
uint8_t embf_crccheck(uint8_t* pbuf, uint16_t buflen, uint8_t crctype)
{
    if (buflen < 3)
    {
        return 0 ;
    }
    uint16_t crcresult=0;
    uint8_t tmp[2]={0}; 

    if (crctype == 0)
    {
        crcresult=embf_get_crc16(pbuf, buflen-2);     
    }
    else if (crctype == 1)
    {
        crcresult=CRC_CCITT(pbuf, buflen-2);    
    }
    else
    {
        return 0 ;  
    }

    tmp[1] = crcresult & 0xff; 
    tmp[0] = (crcresult >> 8) & 0xff;

    if ((pbuf[buflen-1] == tmp[0])&&(pbuf[buflen-2] == tmp[1]))
    {
        return 1 ;
    }
    return 0 ;
}
/******************************************************************************
* Function       :watch_dog_inter_open
* Author         :wujing	
* Date           :2012-12-18
* Description    :写EEPROM操作，在leek和write前后整体加关调度。
*                :注意:lseek起始均默认为SEEK_SET
* Calls          :iwdg_init();
* Table Accessed :无
* Table Updated  :无
* Input          :addr:指定写的地址
*                :buf[]:存放要写数据的指针
*                :count:要写入数据的个数
* Output         :无
* Return         :成功返回写的个数，失败返回-1
*******************************************************************************
* History:        2012-12-18 
*------------------------------------------------------------------------------
* 2012-12-18 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void watch_dog_inter_open(void)
{
    iwdg_init();
}
/******************************************************************************
* Function       :write_eeprom
* Author         :wujing	
* Date           :2012-12-18
* Description    :写EEPROM操作，在leek和write前后整体加关调度。
*                :注意:lseek起始均默认为SEEK_SET
* Calls          :OSSchedLock()、OSSchedUnlock()、drv_lseek()、drv_write()
* Table Accessed :无
* Table Updated  :无
* Input          :addr:指定写的地址
*                :buf[]:存放要写数据的指针
*                :count:要写入数据的个数
* Output         :无
* Return         :成功返回写的个数，失败返回-1
*******************************************************************************
* History:        2012-12-18 
*------------------------------------------------------------------------------
* 2012-12-18 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t write_eeprom(uint32_t addr, const void* buf, uint32_t count)  
{   
	int32_t ret;                     
    OSSchedLock();
    ret = drv_lseek(gt_glp.feeprom1,addr,SEEK_SET);
    if(ret == DSUCCESS)
    {
        ret = drv_write(gt_glp.feeprom1,buf,count);
    }
    OSSchedUnlock();
    return ret;
} 
/******************************************************************************
* Function       :read_eeprom
* Author         :wujing	
* Date           :2012-12-18
* Description    :读EEPROM操作，在leek和read前后整体加关调度。
*                :注意:lseek起始均默认为SEEK_SET
* Calls          :OSSchedLock()、OSSchedUnlock()、drv_lseek()、drv_read()
* Table Accessed :无
* Table Updated  :无
* Input          :addr:指定读的地址
*                :buf[]:存放读取数据的指针
*                :count:要读取数据的个数
* Output         :无
* Return         :成功返回读取数据的个数，失败返回-1
*******************************************************************************
* History:        2012-12-18 
*------------------------------------------------------------------------------
* 2012-12-18 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t read_eeprom(uint32_t addr, void* buf, uint32_t count)  
{   
	int32_t ret ;                     
    OSSchedLock();
    ret = drv_lseek(gt_glp.feeprom1,addr,SEEK_SET);
    if(ret == DSUCCESS)
    {
        ret = drv_read(gt_glp.feeprom1,buf,count);
    }
    OSSchedUnlock();
    return ret;
} 
/******************************************************************************
* Function       :read_flash
* Author         :wujing	
* Date           :2012-12-18
* Description    :读FLASH操作，在leek和read前后整体加关调度。
*                :注意:lseek起始均默认为SEEK_SET
* Calls          :OSSchedLock()、OSSchedUnlock()、drv_lseek()、drv_read()
* Table Accessed :无
* Table Updated  :无
* Input          :addr:指定写的地址
*                :buf[]:存放要写数据的指针
*                :count:要写入数据的个数
* Output         :无
* Return         :成功返回写的个数，失败返回-1
*******************************************************************************
* History:        2012-12-18 
*------------------------------------------------------------------------------
* 2012-12-18 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t read_flash(uint32_t addr, void* buf, uint32_t count)  
{   
	int32_t ret ;                     
    OSSchedLock();
    ret = drv_lseek(gt_glp.fflash1,addr,SEEK_SET);
    if(ret == DSUCCESS)
    {
        ret = drv_read(gt_glp.fflash1,buf,count);
    }
    OSSchedUnlock();
    return ret;
} 
/******************************************************************************
* Function       :write_flash
* Author         :wujing	
* Date           :2012-12-18
* Description    :写FLASH操作，在leek和write前后整体加关调度。
*                :注意:lseek起始均默认为SEEK_SET
* Calls          :OSSchedLock()、OSSchedUnlock()、drv_lseek()、drv_write()
* Table Accessed :无
* Table Updated  :无
* Input          :addr:指定写的地址
*                :buf[]:存放要写数据的指针
*                :count:要写入数据的个数
* Output         :无
* Return         :成功返回写的个数，失败返回-1
*******************************************************************************
* History:        2012-12-18 
*------------------------------------------------------------------------------
* 2012-12-18 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t write_flash(uint32_t addr, const void* buf, uint32_t count)  
{   
	int32_t ret ;                     
    OSSchedLock();
    ret = drv_lseek(gt_glp.fflash1,addr,SEEK_SET);
    if(ret == DSUCCESS)
    {
        ret = drv_write(gt_glp.fflash1,buf,count);
    }
    OSSchedUnlock();
    return ret;
} 
/******************************************************************************
* Function       :clear_flash
* Author         :wujing	
* Date           :2012-12-18
* Description    :擦除FLASH操作，需加关调度保护。
* Calls          :OSSchedLock()、OSSchedUnlock()、drv_ioctl()
* Table Accessed :无
* Table Updated  :无
* Input          :opcode:相应的操作指令，如整片擦除、块擦除等
*                :para:擦除4KB时候需要设置的起始擦除地址
* Output         :无
* Return         :成功返回写的个数，失败返回-1
*******************************************************************************
* History:        2012-12-18 
*------------------------------------------------------------------------------
* 2012-12-18 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t clear_flash(uint32_t opcode, void *para)  
{    
	int32_t ret ;                       
    OSSchedLock();                                //要加喂狗
    ret = drv_ioctl(gt_glp.fflash1,opcode,para);
    OSSchedUnlock();
    return ret;

} 
/******************************************************************************
* Function       :task_prio_prompt
* Author         :wujing	
* Date           :2012-12-4
* Description    :更改任务自身优先级至三个任务中最高，确保一次只有一个任务调用
*				 :必须与task_prio_recover()成对调用
* Calls          :OSTaskChangePrio()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :成功返回0，失败返回-1
*******************************************************************************
* History:        2012-12-04 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t task_prio_prompt(void)
{
	int32_t ret = -1;
	uint32_t tmp;
	uint32_t tmp1;
	OS_TCB task_data;//用来获取当前任务优先级
	if(false == g_prio_open_flag) //只有为false时才执行
	{	
		tmp1 = OSTaskQuery(OS_PRIO_SELF,&task_data);
		if(0U == tmp1)	 //执行成功
		{
			g_prio = task_data.OSTCBPrio; //保存当前任务优先级
			tmp =  OSTaskChangePrio(OS_PRIO_SELF,1);//将当前任务提升为优先级1
			if(0U == tmp)
			{
				ret = 0;
				g_prio_open_flag = true;	
			}
		}
	}
	return ret;
}
/******************************************************************************
* Function       :task_prio_recover
* Author         :wujing	
* Date           :2012-12-4
* Description    :恢复自身优先级至优先级提升之前，必须与task_prio_prompt()成对调用
* Calls          :OSTaskChangePrio()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :成功返回0，失败返回-1
*******************************************************************************
* History:        2012-12-04 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t task_prio_recover(void)
{
	int32_t ret = -1;
	uint32_t tmp;
	if(true == g_prio_open_flag) //只有为true时才执行
	{	
		tmp =  OSTaskChangePrio(OS_PRIO_SELF,g_prio);//将当前任务恢复到原来状态
		if(0U == tmp)
		{
			ret = 0;
			g_prio_open_flag = false;	
		}		
	}
	return ret;
}
/******************************************************************************
* Function       :embf_get_stksize_used
* Author         :wujing	
* Date           :2012-11-2
* Description    :获取任务堆栈已使用部分的大小。
* Calls          :OSTaskStkChk()
* Table Accessed :无
* Table Updated  :无
* Input          :指定任务优先级，如果是自身的话：OS_PRIO_SELF
* Output         :无
* Return         :读取失败返回:OPFAULT;读取成功返回：已经使用的堆栈大小
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t embf_get_stksize_used(uint8_t prio)
{
	int32_t ret = OPFAULT;
	uint8_t err = 0u;
	OS_STK_DATA stk_data;
	err = OSTaskStkChk(prio,&stk_data);
	if(err ==OS_ERR_NONE)
	{
		ret = (int32_t)stk_data.OSUsed;
	}
	return ret;
}
/******************************************************************************
* Function       :embf_get_tick
* Author         :llemmx	
* Date           :2012-11-2
* Description    :获取当前的OSTime值，OSTime在每个时钟节拍中执行加1动作
* Calls          :OSTimeGet()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :读取的OSTime值。
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint32_t embf_get_tick(void)
{
    return OSTimeGet();
}
/******************************************************************************
* Function       :embf_delay
* Author         :llemmx	
* Date           :2012-11-2
* Description    :延时指定的Tick数
* Calls          :OSTimeDly()
* Table Accessed :无
* Table Updated  :无
* Input          :需要延时的时间，以Tick为单位
* Output         :无
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void embf_delay(uint32_t p_time)
{
    OSTimeDly(p_time);
}
/******************************************************************************
* Function       :embf_get_usage
* Author         :llemmx	
* Date           :2012-11-2
* Description    :获取CPU占用率
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :返回全局变量
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t embf_get_usage(void)
{
    return OSCPUUsage;
}
/******************************************************************************
* Function       :embf_get_current_ver
* Author         :llemmx	
* Date           :2012-11-2
* Description    :获取平台版本号及相应PCB序列号
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :size: 需读取的字符串长度
* Output         :buf :	读取结果存储缓冲区
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void embf_get_current_ver(char *buf,uint8_t size)
{
    if ((size>strlen(EMBF_VER))&&(NULL!=buf)){
        strcpy(buf,EMBF_VER);
    }
}

#if (defined P_P_000071) || (defined P_P_000069)
/******************************************************************************
* Function       :io_ctrl
* Author         :llemmx	
* Date           :2012-11-2
* Description    :控制指示灯、休眠控制、电源控制相应IO口，对于指示灯来说，高电平点亮
* Calls          :GPIO_SetBits()，GPIO_ResetBits()
* Table Accessed :无
* Table Updated  :无
* Input          :io: IO口对应的宏定义
* Output         :flag : UTILS_TO_HIGH-置高电平; UTILS_TO_LOW-置低电平。
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void io_ctrl(uint32_t io,uint8_t flag)
{
    switch(io){
    case UTILS_IO_LEDGREEN:   //wujing 2012.10.30 exchange the location of UTILS_IO_LEDGREEN and UTILS_IO_LEDRED.
        if (UTILS_IO_HIGH==flag){
            dido_ioctl(LED_GREEN|DO_STATE_HIGH,NULL);	  
        }else{
            dido_ioctl(LED_GREEN|DO_STATE_LOW,NULL);	 
        }
        break;
    case UTILS_IO_LEDRED:     //wujing 2012.10.30 exchange the location of UTILS_IO_LEDGREEN and UTILS_IO_LEDRED.
        if (UTILS_IO_HIGH==flag){
            dido_ioctl(LED_RED|DO_STATE_HIGH,NULL);	 
        }else{
            dido_ioctl(LED_RED|DO_STATE_LOW,NULL);	 
        }
        break;
    case UTILS_IO_WAKE_DSP_OUT:             //唤醒DSP输出引脚   
        if (UTILS_IO_HIGH==flag){
            dido_ioctl(HMI_A|DO_STATE_HIGH,NULL);	
        }else{
            dido_ioctl(HMI_A|DO_STATE_LOW,NULL);	
        }
        break;
    case UTILS_IO_PWRIO:                   //外设电源控制管脚
        if (UTILS_IO_HIGH==flag){
            dido_ioctl(POWER_IO_5V|DO_STATE_HIGH,NULL);
        }else{
            dido_ioctl(POWER_IO_5V|DO_STATE_LOW,NULL);	
        }
        break;
    case UTILS_IO_WIFI_RESET:				 //WIFI_RESET输出管脚的控制
        if (UTILS_IO_HIGH==flag){
            dido_ioctl(WIFI_RESET|DO_STATE_HIGH,NULL);
        }else{
            dido_ioctl(WIFI_RESET|DO_STATE_LOW,NULL);	
        }
        break;
    case UTILS_IO_WIFI_PROG:				 //WIFI_PROG输出管脚的控制
        if (UTILS_IO_HIGH==flag){
            dido_ioctl(WIFI_PROG|DO_STATE_HIGH,NULL);
        }else{
            dido_ioctl(WIFI_PROG|DO_STATE_LOW,NULL);	
        }
        break;
    default:
        break;
    }
}
/******************************************************************************
* Function       :io_get
* Author         :llemmx	
* Date           :2012-11-2
* Description    :读取指示灯、休眠控制、电源控制相应IO口状态
* Calls          :GPIO_ReadInputDataBit()
* Table Accessed :无
* Table Updated  :无
* Input          :io: IO口对应的宏定义
* Output         :无
* Return         :对应IO口的电平：低-0; 高-1
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t io_get(uint32_t io)
{
    uint8_t ret;
    switch(io){
    case UTILS_IO_LEDGREEN:                         //获取绿灯输出脚的电平情况
        ret=dido_ioctl(LED_GREEN|DO_STATE_GET,NULL);
        break;
    case UTILS_IO_LEDRED:                           //获取红灯输出脚的电平情况
        ret=dido_ioctl(LED_RED|DO_STATE_GET,NULL);
        break;
    case UTILS_IO_WAKE_DSP_OUT:                         //获取液晶唤醒DSP输出脚的电平情况 :HMI_A
        ret=dido_ioctl(HMI_A|DO_STATE_GET,NULL);
        break;
    case UTILS_IO_PWRIO:                            //获取外设电源控制输出脚的电平情况:IO_5V
        ret=dido_ioctl(POWER_IO_5V|DO_STATE_GET,NULL);
        break;
    case UTILS_IO_WIFI_RESET:                        //获取WIFI_RESET输出脚的电平情况:RST_WIFI
        ret=dido_ioctl(WIFI_RESET|DO_STATE_GET,NULL);
        break;
    case UTILS_IO_WAKE_HMI_IN:                      //获取DSP唤醒液晶输入脚的电平情况 :DSP_A
        dido_read(dsp_wake_buf, 8);
        if(0 == dsp_wake_buf[7])
        {
            ret = 0;
        }
        else
        {
            ret = 1;
        }
        break;
     case WIFI_LINK_STA:                      //获取WIFI n_Link状态
      //  dido_read(dsp_wake_buf, 9);
        ret=GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8); //此处封装不好 2013.04.02
        break;
     case WIFI_READY_STA:                      //获取WIFI n_Ready状态
      //  dido_read(dsp_wake_buf, 10);
        ret=GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_9);//此处封装不好  2013.04.02
        break;
    default:
        ret=0U;
        break;
    }
    return ret;
}
#endif
#ifdef P_A_000092
/******************************************************************************
* Function       :io_ctrl
* Author         :llemmx	
* Date           :2012-11-2
* Description    :控制指示灯、休眠控制、电源控制相应IO口，对于指示灯来说，高电平点亮
* Calls          :GPIO_SetBits()，GPIO_ResetBits()
* Table Accessed :无
* Table Updated  :无
* Input          :io: IO口对应的宏定义
* Output         :flag : UTILS_TO_HIGH-置高电平; UTILS_TO_LOW-置低电平。
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void io_ctrl(uint32_t io,uint8_t flag)
{
    switch(io)
	{
    case UTILS_IO_LED_RUN:   
        if (UTILS_IO_HIGH==flag)
        {
            dido_ioctl(RUN_LED|DO_STATE_HIGH,NULL);  //运行灯灭
        }
        else
        {
            dido_ioctl(RUN_LED|DO_STATE_LOW,NULL); //运行灯亮
        }
        break;
    default:
        break;
    }
}
#endif

