/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :rtc.c
 * Author         :Linfei
 * Date           :2012.07.13
 * Description    :DS1340C驱动，模拟I2C方式，在总线频率为168MHZ时为20KHZ，修改
  *                I2C_delay函数中的i变量值可调节波特率，具体用示波器观察
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2012.07.11 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 ********************************************************************************/
#include "rtc.h"
#include <time.h>

/* RTC设备地址11010000*/
#define RTC_ADDRESS                 0xD0U

#define RTC_TIME_ADDR     		    0x00U	/* 秒*/
#define RTC_CTROL_ADDR              0x07U    /* 控制寄存器*/

/* 涓流充电控制寄存器*/
/* TCS[3:0]      DS[1,0]     ROUT[1,0]*/
/* TCS为充电开关，只有在1010状态才开启充电*/
/* DS指示VCC和VBACKUP间是否有二极管 10表示有二极管 01表示没有二级管*/
/* ROUT表示所接的电阻是250欧姆(01)、2K欧姆(10)、4K欧姆(11)*/
#define RTC_CHARGER_ADDR            0x08U   /* 充放电控制寄存器*/
#define RTC_FLAG_ADDR               0x09U    /* 标志寄存器*/


#define RTC_READ_TRY_TIMES          5


int32_t      rtc_opened = false;     /*RTC打开标识*/
time_t   rtc_refTcount = 0U;          /* 参考时间滴答数(秒数)*/


static int32_t rtc_checktimer(uint8_t ctime[], uint8_t bcdFlag);
void I2C_delay(void);
void SendCLK(void);
void I2C_Stop(void);
void I2C_NoAck(void);
bool I2C_Start(void);
bool rtc_timeRead(uint8_t time[]);
void I2C_SendByte(u8 SendByte);
bool I2C_WaitAck(void);
void I2C_Ack(void);
bool I2C_SendByteWithAck(u8 SendByte);
uint8_t I2C_ReceiveByte(void);
bool I2C_WriteByte(uint8_t dev, uint16_t addr, const uint8_t *byte);
bool I2C_ReadByte(uint8_t dev, uint8_t *byte);
bool is_valid_rtc_cmd(uint32_t x);



/* 延时函数，调节i改变波特率，目前i=550在总线频率为168MHZ时为20KHZ，i=137时为80KHZ，i=221时为50KHZ*/
void I2C_delay(void)
{	
    u16 i=137U;
    while(i!= 0U)
    {
        i--;
    }
}


/********************************************************************************
 * Function       : SendCLK
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机在I2C总线上发送五个脉冲，以便重新获得总线控制权 ,堆栈使用
 *                  了8 bytes 
 * Calls          : I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None 
 *********************************************************************************/
void SendCLK(void)
{
    /*主机释放总线*/
    SDA_H;

    for (uint8_t i = 1U; i < 11U; ++i)
    {
        if ((i % 2U)!= 0U)
        {
            SCL_L;
        }
        else
        {
            SCL_H;
        }

        I2C_delay();
    }
}

/********************************************************************************
 * Function       : I2C_Start
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 启动I2C操作，此时SCL=1,SDA形成一个下降沿,堆栈使用了4 bytes
 * Calls          : I2C_delay，SendCLK
 * Input          : None
 * Output         : None
 * Return         : TRUE：I2C启动成功； FALSE：I2C启动失败
 *********************************************************************************/
bool I2C_Start(void)
{
	bool ret =true;
    SDA_H;

    SCL_H;
    I2C_delay();

    if (!SDA_read )
    {
        SendCLK();
        ret = false; /* SDA线为低电平则总线忙,退出*/
    }
    else
    {
		SDA_L;
		I2C_delay();

		if (SDA_read)
		{
			SendCLK();
			ret = false; /* SDA线为高电平则总线出错,退出*/
		}
		else
		{
			SDA_L;
			I2C_delay();
		}
	}

    return ret;
}


/********************************************************************************
 * Function       : I2C_Stop
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 停止I2C操作，此时SCL=1,SDA形成一个上升沿，堆栈使用了8 bytes
 * Calls          : I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void I2C_Stop(void)
{
    SCL_H;

    SDA_L;
    I2C_delay();

    SDA_H;
    I2C_delay();

    SCL_L;
    I2C_delay();
}


/********************************************************************************
 * Function       : I2C_Ack
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机应答从机,此时SDA=0，SCL形成一个正脉冲，堆栈使用了4 bytes
 * Calls          : I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void I2C_Ack(void)
{
    SDA_L;

    SCL_L;
    I2C_delay();

    SCL_H;
    I2C_delay();

    SCL_L;
    I2C_delay();
}


/********************************************************************************
 * Function       : I2C_NoAck
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机不应答从机,此时SDA=1，SCL形成一个正脉冲，堆栈使用了4 bytes
 * Calls          : I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void I2C_NoAck(void)
{
    SDA_H;

    SCL_L;
    I2C_delay();

    SCL_H;
    I2C_delay();

    SCL_L;
    I2C_delay();
}


/********************************************************************************
 * Function       : I2C_WaitAck
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机非阻塞式监听从机发来的应答信号,时序:SCL=1,SDA=0，堆栈使用
 *                  了4 bytes
 * Calls          : I2C_delay
 * Input          : None
 * Output         : None
 * Return         : TRUE：成功监听到，FALSE：未能监听到
 *********************************************************************************/
bool I2C_WaitAck(void)
{
	bool ret = true;

    /*主机释放总线*/
    SDA_H;

    SCL_L;
    I2C_delay();

    SCL_H;
    I2C_delay();

    if (SDA_read)
    {
        SCL_L;
        ret = false;
    }
    else
    {
		SCL_L;
		I2C_delay();
    }
    return ret;
}


/********************************************************************************
 * Function       : I2C_SendByte
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机向从机发送一个字节,数据从高位到低位；时序:SCL上升沿时将SDA
 *                  数据发送给从机，堆栈使用了16 bytes
 * Calls          : I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void I2C_SendByte(u8 SendByte)
{
    uint8_t i = 8U;

    SCL_L;
    I2C_delay();

    while (i!= 0U)
    {
    	i--;
        if ((SendByte &0x80U)!= 0U)
        {
            SDA_H;
        }
        else
        {
            SDA_L;
        }

        I2C_delay();

        SendByte <<= 1;

        SCL_H;
        I2C_delay();

        SCL_L;
        I2C_delay();
    }
}


/*********************************************************************************
 * Function       : I2C_SendByteWithAck
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机向从机发送一个字节,数据从高位到低位,但此时主机需要从机应答
 *                  堆栈使用了4 bytes
 * Calls          : I2C_WaitAck，I2C_SendByte，I2C_Stop
 * Input          : None
 * Output         : None
 * Return         : TRUE:主机发送数据后收到从机应答；FALSE:主机发送数据后未收到应答
 **********************************************************************************/
bool I2C_SendByteWithAck(u8 SendByte)
{
	bool ret;
    I2C_SendByte(SendByte);

    ret = I2C_WaitAck();
    if (ret == false)
    {
        I2C_Stop();
        ret = false;
    }
    else
    {
        ret = true;
    }
    return ret;
}


/*********************************************************************************
 * Function       : I2C_ReceiveByte
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机读取从机发来的一个字节,数据从高位到低位.时序:SCL=1,SDA读出
 *                  堆栈使用了12 bytes
 * Calls          : I2C_delay
 * Input          : None
 * Output         : None
 * Return         : ReceiveByte:主机读取的字节数据
 **********************************************************************************/
uint8_t I2C_ReceiveByte(void)
{
    uint8_t i = 8U;
    uint8_t ReceiveByte = 0U;

    /*主机释放总线*/
    SDA_H;
    SCL_L;
    I2C_delay();

    while (i!= 0U)
    {
    	i--;
        ReceiveByte <<= 1;

        SCL_H;
        I2C_delay();

        if (SDA_read)
        {
            ReceiveByte |= 0x01U;
        }

        SCL_L;
        I2C_delay();
    }

    return ReceiveByte;
}


/**********************************************************************************
 * Function       : I2C_WriteByte
 * Author         : Linfei
 * Date           : 2012.07.11
 * Description    : 主机向从机写入一个字节的数据
 * Calls          : I2C_Start，I2C_SendByteWithAck，I2C_Stop
 * Input          : dev:设备地址；
 *                  addr:待写入数据的地址，低8位有效
 *　　　　　　　　　byte:待写入字节的地址
 * Output         : None
 * Return         : TRUE：写入成功；FALSE：I2C未开启、设备未应答、地址错误，写入失败
 ***********************************************************************************/
bool I2C_WriteByte(uint8_t dev, uint16_t addr, const uint8_t *byte)
{
	bool ret = false;
	bool tmp;
    /*开启I2C操作*/
	tmp = I2C_Start();
    if (tmp != 0)
    {
		/*发送设备号*/
		tmp = I2C_SendByteWithAck(dev & 0xFEU);
		if (tmp != 0)
		{
			/* 发送低8位地址*/
			tmp = I2C_SendByteWithAck((uint8_t)(addr & 0xFFU));
			if (tmp != 0)
			{
				
                if(byte != NULL)
				{
					/* 发送内容*/
					tmp = I2C_SendByteWithAck(*byte);
					if (tmp != 0)
					{
						I2C_Stop();				/* 结束操作*/
                        ret = true;
					}
				}
                else
                {
                    I2C_Stop();				/* 结束操作*/
                    ret = true;
                }
			}
		}
    }
    return ret;
}


/**********************************************************************************
 * Function       : I2C_ReadByte
 * Author         : Linfei
 * Date           : 2012.07.11
 * Description    : 主机从从机读取一个字节的数据，数据地址由I2C_WriteByte操作提供
 * Calls          : I2C_Start，I2C_SendByteWithAck，I2C_Stop，I2C_ReceiveByte
 * Input          : dev:设备地址；
 *　　　　　　　　　byte:存放读出字节的地址
 * Output         : None
 * Return         : TRUE：读取成功；FALSE：I2C未开启、设备未应答、地址错误，读取失败
 ***********************************************************************************/
bool I2C_ReadByte(uint8_t dev, uint8_t *byte)
{
	bool ret = false;
	bool tmp;
    /* 开启I2C操作*/
	tmp = I2C_Start();
    if (tmp != 0)
    {
		/* 发送设备号和读操作码*/
    	tmp = I2C_SendByteWithAck(dev | 0x01U);
		if (tmp != 0)
		{
			*byte = I2C_ReceiveByte();
			I2C_NoAck();

			/*结束读操作*/
			I2C_Stop();
			ret = true;
		}
    }
    return ret;
}


/**********************************************************************************
 * Function       : rtc_open
 * Author         : Linfei
 * Date           : 2012.07.13
 * Description    : 打开RTC时钟，初始化开启内部晶振（防止某些情况下晶振停止，详见手册）
 * Calls          : I2C_WriteByte，I2C_ReadByte
 * Input          : flag:未使用
 *　　　　　　　　mode:未使用
 * Output         : None
 * Return         : OPFAULT：开启失败；
 *                  DSUCCESS：开启成功
 ***********************************************************************************/
int32_t rtc_open(int32_t flag, int32_t mode)
{
	int32_t ret = OPFAULT ;
	uint8_t tmp;
	uint8_t tmp1;
    if(rtc_opened != true)
    {
		/* 用户设定的参考时间合法性检查*/
		uint8_t time[6] = { REF_YEAR, REF_MONTH, REF_DATE, 0x00U, 0x00U, 0x00U};
		if(rtc_checktimer(time, 1U) == true)
		{
			/* 清除晶振停止标志*/
			uint8_t cmd = 0U;
			if(I2C_WriteByte(RTC_ADDRESS, RTC_FLAG_ADDR, &cmd) == true)
			{
				struct tm tmpTime;
                tmp1 = ((time[0] >> 4) & 0x0FU)*10U;  /*for MISRA-2004*/
                tmp = tmp1 + (time[0] & 0x0FU) + 100U;

                tmpTime.tm_year = (int32_t)tmp;

                tmp1 = ((time[1] >> 4) & 0x0FU)*10U;
                tmp = (tmp1 + (time[1] & 0x0FU)) - 1U;

                tmpTime.tm_mon  = (int32_t)tmp;

                tmp1 = ((time[2] >> 4) & 0x0FU)*10U;
                tmp = tmp1 + (time[2] & 0x0FU);

                tmpTime.tm_mday = (int32_t)tmp;

                tmp1 = ((time[3] >> 4) & 0x0FU)*10U;
                tmp = tmp1 + (time[3] & 0x0FU);

                tmpTime.tm_hour = (int32_t)tmp;

                tmp1 = ((time[4] >> 4) & 0x0FU)*10U;
                tmp = tmp1 + (time[4] & 0x0FU);

                tmpTime.tm_min  = (int32_t)tmp;

                tmp1 = ((time[5] >> 4) & 0x0FU)*10U;
                tmp = tmp1 + (time[5] & 0x0FU);

                tmpTime.tm_sec  = (int32_t)tmp;
                tmpTime.tm_isdst= 0;

				rtc_refTcount = mktime(&tmpTime);

				if(rtc_refTcount != 0xFFFFFFFFU)
				{
					rtc_opened = true;
					ret = DSUCCESS;
				}
			}
		}
    }
    return ret;
}

/* 读取RTC时间*/
bool rtc_timeRead(uint8_t time[])
{
	bool ret = false;
    /* 设置寄存器指针为0*/
    if(I2C_WriteByte(RTC_ADDRESS, 0U, NULL) == true)
    {
		/* 读取时间*/
		uint8_t i;
        for(i = 0U; i < 7U; i++)
		{
			if(I2C_ReadByte(RTC_ADDRESS, &time[i]) != true)
			{
				break;
			}
		}
		if(i == 7)
		{
			time[0] &= 0x7FU;
			time[1] &= 0x7FU;
			time[2] &= 0x3FU;
			time[4] &= 0x3FU;
			time[5] &= 0x1FU;
            ret = true;
		}
	}
	return ret;
}



/**********************************************************************************
 * Function       : rtc_read
 * Author         : Linfei
 * Date           : 2012.07.13
 * Description    : 读取RTC时间，顺序为：年 月 日 时 分 秒
 * Calls          : I2C_WriteByte，I2C_ReadByte
 * Input          : buf:保存待读取出的时间
 *　　　　　　　  : count:待读取出的时间数组长度，固定为6
 * Output         : None
 * Return         : 0：读取失败
 *                  6：读取成功 
 ***********************************************************************************/
int32_t rtc_read(uint8_t buf[], uint32_t count)
{
	int32_t ret = 0;
	uint8_t tmp1;
	uint8_t tmp2;
    if((rtc_opened == true) && (buf != NULL) && (count >= 6U))
    {
		uint8_t time[7] = { 0 };
		uint8_t rTry = (uint8_t)RTC_READ_TRY_TIMES;
		while(rTry!= 0U)
		{
			rTry--;
			/* 读取时间*/
			if(rtc_timeRead(time) != true)
			{
				ret = 0;
				break;
			}

			/* 合法性判断，与参考时间对比*/
			struct tm readTime;
			tmp1 = ((time[6] >> 4) & 0x0FU)*10U;
			tmp2 = tmp1 + (time[6] & 0x0FU) + 100U;

			readTime.tm_year =  (int32_t)tmp2;

			tmp1 = ((time[5] >> 4) & 0x0FU)*10U;
			tmp2 = (tmp1 + (time[5] & 0x0FU)) - 1U;

			readTime.tm_mon  = (int32_t)tmp2;

			tmp1 = ((time[4] >> 4) & 0x0FU)*10U;
			tmp2 = tmp1 + (time[4] & 0x0FU);

			readTime.tm_mday = (int32_t)tmp2;

			tmp1 = ((time[2] >> 4) & 0x0FU)*10U;
			tmp2 = tmp1 + (time[2] & 0x0FU);

			readTime.tm_hour = (int32_t)tmp2;

			tmp1 = ((time[1] >> 4) & 0x0FU)*10U;
			tmp2 = tmp1 + (time[1] & 0x0FU);

			readTime.tm_min  = (int32_t)tmp2;

			tmp1 = ((time[0] >> 4) & 0x0FU)*10U;
			tmp2 = tmp1 + (time[0] & 0x0FU);

			readTime.tm_sec  = (int32_t)tmp2;
			readTime.tm_isdst= 0;

			time_t checkTcount = mktime(&readTime);
			if(checkTcount > rtc_refTcount)
			{
				/* 按年月日时分秒排序*/
				buf[0] = time[6];
				buf[1] = time[5];
				buf[2] = time[4];
				buf[3] = time[2];
				buf[4] = time[1];
				buf[5] = time[0];
				ret = 6;
			}
		}
    }
    return ret;
}

/* 时间合法性判断，顺序：年月日时分秒，支持BCD码和二进制*/
static int32_t rtc_checktimer(uint8_t ctime[], uint8_t bcdFlag)
{
	int32_t ret = true; /*for MISRA-2004*/
	uint8_t tmp;		/*for MISRA-2004*/
    uint8_t tmpTime[6];
    uint8_t *time = ctime;
    if(bcdFlag == 1U)
    {
    	tmp = ((ctime[0]>>4) & 0x0FU)*10U;      /*for MISRA-2004*/
        tmpTime[0] = tmp + (ctime[0] & 0x0FU);

        tmp = ((ctime[1]>>4) & 0x0FU)*10U;
        tmpTime[1] = tmp + (ctime[1] & 0x0FU);

        tmp = ((ctime[2]>>4) & 0x0FU)*10U;
        tmpTime[2] = tmp + (ctime[2] & 0x0FU);

        tmp = ((ctime[3]>>4) & 0x0FU)*10U;
        tmpTime[3] = tmp + (ctime[3] & 0x0FU);

        tmp = ((ctime[4]>>4) & 0x0FU)*10U;
        tmpTime[4] = tmp + (ctime[4] & 0x0FU);

        tmp = ((ctime[5]>>4) & 0x0FU)*10U;
        tmpTime[5] = tmp + (ctime[5] & 0x0FU);
        time = tmpTime;
    }
    if((time[0] > 99U) || (time[1] > 12U) || \
       (time[1] == 0U) || (time[2] == 0U) || \
       (time[3] > 23U) || (time[4] > 59U) || \
       (time[5] > 59U))
    {
        ret = false;
    }
    /*月大31天，2月正常28天，遇润年29天，其余月小30天*/
    else if ((time[1] == 1U) || (time[1] == 3U) || \
             (time[1] == 5U) || (time[1] == 7U) || \
             (time[1] == 8U) || (time[1] == 10U)|| \
             (time[1] == 12U))
    {
        if (time[2] > 31U)
        {
            ret = false;
        }
    }

    else if ((time[1] == 4U) || (time[1] == 6U) || \
             (time[1] == 9U) || (time[1] == 11U))
    {
        if (time[2] > 30U)
        {
            ret = false;
        }
    }

    else if (time[1] == 2U)
    {
        if (((time[0] % 4U) == 0U) && (time[2] > 29U))
        {
            ret = false;
        }
        else if (((time[0] % 4U) != 0U) && (time[2] > 28U))
        {
            ret = false;
        }
        else
        {
        	/*do nothing*/
        }
    }
    else
    {
    	/*do nothing*/
    }
    return ret;
}

/**********************************************************************************
 * Function       : rtc_write
 * Author         : Linfei
 * Date           : 2012.07.11
 * Description    : 设定RTC时间，顺序为：年 月 日 时 分 秒
 * Calls          : I2C_WriteByte
 * Input          : buf:保存待设置的时间
 *　　　　　　　        : count:待设置时间数组长度，固定为6
 * Output         : None
 * Return         : 0：设置失败
 *                  6：设置成功
 ***********************************************************************************/
int32_t rtc_write(const uint8_t buf[], uint32_t count)
{
	int32_t ret = 0;     /*for MISRA-2004*/
    uint16_t i = 0U;
    if((rtc_opened != false) && (buf != NULL) && (count >= 6U))
    {
		if(rtc_checktimer((uint8_t *)buf, 1U) == true)
		{
			struct tm readTime;							 //wujing 2012.11.12 the time parameter which user set must larger than referance time .
	        readTime.tm_year = ((((uint32_t)buf[0] >> 4) & 0x0FU) * 10U) + (((uint32_t)buf[0] & 0x0FU) + 100U);
	        readTime.tm_mon  = ((((uint32_t)buf[1] >> 4) & 0x0FU) * 10U) + (((uint32_t)buf[1] & 0x0FU) - 1U);
	        readTime.tm_mday = ((((uint32_t)buf[2] >> 4) & 0x0FU) * 10U) + ((uint32_t)buf[2] & 0x0FU);
	        readTime.tm_hour = ((((uint32_t)buf[3] >> 4) & 0x0FU) * 10U) + ((uint32_t)buf[3] & 0x0FU);
	        readTime.tm_min  = ((((uint32_t)buf[4] >> 4) & 0x0FU) * 10U) + ((uint32_t)buf[4] & 0x0FU);
	        readTime.tm_sec  = ((((uint32_t)buf[5] >> 4) & 0x0FU) * 10U) + ((uint32_t)buf[5] & 0x0FU);
	        readTime.tm_isdst = 0;
	        time_t checkTcount = mktime(&readTime);
	        if(checkTcount > rtc_refTcount)	  /* 合法性判断，与参考时间对比 */
	        {
				uint8_t time[7];
				time[0] = buf[5];
				time[1] = buf[4];
				time[2] = buf[3];
				time[3] = 1U;
				time[4] = buf[2];
				time[5] = buf[1];
				time[6] = buf[0];

				for(i = 0U; i < 7U; i++)
				{
					if(I2C_WriteByte(RTC_ADDRESS, i, (uint8_t *)&time[i]) != true)
					{
						break;
					}
				}
				
				if(i == 7)
				{
					ret = 6;
				}
			}
		}
    }
  return ret;
}


/**********************************************************************************
 * Function       : rtc_ioctl
 * Author         : Linfei
 * Date           : 2012.07.11
 * Description    : RTC控制，设置开启或关闭涓流充电
 * Calls          : I2C_WriteByte
 * Input          : op:操作码，目前支持的是RTC_CHARGER_ON，RTC_CHARGER_OFF
 *　　　　　　　  : arg:用户参数，暂时未用
 * Output         : None
 * Return         : OPFAULT：设置失败
 *                  DSUCCESS：设置成功
 ***********************************************************************************/
int32_t rtc_ioctl(uint32_t op, void *arg)
{
	arg = arg;
	int32_t ret = OPFAULT;
    if(rtc_opened == true)
    {
		uint8_t cmd = (uint8_t)op;
		switch(cmd)
		{
			case RTC_CHARGER_OFF:
			case RTC_CHARGER_ON_250:
			case RTC_CHARGER_ON_4K_D:
			case RTC_CHARGER_ON_2K:
			case RTC_CHARGER_ON_4K:
			case RTC_CHARGER_ON_250_D:
			case RTC_CHARGER_ON_2K_D:
				if(I2C_WriteByte(RTC_ADDRESS, RTC_CHARGER_ADDR, &cmd) == true)
				{
				   ret = DSUCCESS;
				}
				break;
			default:
				break;
		}
    }
    return ret;
}

/**********************************************************************************
 * Function       : rtc_release
 * Author         : Linfei
 * Date           : 2012.07.12
 * Description    : RTC程序关闭
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS：设置成功
 ***********************************************************************************/

int32_t rtc_release(void)
{
    if(rtc_opened != true)
    {
		rtc_opened = false;
    }
    return DSUCCESS;
}

