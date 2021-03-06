/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
 * File name      :rtc.c
 * Author         :Linfei
 * Date           :2012.07.13
 * Description    :DS1340C驱动，模拟I2C方式，在总线频率为168MHZ时为20KHZ，修改
  *                r8025_I2C_delay函数中的i变量值可调节波特率，具体用示波器观察
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2012.07.11 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 ********************************************************************************/
#include "r8025.h"
#include <time.h>

GPIO_CTL  R8025_SCL;
GPIO_CTL  R8025_SDA;

/* 总线置高置低宏 */
#define R8025_SCL_H        (GPIO_SetBits(R8025_SCL.port, R8025_SCL.pin))
#define R8025_SCL_L        (GPIO_ResetBits(R8025_SCL.port, R8025_SCL.pin))  
#define R8025_SDA_H        (GPIO_SetBits(R8025_SDA.port, R8025_SDA.pin))
#define R8025_SDA_L        (GPIO_ResetBits(R8025_SDA.port, R8025_SDA.pin))

/* 总线值读取宏 */
#define R8025_SCL_read     (GPIO_ReadInputDataBit(R8025_SCL.port, R8025_SCL.pin) == Bit_SET)
#define R8025_SDA_read     (GPIO_ReadInputDataBit(R8025_SDA.port, R8025_SDA.pin) == Bit_SET)


/* R8025设备地址01100100 */
#define R8025_ADDRESS                 0x64U

/* R8025寄存器地址 */
#define R8025_SECOND_ADDR              0x00U  /* 秒 */
#define R8025_MINUTE_ADDR              0x01U  /* 分 */
#define R8025_HOUR_ADDR                0x02U  /* 时 */
#define R8025_DAY_ADDR                 0x03U  /* 星期 */
#define R8025_DATE_ADDR                0x04U  /* 日 */
#define R8025_MONTH_ADDR               0x05U  /* 月 */
#define R8025_YEAR_ADDR                0x06U  /* 年 */
#define R8025_DIGITAL_OFFSET_ADDR      0x07U  /* 晶振校准 */
#define R8025_CTROL1_ADDR              0x0EU  /* 控制寄存器1 */
#define R8025_CTROL2_ADDR              0x0FU  /* 控制寄存器2 */


/* 控制字 */
#define R8025_SET_24_TYPE              0x20U  /* 24小时制 */
#define R8025_SET_12_TYPE              0x00U  /* 12小时制 */

#define R8025_SET_PLEVEL21             0x00U  /* 2.1V掉电检测阀值 */
#define R8025_SET_PLEVEL13             0xA0U  /* 1.3V掉电检测阀值 *///wujing 2012.11.30 add /XST setting


/* 读取时间的重试次数 */
#define R8025_READ_TRY_TIMES          5U


int32_t  r8025_opened = false;          /* RTC打开标识 */
time_t   r8025_refTcount = 0U;          /* 参考时间滴答数(秒数) */


bool r8025_checktimer(uint8_t cktime[], uint8_t bcdFlag);
bool r8025_timeRead(uint8_t rtime[]);
void r8025_I2C_delay(void);
void r8025_SendCLK(void);
bool r8025_I2C_Start(void);
void r8025_I2C_Stop(void);
void r8025_I2C_Ack(void);
void r8025_I2C_NoAck(void);
bool r8025_I2C_WaitAck(void);
void r8025_I2C_SendByte(uint8_t SendByte);
bool r8025_I2C_SendByteWithAck(uint8_t SendByte);
uint8_t r8025_I2C_ReceiveByte(void);
bool r8025_I2C_WriteByte(uint8_t dev, uint8_t addr, uint8_t byte);
bool r8025_I2C_ReadByte(uint8_t dev, uint8_t addr, uint8_t *byte);
void r8025_i2c_init(void);



/* 延时函数，调节i改变波特率，目前i=550在总线频率为168MHZ时为20KHZ，i=137时为80KHZ，i=221时为50KHZ */
void r8025_I2C_delay(void)
{
    uint16_t i = 137U;//未开预取：137：12.5us;开预取：480:12us
    while(i != 0U)
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
 * Calls          : r8025_I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void r8025_SendCLK(void)
{
    /*主机释放总线*/
    R8025_SDA_H;

    for (uint8_t i = 1U; i < 11U; ++i)
    {
        if ((i % 2U) != 0U)
        {
            R8025_SCL_L;
        }
        else
        {
            R8025_SCL_H;
        }

        r8025_I2C_delay();
    }
}

/********************************************************************************
 * Function       : r8025_I2C_Start
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 启动I2C操作，此时SCL=1,SDA形成一个下降沿,堆栈使用了4 bytes
 * Calls          : r8025_I2C_delay，SendCLK
 * Input          : None
 * Output         : None
 * Return         : true：I2C启动成功； false：I2C启动失败
 *********************************************************************************/
bool r8025_I2C_Start(void)
{
	bool ret =true;
    R8025_SDA_H;

    R8025_SCL_H;
    r8025_I2C_delay();

    if (!R8025_SDA_read)
    {
        r8025_SendCLK();
        ret = false; /* SDA线为低电平则总线忙,退出*/
    }
    else
    {
		R8025_SDA_L;
		r8025_I2C_delay();

		if (R8025_SDA_read)
		{
			r8025_SendCLK();
			ret = false; /* SDA线为高电平则总线出错,退出*/
		}
		else
		{
			R8025_SDA_L;
			r8025_I2C_delay();
		}
	}
    return ret;
}


/********************************************************************************
 * Function       : r8025_I2C_Stop
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 停止I2C操作，此时SCL=1,SDA形成一个上升沿，堆栈使用了8 bytes
 * Calls          : r8025_I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void r8025_I2C_Stop(void)
{
    R8025_SCL_H;

    R8025_SDA_L;
    r8025_I2C_delay();

    R8025_SDA_H;
    r8025_I2C_delay();

    R8025_SCL_L;
    r8025_I2C_delay();
}


/********************************************************************************
 * Function       : r8025_I2C_Ack
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机应答从机,此时SDA=0，SCL形成一个正脉冲，堆栈使用了4 bytes
 * Calls          : r8025_I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void r8025_I2C_Ack(void)
{
    R8025_SDA_L;

    R8025_SCL_L;
    r8025_I2C_delay();

    R8025_SCL_H;
    r8025_I2C_delay();

    R8025_SCL_L;
    r8025_I2C_delay();
}


/********************************************************************************
 * Function       : r8025_I2C_NoAck
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机不应答从机,此时SDA=1，SCL形成一个正脉冲，堆栈使用了4 bytes
 * Calls          : r8025_I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void r8025_I2C_NoAck(void)
{
    R8025_SDA_H;

    R8025_SCL_L;
    r8025_I2C_delay();

    R8025_SCL_H;
    r8025_I2C_delay();

    R8025_SCL_L;
    r8025_I2C_delay();
}


/********************************************************************************
 * Function       : r8025_I2C_WaitAck
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机非阻塞式监听从机发来的应答信号,时序:SCL=1,SDA=0，堆栈使用
 *                  了4 bytes
 * Calls          : r8025_I2C_delay
 * Input          : None
 * Output         : None
 * Return         : true：成功监听到，false：未能监听到
 *********************************************************************************/
bool r8025_I2C_WaitAck(void)
{
    bool ret = false;
    /*主机释放总线 */
    R8025_SDA_H;

    R8025_SCL_L;
    r8025_I2C_delay();

    R8025_SCL_H;
    r8025_I2C_delay();

    if(R8025_SDA_read)
    {
        R8025_SCL_L;
    }
    else
    {
        R8025_SCL_L;
        r8025_I2C_delay();
        ret = true;
    }
    return ret;
}


/********************************************************************************
 * Function       : r8025_I2C_SendByte
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机向从机发送一个字节,数据从高位到低位；时序:SCL上升沿时将SDA
 *                  数据发送给从机，堆栈使用了16 bytes
 * Calls          : r8025_I2C_delay
 * Input          : None
 * Output         : None
 * Return         : None
 *********************************************************************************/
void r8025_I2C_SendByte(uint8_t SendByte)
{
    uint8_t i = 8U;

    R8025_SCL_L;
    r8025_I2C_delay();

    while(i!= 0U)
    {
        i--;
        if((SendByte & 0x80U) != 0U)
        {
            R8025_SDA_H;
        }
        else
        {
            R8025_SDA_L;
        }

        r8025_I2C_delay();

        SendByte <<= 1;

        R8025_SCL_H;
        r8025_I2C_delay();

        R8025_SCL_L;
        r8025_I2C_delay();
    }
}


/*********************************************************************************
 * Function       : r8025_I2C_SendByteWithAck
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机向从机发送一个字节,数据从高位到低位,但此时主机需要从机应答
 *                  堆栈使用了4 bytes
 * Calls          : r8025_I2C_WaitAck，r8025_I2C_SendByte，r8025_I2C_Stop
 * Input          : None
 * Output         : None
 * Return         : true:主机发送数据后收到从机应答；false:主机发送数据后未收到应答
 **********************************************************************************/
bool r8025_I2C_SendByteWithAck(uint8_t SendByte)
{
    bool ret = false;
    r8025_I2C_SendByte(SendByte);

    if (!r8025_I2C_WaitAck())
    {
        r8025_I2C_Stop();
    }
    else
    {
        ret = true;
    }
    return ret;
}


/*********************************************************************************
 * Function       : r8025_I2C_ReceiveByte
 * Author         : Xu Shun'an
 * Date           : 2011.05.31
 * Description    : 主机读取从机发来的一个字节,数据从高位到低位.时序:SCL=1,SDA读出
 *                  堆栈使用了12 bytes
 * Calls          : r8025_I2C_delay
 * Input          : None
 * Output         : None
 * Return         : ReceiveByte:主机读取的字节数据
 **********************************************************************************/
uint8_t r8025_I2C_ReceiveByte(void)
{
    uint8_t i = 8U;
    uint8_t ReceiveByte = 0U;

    /* 主机释放总线 */
    R8025_SDA_H;
    R8025_SCL_L;
    r8025_I2C_delay();

    while(i != 0U)
    {
        i--;
        ReceiveByte <<= 1;

        R8025_SCL_H;
        r8025_I2C_delay();

        if (R8025_SDA_read)
        {
            ReceiveByte |= 0x01U;
        }

        R8025_SCL_L;
        r8025_I2C_delay();
    }

    return ReceiveByte;
}


/**********************************************************************************
 * Function       : r8025_I2C_WriteByte
 * Author         : Linfei
 * Date           : 2012.07.11
 * Description    : 主机向从机写入一个字节的数据
 * Calls          : r8025_I2C_Start，r8025_I2C_SendByteWithAck，r8025_I2C_Stop
 * Input          : dev:设备地址；
 *                  addr:待写入数据的地址，低8位有效
 *　　　　　　　　　byte:待写入字节的地址
 * Output         : None
 * Return         : true：写入成功；false：I2C未开启、设备未应答、地址错误，写入失败
 ***********************************************************************************/
bool r8025_I2C_WriteByte(uint8_t dev, uint8_t addr, uint8_t byte)
{
    bool ret = false;
    addr <<= 4;
    /*开启I2C操作 */
    if (r8025_I2C_Start() == true)
    {
        /*发送设备号 */
        if (r8025_I2C_SendByteWithAck(dev & 0xFEU) == true)
        {
            /* 发送低8位地址 */
            if (r8025_I2C_SendByteWithAck(addr) == true)
            {
                /* 发送内容 */
                if (r8025_I2C_SendByteWithAck(byte) == true)
                {
                    /* 结束操作 */
                    r8025_I2C_Stop();
                    ret = true;
                }
            }
        }
    }
    return ret;
}


/**********************************************************************************
 * Function       : r8025_I2C_ReadByte
 * Author         : Linfei
 * Date           : 2012.07.11
 * Description    : 主机从从机读取一个字节的数据，数据地址由r8025_I2C_WriteByte操作提供
 * Calls          : r8025_I2C_Start，r8025_I2C_SendByteWithAck，r8025_I2C_Stop，r8025_I2C_ReceiveByte
 * Input          : dev:设备地址；
 *　　　　　　　　　byte:存放读出字节的地址
 * Output         : None
 * Return         : true：读取成功；false：I2C未开启、设备未应答、地址错误，读取失败
 ***********************************************************************************/
bool r8025_I2C_ReadByte(uint8_t dev, uint8_t addr, uint8_t *byte)
{
    bool ret = false;
    addr <<= 4;
    /* 开启I2C操作 */
    if(r8025_I2C_Start() == true)
    {
        /* 发送设备号 */
        if(r8025_I2C_SendByteWithAck(dev & 0xFEU) == true)
        {
            /* 发送低8位地址 */
            if(r8025_I2C_SendByteWithAck(addr | 0x04U) == true)
            {
                *byte = r8025_I2C_ReceiveByte();
                r8025_I2C_NoAck();

                /*结束读操作 */
                r8025_I2C_Stop();
                ret = true;
            }
        }
    }
    return ret;
}

//EMBF1.1.1更新IO口初始化
void r8025_i2c_init(void)
{
    device_gpio_config(DEVICE_ID_RTC, TRUE);
    R8025_SCL = get_gpio_ctl_attr(DEVICE_ID_RTC, PIN_GPIO_SCK);
    R8025_SDA = get_gpio_ctl_attr(DEVICE_ID_RTC, PIN_GPIO_SDA);
}


/**********************************************************************************
 * Function       : r8025_open
 * Author         : Linfei
 * Date           : 2012.07.13
 * Description    : 打开RTC时钟，初始化开启内部晶振（防止某些情况下晶振停止，详见手册）
 * Calls          : r8025_I2C_WriteByte，r8025_I2C_ReadByte
 * Input          : flag:未使用
 *　　　　　　　　mode:未使用
 * Output         : None
 * Return         : EFAULT：开启失败；
 *                  DSUCCESS：开启成功
 ***********************************************************************************/
int32_t r8025_open(int32_t flag, int32_t mode)
{
    int32_t ret = OPFAULT;
    if(r8025_opened != true)
    {
        r8025_i2c_init(); 

        /* 用户设定的参考时间合法性检查 */
        uint8_t stime[6] = { R8025_REF_YEAR, R8025_REF_MONTH, R8025_REF_DATE, 0x00U, 0x00U, 0x00U};
        if(r8025_checktimer(stime, 1U) == true)
        {
            /* 设置掉电检测阀值为1.3V */
            if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_CTROL2_ADDR, R8025_SET_PLEVEL13) == true)
            {
                /* 参考时间换算成绝对时间 */
                struct tm tmpTime;
                tmpTime.tm_year = ((((uint32_t)stime[0] >> 4) & 0x0FU) * 10U) + (((uint32_t)stime[0] & 0x0FU) + 100U);
                tmpTime.tm_mon  = ((((uint32_t)stime[1] >> 4) & 0x0FU) * 10U) + (((uint32_t)stime[1] & 0x0FU) - 1U);
                tmpTime.tm_mday = ((((uint32_t)stime[2] >> 4) & 0x0FU) * 10U) + ((uint32_t)stime[2] & 0x0FU);
                tmpTime.tm_hour = ((((uint32_t)stime[3] >> 4) & 0x0FU) * 10U) + ((uint32_t)stime[3] & 0x0FU);
                tmpTime.tm_min  = ((((uint32_t)stime[4] >> 4) & 0x0FU) * 10U) + ((uint32_t)stime[4] & 0x0FU);
                tmpTime.tm_sec  = ((((uint32_t)stime[5] >> 4) & 0x0FU) * 10U) + ((uint32_t)stime[5] & 0x0FU);
                tmpTime.tm_isdst = 0;
                r8025_refTcount = mktime(&tmpTime);

                if(r8025_refTcount != 0xFFFFFFFFU)
                {
                    r8025_opened = true;
                    ret = DSUCCESS;
                }
            }
        }
    }
    return ret;
}

/* 读取RTC时间 */
bool r8025_timeRead(uint8_t rtime[])
{
    bool ret = false;
    uint8_t i;
    for(i = 0U; i < 7U; i++)
    {
        if(r8025_I2C_ReadByte(R8025_ADDRESS, R8025_SECOND_ADDR + i, &rtime[i]) != true)
        {
            break;
        }
    }
    if(i == 7U)
    {
        rtime[0] &= 0x7FU;
        rtime[1] &= 0x7FU;
        rtime[2] &= 0x3FU;
        rtime[4] &= 0x3FU;
        rtime[5] &= 0x1FU;
        ret = true;
    }
    return ret;
}



/**********************************************************************************
 * Function       : r8025_read
 * Author         : Linfei
 * Date           : 2012.07.13
 * Description    : 读取RTC时间，顺序为：年 月 日 时 分 秒 BCD码
 * Calls          : r8025_I2C_WriteByte，r8025_I2C_ReadByte
 * Input          : buf:保存待读取出的时间
 *　　　　　　　  : count:待读取出的时间数组长度，固定为6
 * Output         : None
 * Return         : 0：读取失败
 *                  6：读取成功
 ***********************************************************************************/
int32_t r8025_read(uint8_t buf[], uint32_t count)
{
    int32_t ret = 0;
    uint8_t tmptime[7];
    if((r8025_opened == true) && (buf != NULL) && (count >= 6U))
    {
        uint8_t rtime[7] = { 0U };
        uint8_t mode = 0U;
        uint8_t rTry = R8025_READ_TRY_TIMES;

        while(rTry > 0U)
        {
        	rTry--;
            if(r8025_I2C_ReadByte(R8025_ADDRESS, R8025_CTROL1_ADDR, &mode) == true)
            {
                /* 读取时间 */
                if(r8025_timeRead(rtime) == true)
                {
                    /* 若是12小时制，则转换成24小时制 */
                    if((mode & R8025_SET_24_TYPE) == 0U)
                    {
                        uint8_t hour = rtime[2] & ~0x20U;
                        hour = ((((uint32_t)hour >> 4) & 0x0FU) * 10U) + ((uint32_t)hour & 0x0FU);
                        if((rtime[2] & 0x20U) == 0U)  /* AM */
                        {
                            hour = hour % 12U;
                        }
                        else                       /* PM */
                        {
                            hour = (hour % 12U) + 12U;
                        }
                        rtime[2] = (((uint32_t)hour / 10U) << 4) + ((uint32_t)hour % 10U);
                    }

                    /* 时间合法性检查，格式检查 */
                    tmptime[0] = rtime[6];
                    tmptime[1] = rtime[5];
                    tmptime[2] = rtime[4];
                    tmptime[3] = rtime[2];
                    tmptime[4] = rtime[1];
                    tmptime[5] = rtime[0];
                    if(r8025_checktimer(tmptime, 1U) == true)
                    {
                        /* 合法性判断，与参考时间对比 */
                        struct tm readTime;
                        readTime.tm_year = ((((uint32_t)rtime[6] >> 4) & 0x0FU) * 10U) + (((uint32_t)rtime[6] & 0x0FU) + 100U);
                        readTime.tm_mon  = ((((uint32_t)rtime[5] >> 4) & 0x0FU) * 10U) + (((uint32_t)rtime[5] & 0x0FU) - 1U);
                        readTime.tm_mday = ((((uint32_t)rtime[4] >> 4) & 0x0FU) * 10U) + ((uint32_t)rtime[4] & 0x0FU);
                        readTime.tm_hour = ((((uint32_t)rtime[2] >> 4) & 0x0FU) * 10U) + ((uint32_t)rtime[2] & 0x0FU);
                        readTime.tm_min  = ((((uint32_t)rtime[1] >> 4) & 0x0FU) * 10U) + ((uint32_t)rtime[1] & 0x0FU);
                        readTime.tm_sec  = ((((uint32_t)rtime[0] >> 4) & 0x0FU) * 10U) + ((uint32_t)rtime[0] & 0x0FU);
                        readTime.tm_isdst = 0;
                        time_t checkTcount = mktime(&readTime);
                        if(checkTcount > r8025_refTcount)
                        {
                            /* 按年月日时分秒排序 */
                            buf[0] = rtime[6];
                            buf[1] = rtime[5];
                            buf[2] = rtime[4];
                            buf[3] = rtime[2];
                            buf[4] = rtime[1];
                            buf[5] = rtime[0];
                            ret = 6;
                            break;
                        }
                    }
                }
            }
        }
    }
    return ret;
}



/* 时间合法性判断，顺序：年月日时分秒，支持BCD码和二进制 */
bool r8025_checktimer(uint8_t cktime[], uint8_t bcdFlag)
{
    uint8_t tmpTime[6];
    uint8_t *stime = cktime;
    bool ret = false;
    if(bcdFlag == 1U)
    {
        tmpTime[0] = ((((uint32_t)cktime[0] >> 4) & 0x0FU) * 10U) + ((uint32_t)cktime[0] & 0x0FU);
        tmpTime[1] = ((((uint32_t)cktime[1] >> 4) & 0x0FU) * 10U) + ((uint32_t)cktime[1] & 0x0FU);
        tmpTime[2] = ((((uint32_t)cktime[2] >> 4) & 0x0FU) * 10U) + ((uint32_t)cktime[2] & 0x0FU);
        tmpTime[3] = ((((uint32_t)cktime[3] >> 4) & 0x0FU) * 10U) + ((uint32_t)cktime[3] & 0x0FU);
        tmpTime[4] = ((((uint32_t)cktime[4] >> 4) & 0x0FU) * 10U) + ((uint32_t)cktime[4] & 0x0FU);
        tmpTime[5] = ((((uint32_t)cktime[5] >> 4) & 0x0FU) * 10U) + ((uint32_t)cktime[5] & 0x0FU);
        stime = tmpTime;
    }
    
    if((stime[0] <= 99U) && (stime[1] <= 12U) && (stime[1] > 0U) && (stime[2] > 0U))
    {
        if((stime[3] <= 23U) &&     /* h 0~23 */
           (stime[4] <= 59U) &&     /* m 0~59 */
           (stime[5] <= 59U))       /* s 0~59 */                
        {
            /*月大31天，2月正常28天，遇润年29天，其余月小30天 */
            if((stime[1] == 1U) || (stime[1] == 3U) || \
               (stime[1] == 5U) || (stime[1] == 7U) || \
               (stime[1] == 8U) || (stime[1] == 10U) || \
               (stime[1] == 12U))
            {
                if (stime[2] <= 31U)
                {
                    ret = true;
                }
            }
            else if((stime[1] == 4U) || (stime[1] == 6U) || \
                    (stime[1] == 9U) || (stime[1] == 11U))
            {
                if (stime[2] <= 30U)
                {
                    ret = true;
                }
            }
            else  /* 2月 */
            {
                if((stime[0] % 4U) == 0U)
                {
                	if(stime[2] <= 29U)
                	{
                        ret = true;
                	}
                }
                else
                {
                	if(stime[2] <= 28U)
                	{
                        ret = true;
                	}
                }
            }
        }
    }
    return ret;
}

/**********************************************************************************
 * Function       : r8025_write
 * Author         : Linfei
 * Date           : 2012.07.11
 * Description    : 设定RTC时间，顺序为：年 月 日 时 分 秒 BCD码
 * Calls          : r8025_I2C_WriteByte
 * Input          : buf:保存待设置的时间
 *　　　　　　　  : count:待设置时间数组长度，固定为6
 * Output         : None
 * Return         : 0：设置失败
 *                  6：设置成功
 ***********************************************************************************/
int32_t r8025_write(const uint8_t buf[], uint32_t count)
{
    int32_t ret = 0;
    if((r8025_opened == true) && (buf != NULL) && (count >= 6U))
    {
        if(r8025_checktimer((uint8_t *)buf, 1U) == true)   
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
	        if(checkTcount > r8025_refTcount)	  /* 合法性判断，与参考时间对比 */
	        {	        
            /* 设置24小时制 */
	            if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_CTROL1_ADDR, R8025_SET_24_TYPE) == true)
	            {
	                /* 设置年月日时分秒 */
	                if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_SECOND_ADDR, buf[5]) == true)
	                {
	                    if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_MINUTE_ADDR, buf[4]) == true)
	                    {
	                        if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_HOUR_ADDR, buf[3]) == true)
	                        {
	                            if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_DATE_ADDR, buf[2]) == true)
	                            {
	                                if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_MONTH_ADDR, buf[1]) == true)
	                                {
	                                    if(r8025_I2C_WriteByte(R8025_ADDRESS, R8025_YEAR_ADDR, buf[0]) == true)
	                                    {
	                                        ret = 6;  //wujing 2012.11.28
	                                    }
	                                }
	                            }
	                        }
	                    }
	                }
	            }
			}
        }
    }
    return ret;
}




/**********************************************************************************
 * Function       : r8025_release
 * Author         : Linfei
 * Date           : 2012.07.12
 * Description    : RTC程序关闭
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : DSUCCESS：设置成功
 ***********************************************************************************/

int32_t r8025_release(void)
{
    if(r8025_opened == true)
    {
        device_gpio_config(DEVICE_ID_RTC, FALSE);
        r8025_opened = false;
    }
    return DSUCCESS;
}

