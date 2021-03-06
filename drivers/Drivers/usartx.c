/****************************************************************************** 
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :usartx.c 
 * Author         :Xu Shun'an
 * Date           :2011.05.27
 * Description    :STM32F10x系列处理器的串口模块函数，包括打开串口、关闭串口、
 *                 读串口数据、写串口数据和配置串口
 * Others         :None
 *******************************************************************************
 *-------------------------------------------------------------------------------
 * 2011-06-01 : 1.0.1 : xusa
 * Modification   :  整理代码格式
 *-------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 ********************************************************************************/
#include "usartx.h"
#include "fcntl.h"

typedef struct
{
    bool opened ; /*是否被打开*/
    bool rend   ; /*接收结束标志*/
    bool send   ; /*发送结束标志*/
    bool rs485  ; /*485支持*/
    uint8_t sa     ; /*同步异步(sa=0异步;sa=1同步)*/
    uint8_t ridle  ;     /*接收空闲计数*/
    uint8_t sidle  ;     /*发送空闲计数*/
    uint16_t slen   ;     /*发送长度*/
    uint16_t scont  ;    /*已经发送长度*/
    uint16_t rcont  ;    /*接收计数*/
    uint16_t sbuflen;  /*缓冲长度*/
    uint16_t rbuflen;  /*缓冲长度*/
    uint8_t *sbuf   ;     /*发送缓冲*/
    uint8_t *rbuf   ;     /*接收缓冲*/
} TUsartx_Buf;




/* 各个串口的表述符表*/
volatile TUsartx_Buf ub[USART_NUM];

/* 系统中所有的串口，这样做是为了方便下面用uid对其进行索引*/
/* 注意,UART4和UART5比较特殊，仅是异步接口，不支持硬件流控制、同步和智能卡模式*/
USART_TypeDef *const usart[] = 
{
    USART1, USART2, USART3, UART4, UART5,USART6
};
uint32_t usartrcc[] = 
{
    RCC_APB2Periph_USART1, RCC_APB1Periph_USART2, RCC_APB1Periph_USART3,
    RCC_APB1Periph_UART4, RCC_APB1Periph_UART5,RCC_APB2Periph_USART6
};
uint8_t usartirqn[] = 
{
    USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn,USART6_IRQn
};

GPIO_CTL uport[USART_NUM];

USART_InitTypeDef USART_InitStructure; //wujing 2012.12.04 add for change BAUD

/* RTU两帧之间间隔(ms)，波特率9600以上为3ms，以下则在串口初始化函数中计算，此时大于3ms*/
uint8_t time_out = 3U;

/* 同步发送完一组数据后的等待时间*/
volatile uint8_t rtu_wait;

int32_t usartx_open(int32_t uid, int32_t flags, int32_t mode);
int32_t usartx_write(int32_t uid, const uint8_t buf[], uint32_t count);
int32_t usartx_read(int32_t uid, uint8_t buf[], uint32_t count);
int32_t usartx_ioctl(int32_t uid, uint32_t cmd, void *arg);
int32_t usartx_close(int32_t uid);
void USARTX_IRQHandler(int32_t uid);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void USART3_IRQHandler(void);
void UART4_IRQHandler(void);
void UART5_IRQHandler(void);
bool uid_valid(int32_t id);

/* 判断串口的id是否有效,STM32串口编号为0,1,2,3,4,5   最多6路串口*/
bool uid_valid(int32_t id)
{
    bool ret;
    if((id >= 0) && (id < (int32_t)USART_NUM))
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    return ret;
}

 
/**********************************************************************************
 * Function       : usartx_timer
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 判断总线空闲是否超时,若超时则认为传输完毕，将相应传输结束标志 
 *                  置1,在系统时钟为72MHz时，函数调用时间为45us，堆栈使用了40 bytes
 * Calls          : None 
 * Input          : None
 * Output         : None
 * Return         : None 
 **********************************************************************************
 *---------------------------------------------------------------------------------
 * 2011-06-08 : 1.0.1 : xusa
 * Modification   :  整理代码格式.
 *---------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 **********************************************************************************/
void usartx_timer(void)
{

    for (uint8_t i = 0U; i < USART_NUM; ++i)
    {
        if (ub[i].opened)
        {
            /* 接收空闲计数*/
            if (ub[i].ridle && (!--ub[i].ridle))
            {
                ub[i].rend = true;
            }

            /* 发送空闲空闲计数*/
            if (ub[i].sidle && (!--ub[i].sidle))
            {
                ub[i].send = true;
                
            }
        }
    }

    if ((rtu_wait)!= 0U)
    {
        rtu_wait--;
    }

}


/****************************************************************************** 
 * Function       : USARTX_IRQHandler
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 通用串口中断服务程序，堆栈使用了40 bytes
 * Calls          : USART_GetITStatus，USART_ReceiveData，USART_ITConfig，
 *                : USART_ClearITPendingBit，USART_SendData，GPIO_ResetBits
 * Input          : 串口号，若有5路串口，分别为0,1,2,3,4
 * Output         : None
 * Return         : None 
 *******************************************************************************
 *------------------------------------------------------------------------------
 * 2011-06-08 : 1.0.1 : xusa
 * Modification   :  整理代码格式.
 *------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 *******************************************************************************/
void USARTX_IRQHandler(int32_t uid)
{
    uint8_t ch = 0U;
    uint16_t tmp;/*for MISRA-2004*/
    

    /* 总线空闲*/
    if (USART_GetITStatus(usart[uid], USART_IT_IDLE) != RESET)
    {
       /* 通过GetITStatus和ReceiveData操作序列来清除中断标志位*/
        tmp = USART_ReceiveData(usart[uid]);/*for MISRA-2004*/
        ch = (uint8_t)tmp;
        ub[uid].ridle = time_out;
        ub[uid].sidle = time_out;
        USART_ITConfig(usart[uid], USART_IT_IDLE, DISABLE);
    }

    /* 接收到数据*/
    if (USART_GetITStatus(usart[uid], USART_IT_RXNE) != RESET)
    {
        /* Clear the USART1 Receive interrupt */
        USART_ClearITPendingBit(usart[uid], USART_IT_RXNE);

        tmp = USART_ReceiveData(usart[uid]);
        ch  = (uint8_t)tmp;

        if ((!ub[uid].rend) && (ub[uid].rcont < ub[uid].rbuflen))
        {
            USART_ITConfig(usart[uid], USART_IT_IDLE, ENABLE);
            ub[uid].ridle = time_out;
            ub[uid].rbuf[ub[uid].rcont] = ch;
            ub[uid].rcont++;
        }
        else
        {
            ub[uid].rend = true;
            USART_ITConfig(usart[uid], USART_IT_IDLE, DISABLE);
            ub[uid].ridle = time_out;
        }
    }

    /* 发送完毕一个字节*/
    if (USART_GetITStatus(usart[uid], USART_IT_TXE) != RESET)
    {
        /* Clear the USARTx transmit interrupt */
        USART_ClearITPendingBit(usart[uid], USART_IT_TC);

        ub[uid].scont++;

        if (ub[uid].scont >= ub[uid].slen)
        {
            USART_ITConfig(usart[uid], USART_IT_TC, ENABLE);
            USART_ITConfig(usart[uid], USART_IT_TXE, DISABLE);
            /* 表示设备已经空闲*/
            ub[uid].slen = 0U;
            ub[uid].sidle = time_out;
        }
        else
        {
            USART_SendData(usart[uid], ub[uid].sbuf[ub[uid].scont]);
        }
    }

    if (USART_GetITStatus(usart[uid], USART_IT_TC) != RESET)
    {
        USART_ITConfig(usart[uid], USART_IT_TC, DISABLE);

        if (ub[uid].rs485)
        {
            GPIO_ResetBits(uport[uid].port, uport[uid].pin);
        }
    }
}


/****************************************************************************** 
 * Function       : usartx_open
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 通用串口初始化，打开串口函数,在系统频率为70MHz时，函数调用
 *                  时间约为250us，堆栈使用了56 bytes
 * Calls          : NVIC_Init， RCC_APB2PeriphClockCmd，RCC_APB1PeriphClockCmd，
 *                : USART_Init，USART_ITConfig，USART_Cmd
 * Input          : 串口号，串口通信方式，中断优先级
 * Output         : None
 * Return         : OPFAULT，串口号错误，串口打开失败；DSUCCESS，串口打开成功
 *******************************************************************************
 *------------------------------------------------------------------------------
 * 2011-06-08 : 1.0.1 : xusa
 * Modification   :  整理代码格式.
 *------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 *******************************************************************************/
int32_t usartx_open(int32_t uid, int32_t flags, int32_t mode)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    int32_t ret = OPFAULT;
    if(uid_valid(uid) == true)
    {
        if (ub[uid].opened != true)
        {
            ub[uid].opened = true;

            // 开启总线时钟
            if ((uid == 0)||(uid == 5))
            {
                RCC_APB2PeriphClockCmd(usartrcc[uid], ENABLE);
            }
            else
            {
                RCC_APB1PeriphClockCmd(usartrcc[uid], ENABLE);
            }

            // 开启串口中断
            if (flags & O_NONBLOCK)
            {
                NVIC_InitStructure.NVIC_IRQChannel = usartirqn[uid];
                NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = (mode >> 8)&0xFF;
                NVIC_InitStructure.NVIC_IRQChannelSubPriority = mode &0xFF;
                NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
                NVIC_Init(&NVIC_InitStructure);
            }

            // 串口默认配置
            USART_InitStructure.USART_BaudRate = DEFAULT_BAUD;	//上电默认波特率为9600
            USART_InitStructure.USART_WordLength = USART_WordLength_8b;
            USART_InitStructure.USART_StopBits = USART_StopBits_1;
            USART_InitStructure.USART_Parity = USART_Parity_No;
            USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
            USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
            USART_Init(usart[uid], &USART_InitStructure);

            // 设置超时间隔(ms)
            time_out = (44000 / DEFAULT_BAUD) >= 3 ? (uint8_t)(44000 / DEFAULT_BAUD): 3;

            // 允许接收中断
            USART_ITConfig(usart[uid], USART_IT_RXNE, ENABLE);

            // 初始时,设备处于接收状态
            ub[uid].ridle = 0;
            ub[uid].sidle = 0;
            ub[uid].rend = false;
            ub[uid].send = true;

            // 同步模式下,不需要缓冲区,可以使能串口
            if (flags &O_SYNC)
            {
                ub[uid].sa = true;
                USART_Cmd(usart[uid], ENABLE);
            }
            // 异步模式下,必须设置好缓冲区后,才使能串口
            else
            {
                ub[uid].sa = false;
            }

            ub[uid].slen = 0U;
            ub[uid].scont = 0U;
            ub[uid].rcont = 0U;
            ub[uid].sbuf = NULL;
            ub[uid].sbuflen = 0U;
            ub[uid].rbuf = NULL;
            ub[uid].rbuflen = 0U;
            ret = DSUCCESS;
        }
    }
    return ret;
}


/******************************************************************************* 
 * Function       : usartx_close
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 通用串口关闭函数,在系统频率为70MHz时，函数调用时间约为45us，
 *                  堆栈使用了16 bytes
 * Calls          : RCC_APB2PeriphClockCmd，RCC_APB1PeriphClockCmd，USART_DeInit
 * Input          : 串口号，串口通信方式，中断优先级
 * Output         : None
 * Return         : OPFAULT，串口号错误，关闭失败；DSUCCESS，串口成功关闭
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2011-06-08 : 1.0.1 : xusa
 * Modification   :  整理代码格式.
 *-------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 ********************************************************************************/
int32_t usartx_close(int32_t uid)
{
    int32_t ret;
    if (!uid_valid(uid))
    {
        ret = OPFAULT;
    }

    else if (!ub[uid].opened)
    {
        ret = DSUCCESS;
    }
    else
    {
        if (uid == 0)
        {
            RCC_APB2PeriphClockCmd(usartrcc[uid], DISABLE);
        }
        else
        {
            RCC_APB1PeriphClockCmd(usartrcc[uid], DISABLE);
        }

        ub[uid].opened = false;

        USART_DeInit(usart[uid]);
        ret = DSUCCESS;
    }
    return ret;
}


/******************************************************************************* 
 * Function       : usartx_write 
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 通用串口写函数,在系统频率为70MHz时，函数调用时间约为15ms，堆
 *                  栈使用了40 bytes
 * Calls          : GPIO_SetBits，memcpy，USART_SendData，USART_ITConfig，
 *                  USART_GetFlagStatus，GPIO_ResetBits
 * Input          : 串口号，串口待写入数据存储位置的指针，待写入数据长度
 * Output         : None
 * Return         : OPFAULT，串口号错误，写入失败；EBUSY，串口当前正在发送数据，
 *                  写入失败；count，写入成功，返回写入数据长度
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2011-06-08 : 1.0.1 : xusa
 * Modification   :  整理代码格式.
 *-------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 ********************************************************************************/
int32_t usartx_write(int32_t uid, const uint8_t buf[], uint32_t count)
{
    bool tmp;       /*for MISRA-2004*/
    int32_t ret;    /*for MISRA-2004*/
    uint32_t ret1;  /*for MISRA-2004*/
    if (!uid_valid(uid))
    {
        ret = OPFAULT;
    }

    /* 判忙*/
    else if (!ub[uid].send)
    {
        ret = DEVBUSY;
    }
    else
    {
        /* 配置485为发送*/
        if (ub[uid].rs485)
        {
            GPIO_SetBits(uport[uid].port, uport[uid].pin);
        }

        /* 异步发送*/
        tmp = (bool)ub[uid].sa; /*for MISRA-2004*/
        if (!tmp)
        {
            /* 检查是否设置了缓冲区*/
            if (ub[uid].sbuf == NULL)
            {
                ret = OPFAULT;
            }

            /* 检查缓冲区溢出*/
            else if (count > ub[uid].sbuflen)
            {
                ret = OPFAULT;
            }
            else
            {
                ub[uid].send = false;
                ub[uid].slen = (uint16_t)(count &0xFFFFU);
                ub[uid].scont = 0U;
                ret1 = (uint32_t)ub[uid].slen;
                if(ret1 > 0U)
                {
                    memcpy(ub[uid].sbuf, &buf[0], ret1);
                }
                /* 这里只发送一个字节,其余的字节在中断例程中发送  */
                USART_SendData(usart[uid], buf[0]);

                /* 允许发送完毕一个字节中断*/
                USART_ITConfig(usart[uid], USART_IT_TXE, ENABLE);
                ret1 = count &0xFFFFU;  /*for MISRA-2004*/
                ret = (int32_t)ret1;    /*for MISRA-2004*/
            }
        }
        /* 同步发送*/
        else
        {
            USART_ITConfig(usart[uid], USART_IT_TXE, DISABLE);

            for (uint32_t i = 0U; i < count; i++)
            {
                USART_SendData(usart[uid], buf[i]);
                while (USART_GetFlagStatus(usart[uid], USART_FLAG_TC) == RESET)
                {
                    ;
                }
            }
            if (ub[uid].rs485)
            {
                GPIO_ResetBits(uport[uid].port, uport[uid].pin);
            }
            /*发送完一组数据后,延时*/
            rtu_wait = time_out;

            while (rtu_wait!= 0U)
            {
                ;
            }

            ret = (int32_t)count;
        }

    }
    return ret;
}


/******************************************************************************* 
 * Function       : usartx_read
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 通用串口读函数，在系统时钟为72MHz，函数调用时间约为13ms，堆
 *                  栈使用了24 bytes
 * Calls          : memcpy
 * Input          : 串口号，串口待读入数据存储位置的指针，待读入数据长度
 * Output         : None
 * Return         : 0，读串口数据失败；count,读串口数据成功，返回所读取数据的长度
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2011-06-08 : 1.0.1 : xusa
 * Modification   :  整理代码格式.
 *-------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 ********************************************************************************/
int32_t usartx_read(int32_t uid, uint8_t buf[], uint32_t count)
{
    int32_t ret;
    if ((!uid_valid(uid)) || (ub[uid].rbuf == NULL))
    {
        ret = 0;
    }

    /* rend置位有两种途径:缓冲区满/超时*/
    else if (!ub[uid].rend)
    {
        ret = 0;
    }
    else
    {
        uint32_t len = ub[uid].rcont;

        if (len > count)
        {
            len = count;
        }


        memcpy(&buf[0], ub[uid].rbuf, len);

        ub[uid].rend = false;
        ub[uid].rcont = 0U;



        ret = (int32_t)len;
    }

    return ret;
}


/****************************************************************************** 
 * Function       : usartx_ioctl
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 通用串口配置函数,在系统时钟为72MHz时，函数调用时间约为65us
 *                  堆栈使用了40 bytes
 * Calls          : USART_Init，USART_Cmd，GPIO_ResetBits，
 * Input          : 串口号，配置类型，串口重新配置结构体的指针
 * Output         : None
 * Return         : OPFAULT;，串口配置失败；DSUCCESS，串口配置成功
 *******************************************************************************
 *------------------------------------------------------------------------------
 * 2011-06-08 : 1.0.1 : xusa
 * Modification   :  整理代码格式.
 *------------------------------------------------------------------------------
 * 2010-08-03 : 1.0.0 : liulei
 * Modification   : 初始代码编写。
 *******************************************************************************/
int32_t usartx_ioctl(int32_t uid, uint32_t cmd, void *arg)
{
    float tmp;            /*for MISRA-2004*/
    int32_t ret = DSUCCESS;  /*for MISRA-2004*/
    TUsartX_ioctl *s = (TUsartX_ioctl*)arg;
	uint32_t *baud = (uint32_t*)arg; //串口波特率设置和获取

    if (!uid_valid(uid))
    {
        ret = OPFAULT;
    }
    else
    {   switch (cmd)
        {
            /* 重新初始化串口*/
            case USART_SET_PORT:
            if (s == NULL)
            {
                ret = OPFAULT;
            }
            else
            {
                USART_InitStructure.USART_BaudRate = s->baudrate;
                USART_InitStructure.USART_WordLength = s->wordlength;
                USART_InitStructure.USART_StopBits = s->stopbits;
                USART_InitStructure.USART_Parity = s->parity;
                USART_InitStructure.USART_HardwareFlowControl = s->flowcontrol;
                USART_InitStructure.USART_Mode = s->mode;
                USART_Init(usart[uid], &USART_InitStructure);
                tmp = 44000.0 / (float)s->baudrate;  /*for MISRA-2004*/
                if(tmp >= (float)3)
                {
                    time_out = (uint8_t)tmp;
                }
                else
                {
                    time_out = (uint8_t)3;
                }
            }
            break;
			case USART_SET_BAUD:	//设置串口波特率
            if (s == NULL)
            {
                ret = OPFAULT;
            }
			else
			{
				USART_InitStructure.USART_BaudRate = *baud;
                USART_Init(usart[uid], &USART_InitStructure);	
			}
			break;
			case USART_GET_BAUD:	//获取串口波特率
            if (s == NULL)
            {
                ret = OPFAULT;
            }
			else
			{
		   		*baud = USART_InitStructure.USART_BaudRate;	
			}
			break;
            /* 设置串口缓冲区*/
            case USART_SET_BUF:
            if (s == NULL)
            {
                ret = OPFAULT;
            }

            else if ((s->rbuf == NULL) || (s->sbuf == NULL))
            {
                ret = OPFAULT;
            }
            else
            {
                ub[uid].rbuf = s->rbuf;
                ub[uid].rbuflen = s->rbuflen;
                ub[uid].sbuf = s->sbuf;
                ub[uid].sbuflen = s->sbuflen;
                USART_Cmd(usart[uid], ENABLE);
            }
            break;

            /* 开启485支持      */
            case USART_SET_485E:
            if (uport[uid].port == NULL)
            {
                ret = OPFAULT;
            }
            else
            {
                ub[uid].rs485 = true;
                /* 准备接收*/
                GPIO_ResetBits(uport[uid].port, uport[uid].pin);
            }
            break;

            /* 关闭485支持*/
            case USART_SET_485D:
            if (uport[uid].port == NULL)
            {
                ret = OPFAULT;
            }
            else
            {
                ub[uid].rs485 = false;
            }
            break;

            case USART_SET_SYNC:
            if ((uid == 3) || (uid == 4))
            {
                ret = OPFAULT;
            }
            else
            {
                ub[uid].sa = (uint8_t)true;
                USART_Cmd(usart[uid], ENABLE);
            }
            break;

            case USART_SET_ASYN:
            ub[uid].sa = (uint8_t)false;
            break;

            default:
            ret = OPFAULT;
            break;
        }
    }
    return ret; /*for MISRA-2004*/
}


/****************************************************************************** 
 * Function       : USART1_IRQHandler
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口1中断服务子程序
 * Calls          : USARTX_IRQHandler
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART1_IRQHandler(void)
{
    USARTX_IRQHandler(0);
}


/****************************************************************************** 
 * Function       : usart1_open
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口1打开函数
 * Calls          : usartx_open
 * Input          : 串口1通信方式，串口1中断优先级
 * Output         : None
 * Return         : OPFAULT，串口1打开失败；DSUCCESS，串口1打开成功
 *******************************************************************************/
int32_t usart1_open(int32_t flags, int32_t mode)
{
    device_gpio_config(DEVICE_ID_RS485_1, TRUE);
    uport[0] = get_gpio_ctl_attr(DEVICE_ID_RS485_1, PIN_GPIO_485_EN);
    return usartx_open(0, flags, mode);
}


/****************************************************************************** 
 * Function       : usart1_close
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口1关闭函数
 * Calls          : usartx_release
 * Input          : None
 * Output         : None
 * Return         : OPFAULT，串口1关闭失败；DSUCCESS，串口1关闭成功
 *******************************************************************************/
int32_t usart1_close(void)
{
    device_gpio_config(DEVICE_ID_RS485_1, FALSE);
    return usartx_close(0);
}


/****************************************************************************** 
 * Function       : usart1_write
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口1写数据函数
 * Calls          : usartx_write
 * Input          : 串口1待写入数据存储位置的指针，待写入数据长度
 * Output         : None
 * Return         : OPFAULT，串口1写入失败；EBUSY，串口1正在发送数据，写入失败；
 *                  count，写入成功，返回写入数据长度
 *******************************************************************************/
int32_t usart1_write(const uint8_t buf[], uint32_t count)
{
    return usartx_write(0, buf, count);
}


/****************************************************************************** 
 * Function       : usart1_read
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口1读取数据函数
 * Calls          : usartx_read
 * Input          : 串口1待读入数据存储位置的指针，待读入数据长度
 * Output         : None
 * Return         : 0，串口1读取数据失败；
 *                  count,串口1读取数据成功，返回所读取数据的长度
 *******************************************************************************/
int32_t usart1_read( uint8_t buf[], uint32_t count)
{
    return usartx_read(0, buf, count);
}


/****************************************************************************** 
 * Function       : usart1_ioctl
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口1配置函数
 * Calls          : usartx_ioctl
 * Input          : 配置类型，串口重新配置结构体的指针
 * Output         : None
 * Return         : OPFAULT;，串口1配置失败；DSUCCESS，串口1配置成功
 *******************************************************************************/
int32_t usart1_ioctl(uint32_t cmd, void *arg)
{
    return usartx_ioctl(0, cmd, arg);
}


/****************************************************************************** 
 * Function       : USART2_IRQHandler
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口2中断服务子程序
 * Calls          : USARTX_IRQHandler
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART2_IRQHandler(void)
{
    USARTX_IRQHandler(1);
}


/****************************************************************************** 
 * Function       : usart2_open
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口1打开函数
 * Calls          : usartx_open
 * Input          : 串口2通信方式，串口2中断优先级
 * Output         : None
 * Return         : OPFAULT，串口2打开失败；DSUCCESS，串口2打开成功
 *******************************************************************************/
int32_t usart2_open(int32_t flags, int32_t mode)
{
    device_gpio_config(DEVICE_ID_RS485_2, TRUE);
    uport[1] = get_gpio_ctl_attr(DEVICE_ID_RS485_2, PIN_GPIO_485_EN);
    return usartx_open(1, flags, mode);
}


/****************************************************************************** 
 * Function       : usart2_close
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口2关闭函数
 * Calls          : usartx_release
 * Input          : None
 * Output         : None
 * Return         : OPFAULT，串口2关闭失败；DSUCCESS，串口2关闭成功
 *******************************************************************************/
int32_t usart2_close(void)
{
    device_gpio_config(DEVICE_ID_RS485_2, FALSE);
    return usartx_close(1);
}


/****************************************************************************** 
 * Function       : usart2_write
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口2写数据函数
 * Calls          : usartx_write
 * Input          : 串口2待写入数据存储位置的指针，待写入数据长度
 * Output         : None
 * Return         : OPFAULT，串口2写入失败；EBUSY，串口2正在发送数据，写入失败；
 *                  count，写入成功，返回写入数据长度
 *******************************************************************************/
int32_t usart2_write(const uint8_t buf[], uint32_t count)
{
    return usartx_write(1, buf, count);
}


/****************************************************************************** 
 * Function       : usart2_read
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口2读取数据函数
 * Calls          : usartx_read
 * Input          : 串口2待读入数据存储位置的指针，待读入数据长度
 * Output         : None
 * Return         : 0，串口2读取数据失败；
 *                  count,串口2读取数据成功，返回所读取数据的长度
 *******************************************************************************/
int32_t usart2_read(uint8_t buf[], uint32_t count)
{
    return usartx_read(1, buf, count);
}


/****************************************************************************** 
 * Function       : usart2_ioctl
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口2配置函数
 * Calls          : usartx_ioctl
 * Input          : 配置类型，串口重新配置结构体的指针
 * Output         : None
 * Return         : OPFAULT;，串口2配置失败；DSUCCESS，串口2配置成功
 *******************************************************************************/
int32_t usart2_ioctl(uint32_t cmd, void *arg)
{
    return usartx_ioctl(1, cmd, arg);
}


/****************************************************************************** 
 * Function       : USART3_IRQHandler
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口3中断服务子程序
 * Calls          : USARTX_IRQHandler
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART3_IRQHandler(void)
{
    USARTX_IRQHandler(2);
}


/****************************************************************************** 
 * Function       : usart3_open
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口3打开函数
 * Calls          : usartx_open
 * Input          : 串口3通信方式，串口3中断优先级
 * Output         : None
 * Return         : OPFAULT，串口3打开失败；DSUCCESS，串口3打开成功
 *******************************************************************************/
int32_t usart3_open(int32_t flags, int32_t mode)
{
    device_gpio_config(DEVICE_ID_RS485_3, TRUE);
    uport[2] = get_gpio_ctl_attr(DEVICE_ID_RS485_3, PIN_GPIO_485_EN);
    return usartx_open(2, flags, mode);
}


/****************************************************************************** 
 * Function       : usart3_close
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口3关闭函数
 * Calls          : usartx_release
 * Input          : None
 * Output         : None
 * Return         : OPFAULT，串口3关闭失败；DSUCCESS，串口3关闭成功
 *******************************************************************************/
int32_t usart3_close(void)
{
    device_gpio_config(DEVICE_ID_RS485_3, FALSE);
    return usartx_close(2);
}


/****************************************************************************** 
 * Function       : usart3_write
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口3写数据函数
 * Calls          : usartx_write
 * Input          : 串口3待写入数据存储位置的指针，待写入数据长度
 * Output         : None
 * Return         : OPFAULT，串口3写入失败；EBUSY，串口3正在发送数据，写入失败；
 *                  count，写入成功，返回写入数据长度
 *******************************************************************************/
int32_t usart3_write(const uint8_t buf[], uint32_t count)
{
    return usartx_write(2, buf, count);
}


/****************************************************************************** 
 * Function       : usart3_read
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口3读取数据函数
 * Calls          : usartx_read
 * Input          : 串口3待读入数据存储位置的指针，待读入数据长度
 * Output         : None
 * Return         : 0，串口3读取数据失败；
 *                  count,串口3读取数据成功，返回所读取数据的长度
 *******************************************************************************/
int32_t usart3_read(uint8_t buf[], uint32_t count)
{
    return usartx_read(2, buf, count);
}


/****************************************************************************** 
 * Function       : usart3_ioctl
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口3配置函数
 * Calls          : usartx_ioctl
 * Input          : 配置类型，串口重新配置结构体的指针
 * Output         : None
 * Return         : OPFAULT;，串口3配置失败；DSUCCESS，串口3配置成功
 *******************************************************************************/
int32_t usart3_ioctl(uint32_t cmd, void *arg)
{
    return usartx_ioctl(2, cmd, arg);
}


/****************************************************************************** 
 * Function       : USART4_IRQHandler
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口4中断服务子程序
 * Calls          : USARTX_IRQHandler
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UART4_IRQHandler(void)
{
    USARTX_IRQHandler(3);
}


/****************************************************************************** 
 * Function       : usart4_open
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口4打开函数
 * Calls          : usartx_open
 * Input          : 串口4通信方式，串口4中断优先级
 * Output         : None
 * Return         : OPFAULT，串口4打开失败；DSUCCESS，串口4打开成功
 *******************************************************************************/
int32_t usart4_open(int32_t flags, int32_t mode)
{
    device_gpio_config(DEVICE_ID_RS485_4, TRUE);
    uport[3] = get_gpio_ctl_attr(DEVICE_ID_RS485_4, PIN_GPIO_485_EN);
    return usartx_open(3, flags, mode);
}


/****************************************************************************** 
 * Function       : usart4_close
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口4关闭函数
 * Calls          : usartx_release
 * Input          : None
 * Output         : None
 * Return         : OPFAULT，串口4关闭失败；DSUCCESS，串口4关闭成功
 *******************************************************************************/
int32_t usart4_close(void)
{
    device_gpio_config(DEVICE_ID_RS485_4, FALSE);
    return usartx_close(3);
}


/****************************************************************************** 
 * Function       : usart4_write
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口4写数据函数
 * Calls          : usartx_write
 * Input          : 串口4待写入数据存储位置的指针，待写入数据长度
 * Output         : None
 * Return         : OPFAULT，串口4写入失败；EBUSY，串口4正在发送数据，写入失败；
 *                  count，写入成功，返回写入数据长度
 *******************************************************************************/
int32_t usart4_write(const uint8_t buf[], uint32_t count)
{
    return usartx_write(3, buf, count);
}


/****************************************************************************** 
 * Function       : usart4_read
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口4读取数据函数
 * Calls          : usartx_read
 * Input          : 串口4待读入数据存储位置的指针，待读入数据长度
 * Output         : None
 * Return         : 0，串口4读取数据失败；
 *                  count,串口4读取数据成功，返回所读取数据的长度
 *******************************************************************************/
int32_t usart4_read(uint8_t buf[], uint32_t count)
{
    return usartx_read(3, buf, count);
}


/****************************************************************************** 
 * Function       : usart4_ioctl
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口4配置函数
 * Calls          : usartx_ioctl
 * Input          : 配置类型，串口重新配置结构体的指针
 * Output         : None
 * Return         : OPFAULT;，串口4配置失败；DSUCCESS，串口4配置成功
 *******************************************************************************/
int32_t usart4_ioctl(uint32_t cmd, void *arg)
{
    return usartx_ioctl(3, cmd, arg);
}


/****************************************************************************** 
 * Function       : USART5_IRQHandler
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口5中断服务子程序
 * Calls          : USARTX_IRQHandler
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UART5_IRQHandler(void)
{
    USARTX_IRQHandler(4);
}


/****************************************************************************** 
 * Function       : usart5_open
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口5打开函数
 * Calls          : usartx_open
 * Input          : 串口5通信方式，串口5中断优先级
 * Output         : None
 * Return         : OPFAULT，串口5打开失败；DSUCCESS，串口5打开成功
 *******************************************************************************/
int32_t usart5_open(int32_t flags, int32_t mode)
{
    device_gpio_config(DEVICE_ID_RS485_5, TRUE);
    uport[4] = get_gpio_ctl_attr(DEVICE_ID_RS485_5, PIN_GPIO_485_EN);
    return usartx_open(4, flags, mode);
}


/****************************************************************************** 
 * Function       : usart1_close
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口5关闭函数
 * Calls          : usartx_release
 * Input          : None
 * Output         : None
 * Return         : OPFAULT，串口5关闭失败；DSUCCESS，串口5关闭成功
 *******************************************************************************/
int32_t usart5_close(void)
{
    device_gpio_config(DEVICE_ID_RS485_5, FALSE);
    return usartx_close(4);
}


/****************************************************************************** 
 * Function       : usart5_write
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口5写数据函数
 * Calls          : usartx_write
 * Input          : 串口5待写入数据存储位置的指针，待写入数据长度
 * Output         : None
 * Return         : OPFAULT，串口5写入失败；EBUSY，串口5正在发送数据，写入失败；
 *                  count，写入成功，返回写入数据长度
 *******************************************************************************/
int32_t usart5_write(const uint8_t buf[], uint32_t count)
{
    return usartx_write(4, buf, count);
}


/****************************************************************************** 
 * Function       : usart5_read
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口5读取数据函数
 * Calls          : usartx_read
 * Input          : 串口5待读入数据存储位置的指针，待读入数据长度
 * Output         : None
 * Return         : 0，串口5读取数据失败；
 *                  count,串口5读取数据成功，返回所读取数据的长度
 *******************************************************************************/
int32_t usart5_read(uint8_t buf[], uint32_t count)
{
    return usartx_read(4, buf, count);
}


/****************************************************************************** 
 * Function       : usart5_ioctl
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口5配置函数
 * Calls          : usartx_ioctl
 * Input          : 配置类型，串口重新配置结构体的指针
 * Output         : None
 * Return         : OPFAULT;，串口5配置失败；DSUCCESS，串口5配置成功
 *******************************************************************************/
int32_t usart5_ioctl(uint32_t cmd, void *arg)
{
    return usartx_ioctl(4, cmd, arg);
}
/****************************************************************************** 
 * Function       : USART5_IRQHandler
 * Author         : Xu Shun'an
 * Date           : 2011.05.24
 * Description    : 串口5中断服务子程序
 * Calls          : USARTX_IRQHandler
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USART6_IRQHandler(void)
{
    USARTX_IRQHandler(5);
}
/****************************************************************************** 
 * Function       : usart6_open
 * Author         : wujing
 * Date           : 2013.03.13
 * Description    : 串口6打开函数
 * Calls          : usartx_open
 * Input          : 串口6通信方式，串口6中断优先级
 * Output         : None
 * Return         : OPFAULT，串口6打开失败；DSUCCESS，串口6打开成功
 *******************************************************************************/
int32_t usart6_open(int32_t flags, int32_t mode)
{
    device_gpio_config(DEVICE_ID_RS232_6, TRUE);
  //  uport[4] = get_gpio_ctl_attr(DEVICE_ID_RS485_5, PIN_GPIO_485_EN); //串口6方式非485操作
    return usartx_open(5, flags, mode);
}


/****************************************************************************** 
 * Function       : usart6_close
 * Author         : wujing  
 * Date           : 2013.03.13
 * Description    : 串口6关闭函数
 * Calls          : usartx_release
 * Input          : None
 * Output         : None
 * Return         : OPFAULT，串口6关闭失败；DSUCCESS，串口6关闭成功
 *******************************************************************************/
int32_t usart6_close(void)
{
    device_gpio_config(DEVICE_ID_RS232_6, FALSE);
    return usartx_close(5);
}


/****************************************************************************** 
 * Function       : usart6_write
 * Author         : wujing
 * Date           : 2013.03.13
 * Description    : 串口6写数据函数
 * Calls          : usartx_write
 * Input          : 串口6待写入数据存储位置的指针，待写入数据长度
 * Output         : None
 * Return         : OPFAULT，串口6写入失败；EBUSY，串口6正在发送数据，写入失败；
 *                  count，写入成功，返回写入数据长度
 *******************************************************************************/
int32_t usart6_write(const uint8_t buf[], uint32_t count)
{
    return usartx_write(5, buf, count);
}


/****************************************************************************** 
 * Function       : usart6_read
 * Author         : wujing  
 * Date           : 2013.03.13
 * Description    : 串口6读取数据函数
 * Calls          : usartx_read
 * Input          : 串口6待读入数据存储位置的指针，待读入数据长度
 * Output         : None
 * Return         : 0，串口6读取数据失败；
 *                  count,串口6读取数据成功，返回所读取数据的长度
 *******************************************************************************/
int32_t usart6_read(uint8_t buf[], uint32_t count)
{
    return usartx_read(5, buf, count);
}


/****************************************************************************** 
 * Function       : usart6_ioctl
 * Author         : wujing
 * Date           : 2013.03.13
 * Description    : 串口6配置函数
 * Calls          : usartx_ioctl
 * Input          : 配置类型，串口重新配置结构体的指针
 * Output         : None
 * Return         : OPFAULT;，串口6配置失败；DSUCCESS，串口6配置成功
 *******************************************************************************/
int32_t usart6_ioctl(uint32_t cmd, void *arg)
{
    return usartx_ioctl(5, cmd, arg);
}

