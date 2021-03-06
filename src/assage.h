/****************************************************************************** 
* Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :assage.h
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
#ifndef ASSAGE_H_
#define ASSAGE_H_

#include "global.h"

#define ASSAGE_MAX_TASK 6u
#define ASSAGE_MAX_SIG  20

//message type
#define ASSAGE_TYPE_TIMER10  (1 << 0)
#define ASSAGE_TYPE_TIMER200 (1 << 1)
#define ASSAGE_TYPE_TIMER500 (1 << 2)
#define ASSAGE_TYPE_TIMER1S  (1 << 3)
#define ASSAGE_TYPE_TIMER15M (1 << 4)
#define ASSAGE_TYPE_RS232    (1 << 5)
#define ASSAGE_TYPE_USB      (1 << 6)
#define ASSAGE_TYPE_ETHERNET (1 << 7)
#define ASSAGE_TYPE_BUTTON   (1 << 8)
#define ASSAGE_TYPE_CAN      (1 << 9)
#define ASSAGE_TYPE_IO       (1 << 10)

typedef union {
    int8_t   c_var;
    uint8_t  uc_var;
    int16_t  s_var;
    uint16_t w_var;
    int32_t  i_var;
    uint32_t dw_var;
    int64_t  ll_var;
    uint64_t ull_var;
    void    *p_var;
}t_assage_variant;

typedef struct {
    uint32_t msg;
    t_assage_variant param1;
    t_assage_variant param2;
}t_assage_msg;

typedef uint32_t assage_handle;

void          assage_init(void);
assage_handle assage_create_msg(t_assage_msg msg_buf[],uint16_t size,uint32_t pmask);//for MISRA-2004 name 'mask' already used in other name space
void          assage_post_msg  (assage_handle handle,t_assage_msg *pmsg);
void          assage_post_all  (t_assage_msg *pmsg);//for MISRA-2004 name 'msg' already used in other name space
#define ASSAGE_GET_NODELAY 0xFF
void          assage_get_msg   (assage_handle handle,uint16_t timeout,t_assage_msg *lmsg);
void          assage_del_msg   (assage_handle handle);



#endif
