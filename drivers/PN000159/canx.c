/******************************************************************************
* Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
* File name      : canx.c
* Author         : Liulei
*                  Linfei
* Date           : 2011.07.18
* Description    : STM32F10x系列处理器的CAN通信模块函数，包括打开、关闭、接收、
*                  发送、配置函数
* Others         : None
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2012.07.28 : 1.0.2 : Linfei
* Modification   : 增加对STM32F4xx芯片支持
*------------------------------------------------------------------------------
* 2011.07.18 : 1.0.1 : Linfei
* Modification   : CAN2支持，STM32F103支持的bug，接收中断号错误
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
******************************************************************************/
#include "canx.h"
#include "fcntl.h"

/* CAN外设列表 */
CAN_TypeDef *can_list[MAX_CAN_NUM] = 
{
    CAN1,
#ifdef MAX_CAN_NUM
#if (MAX_CAN_NUM > 1)
    CAN2
#endif
#endif
};

/*lint -e46 */

/* CAN设备操作结构 */
typedef struct
{
    CanRxMsg rxmsg[MAX_CAN_BUF];   /* 接收缓冲队列 */
    bool    opened;             /* CAN设备打开标志 */
    uint8_t mute;
    uint8_t headp;             /* 接收缓冲队列头指针 */
    uint8_t endp;              /* 接收缓冲队列尾指针 */
    uint8_t curp;
} CANx_Buf;

CANx_Buf cb[MAX_CAN_NUM] = { 0U };

/** 各个CAN口的初始化结构体,声明为全局变量是为了方便在ioctl中调整其中某一个值
  * 而不改变其它的值
  */
CAN_InitTypeDef CAN_InitStructure[MAX_CAN_NUM];


/* 模块内部函数声明 */
bool IS_CAN_BAUD(uint32_t baudrate);
int32_t canx_open(uint32_t cid, int32_t flag, int32_t prio);
int32_t canx_release(uint32_t cid);
int32_t canx_read(uint32_t cid, uint8_t buf[], uint32_t count);
int32_t canx_write(uint32_t cid, const uint8_t buf[], uint32_t count);
int32_t canx_ioctl(uint32_t cid, uint32_t op, void *arg);
void CAN_RXx_IRQHandler(uint32_t cid);
void CAN1_RX0_IRQHandler(void);
void CAN2_RX0_IRQHandler(void);

#if SYS_OS_SUPPORT

OS_EVENT *m_canmsg_queue[MAX_CAN_NUM]={NULL};    /* upoll支持 */

#endif

/*******************************************************************************
 * Function       : IS_CAN_BAUD
 * Author         : Linfei
 * Date           : 2012.09.13
 * Description    : 判断是否是合法的设置波特率
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None
 ********************************************************************************
 *-------------------------------------------------------------------------------
 * 2012.09.13 : 1.0.0 : Linfei
 * Modification   : 初始代码编写
 ********************************************************************************/
bool IS_CAN_BAUD(uint32_t baudrate)
{
    bool ret = false;
    if((baudrate == CAN_BAUD_1Mbps)   || 
       (baudrate == CAN_BAUD_500Kbps) || 
       (baudrate == CAN_BAUD_250Kbps) || //wujing 2012.11.26 add for xu
       (baudrate == CAN_BAUD_200Kbps) || 
       (baudrate == CAN_BAUD_100Kbps) || 
       (baudrate == CAN_BAUD_80Kbps)  || 
       (baudrate == CAN_BAUD_30Kbps)  || 
       (baudrate == CAN_BAUD_20Kbps))
    {
        ret = true;
    }
    return ret;
}


/*******************************************************************************
* Function       : canx_open
* Author         : Liulei
*                  Linfei
* Date           : 2011.08.03
* Description    : CAN模块打开函数，完成指定CAN设备的初始化，默认波特率为500kbps，
*                  失败自动重传，离线后自动恢复在线，报文ID决定优先级，使用FIFO0，
*                  标识符及过滤设置为可以接收任何有效报文。
* Calls          : RCC_APB1PeriphClockCmd, NVIC_Init, CAN_DeInit, CAN_StructInit,
*                  CAN_Init, CAN_FilterInit, CAN_ITConfig
* Table Accessed : None
* Table Updated  : None
* Input          : cid: CAN控制器序号，STM32F107系列可以取0,1；
*                  flag: 未使用；
*                  mode: CAN控制器中断优先级，低16位有效，高八位为抢占优先级，低
*                        八位为子优先级。优先级取值需低于0x10，该处理有待改进
* Output         : None
* Return         : OPFAULT：用户传入参数不合法或CAN口已打开；
*                  DSUCCESS：CAN打开成功。
*
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2012.07.28 : 1.0.2 : Linfei
* Modification   : 增加对STM32F4xx芯片支持
*------------------------------------------------------------------------------
* 2011.08.03 : 1.0.1 : Linfei
* Modification   : 修复CAN2无法工作的bug：CAN2过滤器未设置，同时要开启CAN1时钟；
*                  CAN2开启状态下，若调用CAN_DeInit(CAN1)，会导致共享的过滤器复位，
*                  必须重新设置CAN2过滤器避免其使用的过滤器被屏蔽。接收中断号错误
*                  应为：USB_LP_CAN1_RX0_IRQn
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t canx_open(uint32_t cid, int32_t flag, int32_t prio)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    uint8_t result = 0U;
    int32_t ret = OPFAULT;
    uint32_t cannum = MAX_CAN_NUM;
    if((cid < cannum) && (cb[cid].opened != true))
    {
        uint32_t rprio = (uint32_t)prio;
        rprio &= 0xFFFFU;
        if(IS_NVIC_PREEMPTION_PRIORITY((rprio >> 8) & 0xFFU) &&
           IS_NVIC_SUB_PRIORITY(rprio & 0xFFU))
        {
            /* 根据cid开启对应CAN时钟，使用CAN2必须同时开启CAN1和CAN2的时钟 */
            if(cid == 0U)
            {
                RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE); 
            }
            else /*cid = 1U*/
            {
                RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);
            }

            /* 开启对应CAN的接收中断 */
        #if defined(STM32F4XX) || defined(STM32F10X_CL)
            if(cid == 0U)
            {
                NVIC_InitStructure.NVIC_IRQChannel = (uint8_t)CAN1_RX0_IRQn;
            }
            else/* cid = 1 */
            {
                NVIC_InitStructure.NVIC_IRQChannel = (uint8_t)CAN2_RX0_IRQn;
            }
        #else
            NVIC_InitStructure.NVIC_IRQChannel = (uint8_t)USB_LP_CAN1_RX0_IRQn;
        #endif
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = (uint8_t)((rprio >> 8) & 0xFFU);
            NVIC_InitStructure.NVIC_IRQChannelSubPriority = (uint8_t)rprio & 0xFFU;
            NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&NVIC_InitStructure);

            /* 复位CAN相关寄存器 */
            CAN_DeInit(can_list[cid]);
            CAN_StructInit(&CAN_InitStructure[cid]);

        #if defined(STM32F4XX)
            /* CAN时间单元tq=prescaler*tpclk其中tpclk为总线时间单元(1/42MHz)
             * 默认波特率 = 42M/(1+3+3)/6 = 1Mbps
             */
            CAN_InitStructure[cid].CAN_Prescaler = 6U;
            CAN_InitStructure[cid].CAN_SJW       = CAN_SJW_1tq;
            CAN_InitStructure[cid].CAN_BS1       = CAN_BS1_3tq;
            CAN_InitStructure[cid].CAN_BS2       = CAN_BS2_3tq;
        #else
            /* CAN时间单元tq=prescaler*tpclk其中tpclk为总线时间单元(1/36MHz)
             * 默认波特率 = 36M/(1+3+5)/8 = 500Kbps
             */
            CAN_InitStructure[cid].CAN_Prescaler = 8U;
            CAN_InitStructure[cid].CAN_SJW       = CAN_SJW_1tq;
            CAN_InitStructure[cid].CAN_BS1       = CAN_BS1_3tq;
            CAN_InitStructure[cid].CAN_BS2       = CAN_BS2_5tq;
        #endif
            CAN_InitStructure[cid].CAN_Mode      = CAN_Mode_Normal;
            CAN_InitStructure[cid].CAN_TTCM      = DISABLE;          /* 禁止时间触发通信模式 */
            CAN_InitStructure[cid].CAN_ABOM      = ENABLE;           /* 离线后自动退出离线 */
            CAN_InitStructure[cid].CAN_AWUM      = DISABLE;          /* 关闭自动唤醒 */
            CAN_InitStructure[cid].CAN_NART      = DISABLE;          /* 发送失败后自动重传 */
            CAN_InitStructure[cid].CAN_RFLM      = DISABLE;          /* FIFO不加锁 */
            CAN_InitStructure[cid].CAN_TXFP      = DISABLE;          /* 优先级由报文ID决定 */
            result = CAN_Init(can_list[cid], &CAN_InitStructure[cid]);
            if(result != CANINITFAILED)
            {
                /* CAN过滤器初始化 */
                CAN_FilterInitTypeDef CAN_FilterInitStructure;
                if(cid == 0U)
                {
                    /* CAN1过滤器初始化 */
                    CAN_FilterInitStructure.CAN_FilterIdHigh           = 0x0000U;                 /* CAN1标识符高位 */
                    CAN_FilterInitStructure.CAN_FilterIdLow            = 0x0000U;                 /* CAN1标识符低位 */
                    CAN_FilterInitStructure.CAN_FilterMaskIdHigh       = 0x0000U;                 /* CAN1屏蔽符高位 */
                    CAN_FilterInitStructure.CAN_FilterMaskIdLow        = 0x0000U;                 /* CAN1屏蔽符低位 */
                    CAN_FilterInitStructure.CAN_FilterFIFOAssignment   = 0U;                      /* 收到数据后存放到主FIFO0 */
                    CAN_FilterInitStructure.CAN_FilterNumber           = 0U;                      /* 开启0号过滤器 */
                    CAN_FilterInitStructure.CAN_FilterMode             = CAN_FilterMode_IdMask;  /* 标识符屏蔽模式 */
                    CAN_FilterInitStructure.CAN_FilterScale            = CAN_FilterScale_32bit;  /* 32位位宽 */
                    CAN_FilterInitStructure.CAN_FilterActivation       = ENABLE;
                    CAN_FilterInit(&CAN_FilterInitStructure);
                }

                /* 若先打开CAN2，后打开CAN1，则在CAN1复位操作时，CAN2的过滤器会被禁用，因此需重新设置 */
                if(cannum > 1U)
                {
                    if((cid == 1U) || (cb[1].opened == true))
                    {
                        /* CAN2过滤器初始化 */
                        CAN_SlaveStartBank(13U);                                                       /* CAN2起始过滤器号设为13 */
                        CAN_FilterInitStructure.CAN_FilterIdHigh           = 0x0000U;                 /* CAN2标识符高位 */
                        CAN_FilterInitStructure.CAN_FilterIdLow            = 0x0000U;                 /* CAN2标识符低位 */
                        CAN_FilterInitStructure.CAN_FilterMaskIdHigh       = 0x0000U;                 /* CAN2屏蔽符高位 */
                        CAN_FilterInitStructure.CAN_FilterMaskIdLow        = 0x0000U;                 /* CAN2屏蔽符低位 */
                        CAN_FilterInitStructure.CAN_FilterFIFOAssignment   = 0U;                      /* 收到数据后存放到主FIFO0 */
                        CAN_FilterInitStructure.CAN_FilterNumber           = 14U;                     /* 开启14号过滤器 */
                        CAN_FilterInitStructure.CAN_FilterMode             = CAN_FilterMode_IdMask;   /* 标识符屏蔽模式 */
                        CAN_FilterInitStructure.CAN_FilterScale            = CAN_FilterScale_32bit;   /* 32位位宽 */
                        CAN_FilterInitStructure.CAN_FilterActivation       = ENABLE;
                        CAN_FilterInit(&CAN_FilterInitStructure);
                    }
                }

                /* 使能对应CAN的FIFO0接收中断 */
                CAN_ITConfig(can_list[cid], CAN_IT_FMP0, ENABLE);

                cb[cid].opened = true;
                ret = DSUCCESS;
            }
        }
    }
    return ret;
}

/*******************************************************************************
* Function       : canx_release
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN关闭函数
* Calls          : CAN_DeInit
* Table Accessed : None
* Table Updated  : None
* Input          : cid: CAN口号，小于MAX_CAN_NUM
* Output         : None
* Return         : DSUCCESS: 关闭成功
*                  OPFAULT: CAN口号非法
*******************************************************************************
* History        :
* 2011.08.03 : 1.0.1 : Linfei
* Modification   : 加入cid合法性判断，关闭CAN时钟
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t canx_release(uint32_t cid)
{
    int32_t ret = OPFAULT;
    uint32_t cannum = MAX_CAN_NUM;
    if(cid < cannum)
    {
        ret = DSUCCESS;
        if(cb[cid].opened == true)
        {    
            if(cid == 0U)
            {
                /** 若仅CAN1打开则关闭CAN1时钟并复位，若CAN1、CAN2均已打开，
                  * 则不能关闭CAN1时钟和复位CAN1，否则会影响CAN2操作，因此，
                  * 在CAN1同时打开的情况下，关闭CAN1的操作仅是关闭CAN1中断。
                  */
                if(cannum > 1U)
                {
                    if(cb[1].opened != true)
                    {
                        RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, DISABLE);
                        CAN_DeInit(can_list[cid]);
                    }
                    else  /* 有CAN2并且已打开，则CAN1的关闭操作仅是关闭CAN1中断 */
                    {
                        CAN_ITConfig(can_list[cid], CAN_IT_FMP0, ENABLE);
                    }
                }
                else
                {
                    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, DISABLE);
                    CAN_DeInit(can_list[cid]);
                }
            }
            else
            {
                RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, DISABLE);
                if(cb[0].opened != true)
                {
                    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, DISABLE);
                }
                CAN_DeInit(can_list[cid]);        
            }
            
            cb[cid].opened = false;
        }
    }
    return ret;
}

/*******************************************************************************
* Function       : canx_read
* Author         : Liulei
* Date           : 2011.08.03
* Description    : CAN读取函数，从内部接收缓冲队列中返回一帧数据给用户
* Calls          : memcpy
* Table Accessed : None
* Table Updated  : None
* Input          : cid: CAN控制器序号，STM32F107系列可以取0,1；
*                  buf: 用户缓冲区指针，读取成功后用以保存返回的有效数据帧，
*                       用户需传入CanRxMsg类型变量的地址；
*                  count: 用户缓冲区长度，值为sizeof(CanRxMsg)；
* Output         : None
* Return         : OPFAULT: 参数错误；
*                  0: 接收缓冲队列为空，未读到数据；
*                  >0: 读取的有效字节数
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t canx_read(uint32_t cid, uint8_t buf[], uint32_t count)
{
    int32_t ret = OPFAULT;
    if((cb[cid].opened == true) && 
       (count >= sizeof(CanRxMsg)) && 
       (buf != NULL))
    {
        /* 检查接收队列是否为空，空则返回0，表示未读到数据 */
        if(cb[cid].headp != cb[cid].endp)
        {
            /* 从接收缓冲队列中复制一帧到用户缓冲区buf */
            memcpy(buf, &(cb[cid].rxmsg[cb[cid].headp]), sizeof(CanRxMsg));
            /* 读取一个帧后，接收缓冲队列头指针自增 */
            cb[cid].headp = (cb[cid].headp + 1U) % MAX_CAN_BUF;
            ret = (int32_t)(sizeof(CanRxMsg));
        }
        else
        {
            ret = 0;
        }
    }
    return ret;
}

/*******************************************************************************
* Function       : canx_write
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN发送函数，将用户缓冲区buf中的数据通过指定的CAN口发送
* Calls          : CAN_Transmit, CAN_TransmitStatus
* Table Accessed : None
* Table Updated  : None
* Input          : cid: CAN控制器序号，STM32F107系列可以取0,1；
*                  buf: 待发送的用户缓冲区数据指针，用户需传入CanTxMsg类型变量的地址；
*                  count: 用户缓冲区长度，值为sizeof(CanTxMsg)；
* Output         : None
* Return         : DEVBUSY: 没有可用的发送邮箱
*                  OPFAULT: 参数不合法
*                  >0: 发送的字节数
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t canx_write(uint32_t cid, const uint8_t buf[], uint32_t count)
{
    int32_t ret = OPFAULT;
    if((cb[cid].opened == true) &&
       (buf != NULL) &&
       (count >= sizeof(CanTxMsg)))
    {
        uint8_t mbox = CAN_Transmit(can_list[cid], (CanTxMsg *)buf);
        if(mbox != CAN_NO_MB)
        {
            /* 超时判断 */
            uint32_t timeout = 10000U;
            while(timeout > 0U)
            {
                if(CAN_TransmitStatus(can_list[cid], mbox) == CANTXOK)
                {
                    break;
                }
                timeout--;
            }

            if(timeout != 0U)
            {
                ret = (int32_t)(sizeof(CanTxMsg));
            }
        }
    }
    return ret;
}

/*******************************************************************************
* Function       : canx_ioctl
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN配置函数，可以配置波特率，过滤器配置，重新初始化CAN端
*                  口，获取错误状态。
* Calls          :
* Table Accessed : None
* Table Updated  : None
* Input          : cid: CAN控制器序号，STM32F107系列可以取0,1；
*                  op: 第3个字节为操作码,第2个字节保留,第1~0个字节为参数(如果需要)
*                  arg: 与op的第三个字节操作码相对应的结构
* Output         : None
* Return         : OPFAULT：cid不合法；设备未打开；波特率不合法；或者arg不合法。
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2011.08.03 : 1.0.1 : Linfei
* Modification   : 添加出错处理，问题：本模块提供了重新初始化CAN端口的功能，请慎
*                  用，若修改了位时间，会影响波特率设置的准确性
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t canx_ioctl(uint32_t cid, uint32_t op, void *arg)
{
    int32_t ret = OPFAULT;
    if((cid < MAX_CAN_NUM) && (cb[cid].opened == true))
    {
        uint32_t cmd    = op & 0xff000000U;
        uint32_t param  = op & 0x0000ffffU;

        CAN_ErrorStateTypeDef *es;
        switch(cmd)
        {
        /* 重新初始化CAN端口 */
        case CAN_SET_PORT:
            if(arg != NULL)
            {
                CAN_InitStructure[cid] = *(CAN_InitTypeDef *)arg;        
                if(CAN_Init(can_list[cid], &CAN_InitStructure[cid]) == CANINITOK)
                {
                    ret = DSUCCESS;
                }
            }
            break;

        /* 过滤器配置 */
        case CAN_SET_FILTER:
            if(arg != NULL)
            {
                CAN_FilterInit((CAN_FilterInitTypeDef *)arg);
                ret = DSUCCESS;
            }
            break;

        /* 获得错误状态 */
        case CAN_GET_ERR:
            if(arg != NULL)
            {
                es         = (CAN_ErrorStateTypeDef *)arg;
                es->ewgf   = CAN_GetFlagStatus(can_list[cid], CAN_FLAG_EWG);
                es->epvf   = CAN_GetFlagStatus(can_list[cid], CAN_FLAG_EPV);
                es->boff   = CAN_GetFlagStatus(can_list[cid], CAN_FLAG_BOF);
                es->rec    = (CAN1->ESR >> 24) & 0x000000ffU;
                es->tec    = (CAN1->ESR >> 16) & 0x000000ffU;
                es->lec    = (CAN1->ESR >> 4) & 0x000000007U;
                es->buserr = es->ewgf | es->epvf | es->boff;
                ret = DSUCCESS;
            }
            break;

        /* 设置波特率 */
        case CAN_SET_BAUD:
            if(IS_CAN_BAUD(param) == true)
            {
                CAN_InitStructure[cid].CAN_Prescaler = (uint16_t)param;
                if(CAN_Init(can_list[cid], &CAN_InitStructure[cid]) == CANINITOK)
                {
                    ret = DSUCCESS;
                }
            }
            break;
        default:
            break;
        }
    }
    return ret;
}

/*******************************************************************************
* Function       : CAN_RXx_IRQHandler
* Author         : Liulei
*                  Linfei
* Date           : 2011.08.03
* Description    : CAN接收中断处理函数，从接收邮箱读取数据到模块的接收缓冲队列
* Calls          : CAN_MessagePending, CAN_Receive
* Table Accessed : None
* Table Updated  : None
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2011.08.03 : 1.0.1 : Linfei
* Modification   : 去除FIFO1处理，模块未使用。
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
void CAN_RXx_IRQHandler(uint32_t cid)
{
    uint8_t msg_num;
    
    #if OS_CRITICAL_METHOD == 3
    OS_CPU_SR cpu_sr;
    #endif

    /* 获取当前FIFO0中接收到的有效报文个数 */
    msg_num = CAN_MessagePending(can_list[cid], CAN_FIFO0);

    /* 读取数据存入缓冲队列中尾指针所指向的位置，检查接收队列cb[cid].rxmsg是否已满，
     * 若未满，缓冲区尾指针自增，若已满则返回，该处理方式在缓冲队列满时会导致最后收
     * 到的数据被覆盖，并且由于一次接收中断只读取一个数据帧，可能导致数据丢失 */
    if(msg_num > 0U)
    {
        CAN_Receive(can_list[cid], CAN_FIFO0, &(cb[cid].rxmsg[cb[cid].endp]));
        if (((cb[cid].endp + 1U) % MAX_CAN_BUF) != cb[cid].headp)
        {
            cb[cid].endp = (cb[cid].endp + 1U) % MAX_CAN_BUF;
        }
#if SYS_OS_SUPPORT
        if (NULL!=m_canmsg_queue[cid]){
            OS_ENTER_CRITICAL();
            OSIntNesting++;
            OSMboxPost(m_canmsg_queue[cid],(void*)1);
            OS_EXIT_CRITICAL();
            OSIntExit();
        }
#endif
    }
}
#if SYS_OS_SUPPORT
int32_t canx_poll(int32_t cid,void *oe)
{
    m_canmsg_queue[cid]=oe;
    return DSUCCESS;
}

int32_t can1_poll(void *ev)
{
    return canx_poll(0,(upoll_event*)ev);
}

int32_t can2_poll(void *ev)
{
    return canx_poll(1,(upoll_event*)ev);
}
#endif

/*=====================================CAN1驱动=====================================*/


/*******************************************************************************
* Function       : can1_open
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN1打开函数
* Calls          : canx_open
* Table Accessed : None
* Table Updated  : None
* Input          : flag: 未使用
*                  mode: CAN控制器中断优先级，低16位有效，高八位为抢占优先级，低
*                        八位为子优先级。
* Output         : None
* Return         : canx_open操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can1_open(int32_t flag, int32_t prio)
{
    return canx_open(0U, flag, prio);
}


/*******************************************************************************
* Function       : can1_release
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN1关闭函数
* Calls          : canx_release
* Table Accessed : None
* Table Updated  : None
* Input          : None
* Output         : None
* Return         : canx_release操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can1_release(void)
{
    return canx_release(0U);
}

/*******************************************************************************
* Function       : can1_read
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN1读取函数
* Calls          : canx_read
* Table Accessed : None
* Table Updated  : None
* Input          : buf: 用户缓冲区指针，读取成功后用以保存返回的有效数据帧，
*                       用户需传入CanRxMsg类型变量的地址；
*                  count: 用户缓冲区长度，值为sizeof(CanRxMsg)；
* Output         : None
* Return         : canx_read操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can1_read(uint8_t buf[], uint32_t count)
{
    return canx_read(0U, buf, count);
}

/*******************************************************************************
* Function       : can1_write
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN1发送函数
* Calls          : canx_write
* Table Accessed : None
* Table Updated  : None
* Input          : buf: 待发送的用户缓冲区数据指针，用户需传入CanTxMsg类型变量的地址；
*                  count: 用户缓冲区长度，值为sizeof(CanTxMsg)；
* Output         : None
* Return         : canx_write操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can1_write(const uint8_t buf[], uint32_t count)
{
    return canx_write(0U, buf, count);
}


/*******************************************************************************
* Function       : can1_ioctl
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN1口配置函数，可以配置波特率，重新初始化过滤器，重新初始化
*                  CAN1端口，获取错误状态。
* Calls          :
* Table Accessed : None
* Table Updated  : None
* Input          : op: 第3个字节为操作码,第2个字节保留,第1~0个字节为参数(如果需要)
*                  arg: 与op的第三个字节操作码相对应的结构
* Output         : None
* Return         : canx_ioctl操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2011.08.03 : 1.0.1 : Linfei
* Modification   :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can1_ioctl(uint32_t op, void *arg)
{
    return canx_ioctl(0U, op, arg);
}

/*******************************************************************************
* Function       : CAN1_RX0_IRQHandler
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN1接收中断处理函数
* Calls          : CAN_RXx_IRQHandler
* Table Accessed : None
* Table Updated  : None
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2012.07.28 : 1.0.2 : Linfei
* Modification   : 增加对STM32F4xx芯片支持
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
#if defined(STM32F4XX) || defined(STM32F10X_CL)
void CAN1_RX0_IRQHandler (void)
{
#else
void USB_LP_CAN_RX0_IRQHandler(void)
{
#endif
    CAN_RXx_IRQHandler(0U);
}

/*=====================================can2驱动=====================================*/


/*******************************************************************************
* Function       : can2_open
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN2打开函数
* Calls          : canx_open
* Table Accessed : None
* Table Updated  : None
* Input          : flag: 未使用
*                  mode: CAN控制器中断优先级，低16位有效，高八位为抢占优先级，低
*                        八位为子优先级。
* Output         : None
* Return         : canx_open操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can2_open(int32_t flag, int32_t prio)
{
    return canx_open(1U, flag, prio);
}

/*******************************************************************************
* Function       : can2_release
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN2关闭函数
* Calls          : canx_release
* Table Accessed : None
* Table Updated  : None
* Input          : None
* Output         : None
* Return         : canx_release操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can2_release(void)
{
    return canx_release(1U);
}


/*******************************************************************************
* Function       : can2_read
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN2读取函数
* Calls          : canx_read
* Table Accessed : None
* Table Updated  : None
* Input          : buf: 用户缓冲区指针，读取成功后用以保存返回的有效数据帧，
*                       用户需传入CanRxMsg类型变量的地址；
*                  count: 用户缓冲区长度，值为sizeof(CanRxMsg)；
* Output         : None
* Return         : canx_read操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can2_read(uint8_t buf[], uint32_t count)
{
    return canx_read(1U, buf, count);
}

/*******************************************************************************
* Function       : can2_write
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN2发送函数
* Calls          : canx_write
* Table Accessed : None
* Table Updated  : None
* Input          : buf: 待发送的用户缓冲区数据指针，用户需传入CanTxMsg类型变量的地址；
*                  count: 用户缓冲区长度，值为sizeof(CanTxMsg)；
* Output         : None
* Return         : canx_write操作结果
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can2_write(const uint8_t buf[], uint32_t count)
{
    return canx_write(1U, buf, count);
}


/*******************************************************************************
* Function       : can2_ioctl
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN2口配置函数，可以配置波特率，重新初始化过滤器，重新初始化
*                  CAN2端口，获取错误状态。
* Calls          :
* Table Accessed : None
* Table Updated  : None
* Input          : op: 第3个字节为操作码,第2个字节保留,第1~0个字节为参数(如果需要)
*                  arg: 与op的第三个字节操作码相对应的结构
* Output         : None
* Return         : canx_ioctl操作结果
*******************************************************************************
* History        : 
*------------------------------------------------------------------------------
* 2011.08.03 : 1.0.1 : Linfei
* Modification   :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
int32_t can2_ioctl(uint32_t op, void *arg)
{
    return canx_ioctl(1U, op, arg);
}

/*******************************************************************************
* Function       : CAN2_RX0_IRQHandler
* Author         : Liulei
* Date           : 2010.08.03
* Description    : CAN2接收中断处理函数
* Calls          : CAN_RXx_IRQHandler
* Table Accessed : None
* Table Updated  : None
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************
* History        :
*------------------------------------------------------------------------------
* 2010.08.03 : 1.0.0 : liulei
* Modification   : 初始代码编写。
*------------------------------------------------------------------------------
*******************************************************************************/
void CAN2_RX0_IRQHandler(void)
{
    CAN_RXx_IRQHandler(1U);
}
