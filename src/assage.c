/****************************************************************************** 
* Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :assage.c
* Author         :llemmx
* Date           :2012-11-02
* Description    :异步消息模块，用于处理驱动消息和进程间交互消息
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2012-11-2 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include "assage.h"
#include "fcntl.h"

typedef struct {
    uint32_t mask;
    OS_EVENT *task;
}t_assage_buf;

t_assage_buf m_assage_buf[ASSAGE_MAX_TASK+1u]; //用来统一管理系统中建立的消息队列，队列最大个数为ASSAGE_MAX_TASK

OS_EVENT *m_events[ASSAGE_MAX_SIG+1u];//the array length must more than max drivers
/******************************************************************************
* Function       :assage_init
* Author         :llemmx	
* Date           :2012-11-2
* Description    :异步消息初始化，将异步消息数组m_assage_buf[]初始化，异步消息使用
*				 :的总数不能超过异步消息所能管理的最大范围，由ASSAGE_MAX_TASK宏定义。
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void assage_init(void)
{
    for (uint8_t i=0U;i<=ASSAGE_MAX_TASK;++i){
        m_assage_buf[i].mask=0U;
        m_assage_buf[i].task=NULL;
    }
}
/******************************************************************************
* Function       :assage_create_msg
* Author         :llemmx	
* Date           :2012-11-2
* Description    :创建异步消息缓冲区,从m_assage_buf[]中找出空闲项，用来管理新建的消息缓冲区。
* Calls          :OSQCreateEx()
* Table Accessed :无
* Table Updated  :无
* Input          :msg_buf[]:本地使用的消息缓冲，由用户申请，系统管理
*				 :size:消息缓冲的个数，即消息缓冲区同时能够存储的最大消息数量
*				 :mask:消息掩码，用来定义消息类型，此掩码是用来处理定向消息时使用，
*				 :如不需处理定向消息，可设置为0xFFFFFFFF,将接收和发送所有类型的消息。
*                :只针对assage_post_all()函数
* Output         :无
* Return         :创建成功将返回非0数值，为异步消息的操作句柄。
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
assage_handle assage_create_msg(t_assage_msg msg_buf[],uint16_t size,uint32_t pmask)
{
    uint8_t i;
    assage_handle idx=0U;
    if((pmask != 0u)&&(size != 0u))	           //2012.11.14	wujing add for parameter check.
	{
	    for (i=1U;i<=ASSAGE_MAX_TASK;++i){ //从消息队列管理数组中找到空闲项
	        if (0U==m_assage_buf[i].mask){
	            idx=(assage_handle)i;
	            break;
	        }
	    }
	    if (idx>0U){
	        m_assage_buf[idx].mask=pmask;  //创建消息队列
	        m_assage_buf[idx].task=(OS_EVENT*)OSQCreateEx((void*)msg_buf,size,sizeof(t_assage_msg));
	        if (NULL==m_assage_buf[idx].task){
	            idx=0U;
	        }
	    }else{
	        idx=0U;
	    }
    }
    return idx;
}
/******************************************************************************
* Function       :assage_post_msg
* Author         :llemmx	
* Date           :2012-11-2
* Description    :向指定的队列中插入一条消息
* Calls          :OSQPostEx()
* Table Accessed :无
* Table Updated  :无
* Input          :handle:消息句柄，用以指定消息缓冲队列。
*				 :msg:具体要发送的消息。
* Output         :无
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void assage_post_msg(assage_handle handle,t_assage_msg *pmsg)
{
    if (((NULL!=pmsg) && (handle>0U)) && (handle<=ASSAGE_MAX_TASK)){
        (void)OSQPostEx(m_assage_buf[handle].task,pmsg);
    }else{
    }
}
/******************************************************************************
* Function       :assage_post_all
* Author         :llemmx	
* Date           :2012-11-2
* Description    :发送广播消息，但此消息会受消息掩码控制，只有允许接受此消息
*				 :的队列才能得到此消息
* Calls          :OSQPostEx()
* Table Accessed :无
* Table Updated  :无
* Input          :msg:具体要发送的消息。
* Output         :无
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void assage_post_all(t_assage_msg *pmsg)
{
    uint8_t i;
    if (NULL!=pmsg){
        for (i=1U;i<=ASSAGE_MAX_TASK;++i){
            if ((m_assage_buf[i].mask & pmsg->msg)!= 0U){
                (void)OSQPostEx(m_assage_buf[i].task,pmsg);
            }
        }
    }else{
    }
}
/******************************************************************************
* Function       :assage_get_msg
* Author         :llemmx	
* Date           :2012-11-2
* Description    :从队列中获取一条消息
* Calls          :OSQPostEx()
* Table Accessed :无
* Table Updated  :无
* Input          :handle--消息句柄，用以指定消息缓冲队列。
*				 :timeout--0:阻塞等待消息；255:不阻塞立刻返回；
*				 :		   其他数值：等待相应时间(单位：时钟节拍)后超时返回。
* Output         :lmsg --本地消息缓冲的指针, 其中的msg成员为0表示未获得任何消息
*				 :       msg成员不为0表示得到了相应的消息。
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void assage_get_msg(assage_handle handle,uint16_t timeout,t_assage_msg *lmsg)
{
    INT8U err;
    
    if ((0U!=handle) && (handle<=ASSAGE_MAX_TASK) && (lmsg!=NULL)){
		lmsg->msg=0U;
		if (timeout!=0xFFU){
			OSQPendEx(m_assage_buf[handle].task,lmsg,timeout,&err);
		}else{
			OSQAcceptEx(m_assage_buf[handle].task,lmsg,&err);
		}
    }
}
/******************************************************************************
* Function       :assage_del_msg
* Author         :llemmx	
* Date           :2012-11-2
* Description    :删除指定的消息缓冲队列
* Calls          :OSQDel()
* Table Accessed :无
* Table Updated  :无
* Input          :handle--消息句柄，用以指定消息缓冲队列。
* Output         :无
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void assage_del_msg(assage_handle handle)
{
    INT8U err;
    if ((0U!=handle) && (handle<=ASSAGE_MAX_TASK)){//wujing 2012.11.14 logic err: use '&&' instead of '||'.
        OSQDel(m_assage_buf[handle].task,OS_DEL_ALWAYS,&err);
    }else{
    }
}

#if 0 //assage模块不用
/******************************************************************************
* Function       :assage_create_sig
* Author         :llemmx	
* Date           :2012-11-2
* Description    :根据驱动句柄创建异步消息，此接口为驱动专用，不能用于其他功能
* Calls          :OSMboxCreate(),drv_poll()
* Table Accessed :无
* Table Updated  :无
* Input          :fd-驱动文件句柄
* Output         :无
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void assage_create_sig(int32_t fd)
{
    if (fd<ASSAGE_MAX_SIG){
        m_events[fd]=OSMboxCreate((void*)0);
        if (NULL!=m_events[fd]){
            (void)drv_poll(fd,m_events[fd]);
        }
    }
}
/******************************************************************************
* Function       :assage_accept
* Author         :llemmx	
* Date           :2012-11-2
* Description    :无阻塞地获取驱动消息，传递消息的方式为将消息冒充指针发送
* Calls          :OSMboxAccept()
* Table Accessed :无
* Table Updated  :无
* Input          :fd-驱动文件句柄
* Output         :无
* Return         :获取的消息类型
*				 :0值   - 相应驱动文件句柄无消息	
*				 :非0值 - 相应驱动文件句柄有消息，不同驱动相应值不同。
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint32_t assage_accept(int32_t fd)
{
    uint32_t l_ret=(uint32_t)OSMboxAccept(m_events[fd]);
    return l_ret;
}
/******************************************************************************
* Function       :assage_delete_sig
* Author         :llemmx	
* Date           :2012-11-2
* Description    :删除指定驱动文件句柄对应的邮箱，参数OS_DEL_ALWAYS表示
*				 :删除邮箱后，所有等待该邮箱消息的任务都会被置为就绪。
* Calls          :OSMboxDel()
* Table Accessed :无
* Table Updated  :无
* Input          :fd-驱动文件句柄
* Output         :无
* Return         :无
*******************************************************************************
* History:        2012-11-02 
*------------------------------------------------------------------------------
* 2012-11-02 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void assage_delete_sig(int32_t fd)
{
    INT8U err;
    m_events[fd]=OSMboxDel(m_events[fd],OS_DEL_ALWAYS,&err);
}
#endif
