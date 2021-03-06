/****************************************************************************** 
* Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :upoll.c
* Author         :llemmx
* Date           :2012-09-10
* Description    :异步事件模型。
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2012-09-10     : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include "upoll.h"
#include "drivers.h"
#include "fcntl.h"

upoll_event  m_poll_queue[UPOLL_MAX_NUM];
upoll_event *m_fd_queue[UPOLL_MAX_NUM];
//static uint8_t m_size;
OS_EVENT *m_QMsg;

/******************************************************************************
* Function       :upoll_create
* Author         :llemmx
* Date           :2012-09-10
* Description    :创建消息队列
* Calls          :OSQCreate 
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :UPOLL_OK 创建消息队列成功
*                 UPOLL_ERR 创建消息队列失败
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2012-09-10     : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t upoll_create(int8_t size)
{
    //m_size=size;
    int32_t ret;
    for (int8_t idx=0;idx<UPOLL_MAX_NUM;++idx){
        m_poll_queue[idx].events  =0;
        m_poll_queue[idx].data.ui64=0U;
    }
    m_QMsg=OSQCreate((void*)&m_fd_queue,(INT16U)UPOLL_MAX_NUM-1U);
    if ((OS_EVENT *)0==m_QMsg){
        ret=UPOLL_ERR;
    }else{
        ret=UPOLL_OK;
    }
    return ret;
}

/******************************************************************************
* Function       :upoll_ctl
* Author         :llemmx
* Date           :2012-09-10
* Description    :创建消息队列
* Calls          :drv_poll, 
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :UPOLL_OK 文件句柄添加成功
*                 UPOLL_ERR 文件句柄添加失败
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2012-09-10     : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t upoll_ctl(int32_t epfd, int32_t op, int32_t fd,upoll_event *event)
{
    int8_t idx=0;
    int32_t ret=UPOLL_ERR,p_ret;
    switch(op){
    case UPOLL_CTL_ADD:
        idx=fd % UPOLL_MAX_NUM;
        if ((m_poll_queue[idx].events==0) && (NULL!=event)){
            if (event->events!=0){
                memcpy(&m_poll_queue[idx],event,sizeof(upoll_event));
                m_poll_queue[idx].ev=m_QMsg;
                m_poll_queue[idx].dev_flag=100;
                m_poll_queue[idx].data.filed=fd;
                if (DSUCCESS==drv_poll(fd,&m_poll_queue[idx])){
                    ret=UPOLL_OK;
                }else{
                    m_poll_queue[idx].events=0;
                    ret=UPOLL_ERR;
                }
            }else{
                ret=UPOLL_ERR;
            }
        }else{
            ret=UPOLL_ERR;
        }
        break;
    case UPOLL_CTL_DEL:
        idx=fd % UPOLL_MAX_NUM;
        p_ret=drv_poll(fd,NULL);
        if (p_ret!=DSUCCESS){}
        m_poll_queue[idx].events=0;
        ret=UPOLL_OK;
        break;
    case UPOLL_CTL_MOD:
        idx=fd % UPOLL_MAX_NUM;
        if (NULL!=event){
            if (event->events!=0){
                m_poll_queue[idx].ev=m_QMsg;
                m_poll_queue[idx].dev_flag=0;
                m_poll_queue[idx].data.filed=fd;
                memcpy(&m_poll_queue[idx],event,sizeof(upoll_event));
                if (DSUCCESS==drv_poll(fd,&m_poll_queue[idx])){
                    ret=UPOLL_OK;
                }else{
                    ret=UPOLL_ERR;
                }
            }else{
                ret=UPOLL_ERR;
            }
        }else{
            ret=UPOLL_ERR;
        }
        break;
    default:
        ret=UPOLL_ERR;
        break;
    }
    return ret;
}

/******************************************************************************
* Function       :upoll_wait
* Author         :llemmx
* Date           :2012-09-10
* Description    :创建消息队列
* Calls          :OSQPend, memcpy
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :>1 成功捕获到文件事件
*                 UPOLL_ERR 获取事件时产生错误
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2012-09-10     : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t upoll_wait(int32_t epfd,upoll_event *event,int32_t maxevents,int16_t timeout)
{
    INT8U err;
    int8_t idx=0;
    int32_t ret=UPOLL_ERR;
    upoll_event *ev=OSQPend(m_QMsg,(INT16U)timeout,&err);
    
    if ((OS_ERR_NONE!=err) || (NULL==event) || (NULL==ev)){
        ret=UPOLL_ERR;
    }else{
        idx=ev->data.filed % UPOLL_MAX_NUM;
        memcpy(event,&m_poll_queue[idx],sizeof(upoll_event));
        ret=ev->dev_flag;
    }
    return ret;
}
