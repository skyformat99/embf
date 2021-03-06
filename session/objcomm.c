//exobj V1.0.0
#include <string.h>

#include "objcomm.h"
#include "base64.h"
/*
    exh =extern head
    objc=object command
*/
#define PKG_BUF_SIZE PKG_DATA_BUF_SIZE/3*4+1
objc_sys_data g_osd;
uint8_t m_tmp_buf[PKG_BUF_SIZE];
uint8_t m_tmp_sbuf[PKG_BUF_SIZE];

int32_t exh_check_pkg(uint8_t *buf,int32_t len,int32_t *start)
{
    int32_t head=-1,end=-2;
    uint8_t *t_buf;
    for (t_buf=buf;t_buf<(buf+len);++t_buf){
        if (t_buf[0]==0x0A){
            head=(int32_t)(t_buf-buf);
            *start=head+1;
        }
        if (head>=0 && t_buf[0]==0x0D){
            end=(int32_t)(t_buf-buf);
            break;
        }
    }
    
    return end-head+1;
}

uint16_t objc_get_crc16(volatile uint8_t *ptr,uint8_t len) 
{ 
    uint8_t i; 
    uint16_t crc = 0xFFFF; 
    if(len==0) {
        len = 1;
    } 
    while(len--){
        crc ^= *ptr; 
        for(i=0; i<8; i++){ 
            if(crc&1){ 
                crc >>= 1;  
                crc ^= 0xA001; 
            }else{
                crc >>= 1;
            } 
        }         
        ptr++; 
    } 
    return(crc); 
}

int objc_analyst(uint8_t *in_buf,int len,uint8_t *out_buf,int slen,ExCallBack ex_callback)
{
    //pkg head progress
    int start;//=sizeof(TObjc_Head);
    //int send_index=slen-5;
    int data_len=exh_check_pkg(in_buf,len,&start);
    int l_ret;
    
    if (data_len<=0 || NULL==ex_callback){
        return -1;
    }
    //base64
    data_len=base64_decode(in_buf+start,data_len-2,m_tmp_buf,64);
    if (data_len<0){
        return -1;
    }
    
    //CRC16
    uint16_t l_crc,l_crcp=0;
    memcpy(&l_crcp,m_tmp_buf+data_len-2,2);
    l_crc=objc_get_crc16(m_tmp_buf,data_len-2);
    if (l_crc!=l_crcp){
        return -1;//data error
    }
    
    //fault infomatin package
    TObjc_Head *pkg_head;
    pkg_head=(TObjc_Head *)m_tmp_buf;
    if (pkg_head->err){
        if (pkg_head->dir){
            //处理回复的错误包
            
        }
        return -1;
    }
    //package addr check
    if (pkg_head->addr!=g_osd.addr){
        return -1;
    }
    if (pkg_head->cyp){
        //Decryption
        if ((data_len%16)!=0){
            //return error
        }
    }
    
    //analyst
    TObjc_obj_opera *l_oo;
    TExCallParam l_excp;
    uint8_t l_headlen;
    l_oo=(TObjc_obj_opera *)(m_tmp_buf+sizeof(TObjc_Head));
    l_headlen=sizeof(TObjc_Head)+sizeof(TObjc_obj_opera);
    
    l_excp.obj   =l_oo->obj;
    l_excp.opera =l_oo->opera;
    l_excp.param1=m_tmp_buf+l_headlen;
    l_excp.len   =data_len-l_headlen-2;
    l_excp.sbuf  =m_tmp_sbuf+l_headlen;
    l_excp.slen  =PKG_BUF_SIZE-l_headlen-2;
    
    uint8_t l_err=0;
    l_ret=ex_callback(&l_excp,&l_err);

    if (l_ret==0){
        return 0;
    }
    //package data
    l_ret+=l_headlen;
    memcpy(m_tmp_sbuf,m_tmp_buf,l_headlen);
    pkg_head=(TObjc_Head *)m_tmp_sbuf;
    pkg_head->dir=1;
    //crc16
    l_crc=objc_get_crc16(m_tmp_sbuf,l_ret);
    memcpy(m_tmp_sbuf+l_ret,&l_crc,2);
    l_ret+=2;
    //base64
    data_len=base64_encode(m_tmp_sbuf,l_ret,out_buf+1,slen);
    out_buf[0]=0x0A;
    out_buf[data_len+1]=0x0D;
    data_len+=2;
    return data_len;
}
