/****************************************************************************** 
* Copyright (C), 1997-2010, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :message.c
* Author         :llemmx
* Date           :2011-05-03
* Description    :消息调度模块，主要实现消息调度层，支持多级消息传递。
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* 2012-01-23 : 2.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include <string.h>
#include "message.h"
#include "global.h"
#include "utils.h"
static void l_memset(void *buf,uint8_t value,int32_t len);

/******************************************************************************
* Function       :EnableMsgType
* Author         :llemmx
* Date           :2011-05-03
* Description    :注册消息类型
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :MsgType - 消息类型
                  Enabled - 消息启用或不启用，TRUE表示启用，FALSE表示关闭
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void EnableMsgType(PProcBuf Entry,uint32_t MsgType,int32_t Enabled){
    if (1 == Enabled){
        Entry->EnabledMsg|=MsgType;
    }else{
        Entry->EnabledMsg=Entry->EnabledMsg & ~MsgType;
    }
}

/******************************************************************************
* Function       :SelectNewMsgMask
* Author         :llemmx
* Date           :2011-05-03
* Description    :设置消息掩码
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :newmsk - 具体掩码值
* Output         :无
* Return         :返回旧的掩码
*******************************************************************************
* History:        2011-05-03
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint32_t SelectNewMsgMask(PProcBuf Entry,uint32_t newmsk){
    uint32_t oldmsk       =Entry->EnabledMsg;
    Entry->EnabledMsg=newmsk;
    return oldmsk;
}

/******************************************************************************
* Function       :GetMsg
* Author         :llemmx
* Date           :2011-05-03
* Description    :读取有效事件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :msg - 消息
* Output         :无
* Return         :返回消息处理的结果，如果非0则会继续处理相同消息。
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
/*int GetMsg(PMsg msg){
    int m=1;
    
    while ((m=(*pgm)(msg))==0);
    return m;
}*/

static void l_memset(void *buf,uint8_t value,int32_t len)
{
    int32_t i;
    uint8_t *l_data=(uint8_t*)buf;
    for (i=0;i<len;++i){
        l_data[i]=value;
    }
};
/******************************************************************************
* Function       :InitMsg
* Author         :llemmx
* Date           :2011-05-03
* Description    :初始化消息队列，并初始化一个捕获消息函数，例如find_msg()函数
* Calls          :l_memset()
* Table Accessed :无
* Table Updated  :无
* Input          :Entry:消息队列指针
*				 :GetMsg:初始化的消息捕获函数
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void InitMsg(PProcBuf Entry,PMsgProc GetMsg)
{
    l_memset(Entry->MessageProcs,0U,MSG_PROC_SIZE*sizeof(PMsgProc));
    l_memset(Entry->MsgQueue,0U,MSG_SIZE*sizeof(TMsg));
    Entry->mHead=0U;
    Entry->mEnd=0U;
    Entry->MaxPorc=0U;
    Entry->pgm=GetMsg;
    Entry->EnabledMsg=0U;
}

/******************************************************************************
* Function       :DoMsgProcess
* Author         :llemmx
* Date           :2011-05-03
* Description    :消息处理进程，主要实现消息的阻塞运行，程序会阻塞在这里不停的查找与处理
                  消息
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :Obj - 消息处理的回调函数
* Output         :ExitCommand - 满足什么条件就会推出这个消息过程
* Return         :返回消息参数2
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t DoMsgProcess(PProcBuf Entry,void *Obj,int32_t ExitCommand){
    TMsg msg;
    uint32_t index;
    do{
        if (Entry->mEnd!=Entry->mHead){//msg queue is not null
            memcpy(&msg,&Entry->MsgQueue[Entry->mEnd],sizeof(TMsg));
            Entry->mEnd=(Entry->mEnd+1U) % MSG_SIZE;
            msg.Object=Obj;
            index=Entry->MaxPorc;
            while((index>0U) && (msg.Message!=MSG_TYPE_CMD)){
                if (0 == Entry->MessageProcs[index-1U](&msg)){
                    index--;
                }else{
                    break;
                }
            }
        }else{
            Entry->pgm(0);
        }
        embf_delay(10);
    }while(!((msg.Message==MSG_TYPE_CMD) && (msg.Param1==ExitCommand)));
    return msg.Param2;
}

/******************************************************************************
* Function       :RegMsgProc
* Author         :llemmx
* Date           :2011-05-03
* Description    :注册消息队列，注册一个消息队列到队列中。
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :MsgProc - 回调函数的函数指针
* Output         :无
* Return         :返回-1表示注册失败，否则返回注册消息总数
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t RegMsgProc(PProcBuf Entry,PMsgProc MsgProc)
{
	int32_t ret = 0;
	int32_t i = 0;
    for (i=0;i<MSG_PROC_SIZE;++i){
        if (Entry->MessageProcs[i]==0){
            Entry->MaxPorc++;
            Entry->MessageProcs[i]=MsgProc;
            break;
        }
    }
    if(MSG_PROC_SIZE == i)//队列已满，返回-1
    {
    	ret = -1;
    }
    else
    {
    	ret = i;    //队列未满，返回注册的消息总数,为0表明消息个数为1
    }
    return ret;
}

/******************************************************************************
* Function       :UnRegMsgProc
* Author         :llemmx
* Date           :2011-05-03
* Description    :卸载已经注册的消息队列
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :index - 需要卸载消息的索引
* Output         :无
* Return         :返回目前回调函数的总长
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t UnRegMsgProc(PProcBuf Entry,int32_t index)
{
	int32_t ret = -1;
    if ((index>=0) && (index<MSG_PROC_SIZE)){
        Entry->MessageProcs[index]=0;
        if (Entry->MaxPorc>0U){
            Entry->MaxPorc--;
        }else{
            Entry->MaxPorc=0U;
        }
        ret = 1;
    }
    return ret;
}
/******************************************************************************
* Function       :PostMessage
* Author         :llemmx
* Date           :2011-05-03
* Description    :向指定的消息队列发送消息
* Calls          :l_memset()
* Table Accessed :无
* Table Updated  :无
* Input          :Entry:消息队列指针
*				 :Msg:消息类型，例：按键MSG_TYPE_BUTTON；
*				 :P1:消息参数  例：MKEY_BTN_UP
*				 :p2:消息参数  例：MKEY_ENTER
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t PostMessage(PProcBuf Entry,int32_t Msg,int32_t P1,int32_t P2)
{
	int32_t ret = -1;
    if (((Entry->mHead+1U)%MSG_SIZE)!=Entry->mEnd){//queue was full
    Entry->MsgQueue[Entry->mHead].Param1=P1;
    Entry->MsgQueue[Entry->mHead].Param2=P2;
    Entry->MsgQueue[Entry->mHead].Message=Msg;
    Entry->mHead=(Entry->mHead+1U) % MSG_SIZE;	 //MsgQueue[]数组长度为5，mHead为4时重新初始化为0，MsgQueue[]实际有效元素数为4
    ret = 1;
    }
    return ret;
}
