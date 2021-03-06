/*****************************************************
          modbus-rtu 通讯规约
******************************************************/
#include "headall.h"

/***************************************
函数名称：crc16校验
函数功能：crc16校验
函数输入：字节指针*ptr，数据长度len
函数返回：双字节crc
****************************************/

uint16_t getCRC16(uint8_t *ptr,uint8_t len)
{ 
    uint8_t i; 
    uint16_t crc = 0xFFFF; 
    if(len==0) 
    {
        len = 1;
    } 
    while(len--)  
    {   
        crc ^= *ptr; 
        for(i=0; i<8; i++)  
        { 
            if(crc&1) 
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
    return(crc); 
} 

int Modbus_M_Package(TPackageParam *pp)
{
    uint8_t *l_data;
    uint16_t l_crc;
    uint8_t  l_len;
    uint8_t  l_tmp;
    uint8_t  l_num;
    if (pp==NULL){
        return 0;
    }
    if (MODBUS_RTU==pp->type){
        l_len=2;
        l_data=pp->buf;
    }else if (MODBUS_TCP==pp->type){
        l_len=6;
        l_data=pp->buf+6;
    }else{
        return 0;
    }
    l_num=pp->end_addr-pp->start_addr+1;
    switch(pp->cmd){
    case MODBUS_CMD_03:
    case MODBUS_CMD_04:
        l_len+=6;
        l_data[0]=pp->dev_addruc;
        l_data[1]=pp->cmd;
        l_data[2]=(pp->start_addr >> 8) & 0xFF;
        l_data[3]=pp->start_addr & 0xFF;
        l_data[4]=(l_num >> 8) & 0xFF;
        l_data[5]=l_num & 0xFF;
        break;
    case MODBUS_CMD_06:
        l_len+=6;
        l_data[0]=pp->dev_addruc;
        l_data[1]=pp->cmd;
        l_data[2]=(pp->start_addr >> 8) & 0xFF;
        l_data[3]=pp->start_addr & 0xFF;
        if (pp->mcb==NULL){
            return 0;
        }
        pp->mcb(pp->start_addr,l_data+4);
        break;
    case MODBUS_CMD_16:
        l_len=l_len+(l_num<<1)+7;
        l_data[0]=pp->dev_addruc;
        l_data[1]=pp->cmd;
        l_data[2]=(pp->start_addr >> 8) & 0xFF;
        l_data[3]=pp->start_addr & 0xFF;
        l_data[4]=(l_num >> 8) & 0xFF;
        l_data[5]=l_num & 0xFF;
        l_data[6]=l_num<<1;
        if (pp->mcb==NULL){
            return 0;
        }
        for (l_tmp=0;l_tmp<=l_num;++l_tmp){
            pp->mcb(l_tmp+pp->start_addr,l_data+7+(l_tmp<<1));
        }
        break;
    default:
        return 0;
    }
    if (MODBUS_RTU==pp->type){
        l_crc=getCRC16(l_data,l_len-2);
        l_data[l_len-2]=l_crc & 0xFF;
        l_data[l_len-1]=(l_crc>>8) & 0xFF;
    }else if (MODBUS_TCP==pp->type){
        pp->buf[0]=0x01;
        pp->buf[1]=0x01;
        pp->buf[2]=0x00;
        pp->buf[3]=0x00;
        pp->buf[4]=(l_len>>8) & 0xFF;
        pp->buf[5]=l_len & 0xFF;
    }
    return l_len;
}

void Modbus_M_Analyst(uint8_t type,uint8_t *data,uint8_t len,ModbusMRCallBack msb,ModbusMECallback mec)
{
    uint8_t *l_data;
    uint16_t l_crc;
    uint8_t  l_len;
    if (data==NULL || msb==NULL || len<5){
        return ;
    }
    if (MODBUS_TCP==type){
        l_data=data+6;
    }else{
        l_data=data;
    }
    //current is error package
    if ((l_data[1] & 0x80)>0){
        if (MODBUS_RTU==type){
            l_crc=getCRC16(l_data,3);
            if (l_crc!=(l_data[5]<<8 | l_data[4])){//CRC error
                return ;
            }
        }
        if (mec!=NULL){
            mec(data[0],data[1] & 0xF,data[2]);
        }
        return ;
    }
    if (l_data[2]>len){
        return ;
    }

    if (MODBUS_CMD_03==l_data[1] || MODBUS_CMD_04==l_data[1]){
        l_len=data[2];
        if (MODBUS_RTU==type){
            l_crc=getCRC16(data,l_len+3);
            if (l_crc!=(l_data[l_len+4]<<8 | l_data[l_len+3])){//CRC error
                return ;
            }
        }
        msb(l_data[0],l_data[1],l_data+3,l_data[2]);
    }else if (MODBUS_CMD_06==l_data[1] || MODBUS_CMD_16==l_data[1]){
        if (MODBUS_RTU==type){
            l_crc=getCRC16(l_data,6);
            if (l_crc!=(l_data[7]<<8 | l_data[6])){//CRC error
                return ;
            }
        }
        msb(l_data[0],l_data[1],l_data+2,4);
    }else{
        return ;
    }
}

int Modbus_S_get_address(uint8_t type,uint8_t *data,uint8_t len)
{
    uint8_t *l_data;

    if (data==NULL || len<8){
        return 0;
    }

    if (MODBUS_TCP==type){
        l_data=data+6;
    }else{
        l_data=data;
    }
    if (MODBUS_CMD_03==l_data[1] || MODBUS_CMD_04==l_data[1] || MODBUS_CMD_06==l_data[1] || MODBUS_CMD_16==l_data[1]){
        return l_data[0];
    }
    return 0;
}

int Modbus_get_id(uint8_t type,uint8_t *data,uint8_t len)
{
    uint8_t *l_data;
    if (data==NULL || len<8){
        return -1;
    }

    if (MODBUS_TCP==type){
        l_data=data+6;
    }else{
        l_data=data;
    }
    if (MODBUS_CMD_03==l_data[1] || MODBUS_CMD_04==l_data[1] || MODBUS_CMD_06==l_data[1] || MODBUS_CMD_16==l_data[1]){
        return (l_data[2]<<8) | l_data[3];
    }
    return -1;
}

//return -1 is not modbus ,return 0 is not recv data,>0 have some data
int Modbus_S_Analyst(uint8_t type,uint8_t *data,uint8_t len,uint8_t *sbuf,uint8_t slen,ModbusSRCallBack msb)
{
	uint8_t *l_data;
	uint16_t l_crc;
    uint8_t  l_len,l_ret=0,l_err=0;
	TAnalystParam l_ap;
    TSPackageParam l_spp;
	if (data==NULL || msb==NULL || len<8){
        return -1;
    }
    if (MODBUS_TCP==type){
        l_data=data+6;
        l_ap.tcp_headp=data;
    }else{
        l_data=data;
    }
    l_ap.typeuc=type;
	if (MODBUS_CMD_03==l_data[1] || MODBUS_CMD_04==l_data[1] || MODBUS_CMD_06==l_data[1]){
        if (MODBUS_RTU==type){
            l_crc=getCRC16(l_data,6);
            if (l_crc!=(l_data[7]<<8 | l_data[6])){//CRC error
                return -1;
            }
        }
        //data process
		l_ap.addr=l_data[0];
        l_ap.cmd =l_data[1];
        l_ap.data=l_data+2;
        l_ap.len =4;
        if (MODBUS_TCP==type){
            l_ap.sbuf=sbuf+9;
            l_ap.slen=slen-9;
        }else{
            l_ap.sbuf=sbuf+3;
            l_ap.slen=slen-3;
        }
	}else if (MODBUS_CMD_16==l_data[1]){
        l_len=l_data[6];
        if (MODBUS_RTU==type){
            l_crc=getCRC16(l_data,l_len+7);
            if (l_crc!=(l_data[l_len+8]<<8 | l_data[l_len+7])){//CRC error
                return -1;
            }
        }
        //data process
        l_ap.addr=l_data[0];
        l_ap.cmd =l_data[1];
        l_ap.data=l_data+2;
        l_ap.len =l_len;
        if (MODBUS_TCP==type){
            l_ap.sbuf=sbuf+12;
            l_ap.slen=slen-12;
        }else{
            l_ap.sbuf=sbuf+6;
            l_ap.slen=slen-6;
        }
	}
    l_ret=msb(&l_ap,&l_err);
    if (l_err==0 && l_ret==0){
        return 0;
    }
    //data package
    l_spp.typeuc=type;
    l_spp.addruc=l_data[0];
    l_spp.cmduc =l_data[1];
    l_spp.erruc =l_err;
    l_spp.lenuc =l_ret;
    l_spp.slenuc=slen;
    l_spp.datauc=data;
    l_spp.sbufuc=sbuf;
    l_ret=Modbus_S_Package(&l_spp);
    return l_ret;
}

int Modbus_S_Package(TSPackageParam *spp)
{
    uint8_t l_ret=0;
    uint16_t l_crc;
    //uint8_t *l_data;
    //error package
    if (spp->erruc!=0){
        if (MODBUS_TCP==spp->typeuc){
            memcpy(spp->sbufuc,spp->datauc,6);
            spp->sbufuc[4]=0;
            spp->sbufuc[5]=9;
            //l_data=spp->sbufuc+6;
            l_ret=6;
        }else{
            //l_data=spp->sbufuc;
            //l_ret=5;
        }
        spp->sbufuc[l_ret]=spp->addruc;
        spp->sbufuc[l_ret+1]=spp->cmduc | MODBUS_ERROR;
        spp->sbufuc[l_ret+2]=spp->erruc;
        if (MODBUS_RTU==spp->typeuc){
            l_crc=getCRC16(spp->sbufuc,3);
            spp->sbufuc[l_ret+3]=l_crc & 0xFF;
            spp->sbufuc[l_ret+4]=(l_crc>>8) & 0xFF;
            return l_ret+5;
        }
        return l_ret+3;
    }
    l_ret=2;
    if (MODBUS_TCP==spp->typeuc){
        l_ret=8;
    }
    /*if (MODBUS_CMD_03==spp->cmduc || MODBUS_CMD_04==spp->cmduc){
        l_data[0]=spp->lenuc;
        if (MODBUS_RTU==spp->typeuc){
            l_crc=getCRC16(spp->sbufuc,spp->lenuc+3);
            l_data[spp->lenuc+1]=l_crc & 0xFF;
            l_data[spp->lenuc+2]=l_crc>>8 & 0xFF;
        }
        l_ret=spp->lenuc+3;
    }else if (MODBUS_CMD_06==spp->cmduc || MODBUS_CMD_16==spp->cmduc){
        l_ret=6;
        if (MODBUS_CMD_16==spp->cmduc){
            l_ret++;
        }
        if (MODBUS_RTU==spp->typeuc){
            memcpy(spp->sbufuc,spp->datauc,l_ret);
            l_crc=getCRC16(spp->sbufuc,l_ret);
            spp->sbufuc[l_ret]=l_crc & 0xFF;
            spp->sbufuc[l_ret+1]=l_crc>>8 & 0xFF;
        }else{
            memcpy(spp->sbufuc+6,spp->datauc+6,l_ret);
        }
    }*/
    if (MODBUS_CMD_03==spp->cmduc || MODBUS_CMD_04==spp->cmduc){
        spp->sbufuc[l_ret]=spp->lenuc;
        l_ret+=spp->lenuc+1;
    }else if (MODBUS_CMD_06==spp->cmduc){
        memcpy(spp->sbufuc+l_ret,spp->datauc+l_ret,4);
        l_ret+=4;
    }else if (MODBUS_CMD_16==spp->cmduc){
        memcpy(spp->sbufuc+l_ret,spp->datauc+l_ret,5);
        l_ret+=5;
    }

    
    if (MODBUS_TCP==spp->typeuc){
        memcpy(spp->sbufuc,spp->datauc,6);
        spp->sbufuc[4]=(l_ret-6)>>8 & 0xFF;
        spp->sbufuc[5]=(l_ret-6) & 0xFF;
        spp->sbufuc[6]=spp->addruc;
        spp->sbufuc[7]=spp->cmduc;
        //l_ret+=6;//add tcp head len
    }else{
        spp->sbufuc[0]=spp->addruc;
        spp->sbufuc[1]=spp->cmduc;
        l_crc=getCRC16(spp->sbufuc,l_ret);
        spp->sbufuc[l_ret]=l_crc & 0xFF;
        spp->sbufuc[l_ret+1]=l_crc>>8 & 0xFF;
        l_ret+=2;//add crc len
    }

    return l_ret;
}

/*int Modbus_S_Package(TPackageParam *pp)
{
}*/
#if 0
/***************************************
功能：03命令处理函数，03在MODBUS里面是读N个寄存器的值
****************************************/
/////////////////////////////////////////////////////////////////////// 
int Modbus_Function_3(uint8_t* pdata,int datalen,uint8_t *sendbuf) 
{ 
    uint16_t tempdress = 0;
    uint16_t crcresult;
    uint16_t sendlen = 0 ;
    tempdress = (pdata[2] << 8) + pdata[3]; //得到读取的启始地址
    
    //？？ 这里需要增加对访问地址是否越限的判断
    uint8_t tx_flat = 0;
    tx_flat = Modbus_Addr(tempdress,pdata[5],0x03);
   
    if(tx_flat == 1)
    {
        sendbuf[0] = pdata[0]; //站地址
        sendbuf[1] = 0x03;  //命令符号
        sendbuf[2] = 2 * pdata[5]; //请求字节数（4B*2）
    
        Modbus_Read_Data(0x03,tempdress,&sendbuf[3],sendbuf[2]);
    
        sendlen = 2 * pdata[5] + 3;
    }
    else 
    {            
         sendbuf[0]=pdata[0]; //站地址
         sendbuf[1]=0x03|0x80; 
         sendbuf[2]=0x02;
         sendlen = 3 ;
    }
    
    crcresult = getCRC16(sendbuf,sendlen); 
    sendbuf[sendlen] = crcresult & 0xff; 
    sendbuf[sendlen+1] = (crcresult >> 8) & 0xff;
    sendlen = sendlen+2;
    return sendlen;
} 
//////////////////////////////////////////////
/***************************************
功能：04命令处理函数，04在MODBUS里面是读4X地址的数据
****************************************/
//////////////////////////////////////////////
int Modbus_Function_4(uint8_t* pdata,int datalen,uint8_t *sendbuf) 
{
    uint16_t tempdress = 0;
    uint16_t crcresult;
    uint16_t sendlen = 0 ;           
    tempdress = (pdata[2] << 8) + pdata[3]; //得到读取的启始地址
    
    uint16_t tx_flat = Modbus_Addr(tempdress,pdata[5],0x04);
    if(tx_flat == 0x01)
    {
        sendbuf[0] = pdata[0];//站地址
        sendbuf[1] = 0x04;  //命令符号
        sendbuf[2] = 2 * pdata[5]; //请求字节数（4B*2）
        sendlen = 2 * pdata[5] + 3;

        Modbus_Read_Data(0x04,tempdress,&sendbuf[3],sendbuf[2]);
    }
    else 
    {            
         sendbuf[0]=pdata[0]; //站地址
         sendbuf[1]=0x04|0x80; 
         sendbuf[2]=0x02;
    
         sendlen = 3 ;
    }

    crcresult = getCRC16(sendbuf,sendlen); 
    sendbuf[sendlen] = crcresult & 0xff; 
    sendbuf[sendlen+1] = (crcresult >> 8) & 0xff; 
    sendlen = sendlen+2;
    return sendlen;    
}
//////////////////////////////////////////////
/***************************************
功能：06命令处理函数，06在MODBUS里面是写1个寄存器的值
****************************************/
//////////////////////////////////////////////
int Modbus_Function_6(uint8_t* pdata,int datalen,uint8_t *sendbuf) 
{ 
    uint16_t tempdress = 0;
    uint8_t tx_flat = 1;
    uint16_t crcresult;
    uint16_t sendlen = 0 ;

    uint8_t  lx_flat = 1; //lixia;增加数据检错
    tempdress = (pdata[2]<<8) + pdata[3];
    
    tx_flat = Modbus_Addr(tempdress,1,0x06);
    
    
    //如果是有效的地址，则设置发送标志
    //注意：如果是返回错误的话，需要修改下面的代码，当前的处理是如果是无效的地址，则不返回

    if(tx_flat == 1)
    {
        // ????设置值
        lx_flat=Modbus_Write_Data(tempdress,&pdata[4],2);
        
        if(lx_flat==1){       
            sendbuf[0] = pdata[0];   //站地址
            sendbuf[1] = 0x06; 
            sendbuf[2] = pdata[2];   //设置地址
            sendbuf[3] = pdata[3]; 
            sendbuf[4] = pdata[4];   //设置数据
            sendbuf[5] = pdata[5]; 
            sendlen = 6;
        }
        else{
            sendbuf[0] = pdata[0]; //站地址
            sendbuf[1] = 0x06|0x80;   //设置数据错误检错
            sendbuf[2] = 0x04;     
            sendlen = 3;
        }
         
    }
    else
    {
        sendbuf[0] = pdata[0]; //站地址
        sendbuf[1] = 0x06|0x80; 
        sendbuf[2] = 0x02; 
     
        sendlen = 3;
    }
    crcresult = getCRC16(sendbuf,sendlen); 
    sendbuf[sendlen] = crcresult & 0xff; 
    sendbuf[sendlen+1] = (crcresult >> 8) & 0xff; 
    sendlen = sendlen+2; 
    return sendlen;
} 

//////////////////////////////////////////////
/***************************************
功能：10命令处理函数，06在MODBUS里面是写N个寄存器的值
****************************************/
//////////////////////////////////////////////
int Modbus_Function_10(uint8_t* pdata,int datalen,uint8_t *sendbuf) 
{ 
    uint16_t tempdress = 0;
    uint8_t tx_flat = 1;
    uint16_t crcresult;
    uint16_t sendlen = 0;
    uint8_t  lx_flat = 1; //lixia;增加数据检错
    
    tempdress = (pdata[2]<<8) + pdata[3];
    
    tx_flat = Modbus_Addr(tempdress,pdata[5],0x10);
  
    if(tx_flat == 1)
    {
        lx_flat=Modbus_Write_Data(tempdress,&pdata[7],pdata[6]);
        if(lx_flat==1){
            sendbuf[0] = pdata[0];
            sendbuf[1] = 0x10;
            sendbuf[2] = pdata[2];
            sendbuf[3] = pdata[3];
            sendbuf[4] = pdata[4];
            sendbuf[5] = pdata[5];
            sendlen = 6;
        }
        else{
            sendbuf[0] = pdata[0]; //站地址
            sendbuf[1] = 0x10|0x80; 
            sendbuf[2] = 0x04;     //数据错误检错   
            sendlen = 3;
        }
     
    }  
    else
    {
        sendbuf[0] = pdata[0]; //站地址
        sendbuf[1] = 0x10|0x80; 
        sendbuf[2] = 0x02; 
     
        sendlen = 3;
    }
    crcresult = getCRC16(sendbuf,sendlen); 
    sendbuf[sendlen] = crcresult & 0xff; 
    sendbuf[sendlen+1] = (crcresult >> 8) & 0xff; 
    sendlen = sendlen+2;
    return sendlen;
} 

int Modbus_Function_error(uint8_t* pdata,int datalen,uint8_t *sendbuf) 
{
     uint16_t crcresult;
     uint16_t sendlen = 0 ;  
     sendbuf[0] = pdata[0]; //站地址
     sendbuf[1] = pdata[1]|0x80; 
     sendbuf[2] = 0x01;
             
     sendlen = 3;
        
     crcresult = getCRC16(sendbuf,sendlen); 
     sendbuf[sendlen] = crcresult & 0xff; 
     sendbuf[sendlen+1] = (crcresult >> 8) & 0xff; 
     sendlen = sendlen+2; 
     return sendlen;
}
///////////////////////////////////////////////////////////// 
int ModbusDataDeal(uint8_t* pdata,int datalen,uint8_t *sendbuf) 
{ 
    int ret=0;

    if(pdata[0] == 0x01 || pdata[0] == 0)//地址错误不应答
    {
        uint16_t crcresult; 
        uint8_t tmp[2]={0};
        if (g_modbus_mode==MODBUS_RTU){
            crcresult = getCRC16(pdata,datalen-2); 
            tmp[1] = crcresult & 0xff; 
            tmp[0] = (crcresult >> 8) & 0xff;
            if((pdata[datalen-1] == tmp[0])&&(pdata[datalen-2] == tmp[1]))//crc校验错误不应答 
            {
                switch(pdata[1])
                {
                case 0x03:
                    ret=Modbus_Function_3(pdata,datalen,sendbuf);
                    break;
                case 0x06:
                    ret=Modbus_Function_6(pdata,datalen,sendbuf);
                    break;
                case 0x10:
                    ret=Modbus_Function_10(pdata,datalen,sendbuf); 
                    break;
                case 0x04:
                    ret=Modbus_Function_4(pdata,datalen,sendbuf);
                    break;
                default:
                    ret=Modbus_Function_error(pdata,datalen,sendbuf);
                    break;
                }
            }
            else
            {
                ret=Modbus_Function_error(pdata,datalen,sendbuf); 
            }
        }else if (g_modbus_mode==MODBUS_TCP){
            switch(pdata[1])
            {
            case 0x03:
                ret=Modbus_Function_3(pdata,datalen,sendbuf);
                break;
            case 0x06:
                ret=Modbus_Function_6(pdata,datalen,sendbuf);
                break;
            case 0x10:
                ret=Modbus_Function_10(pdata,datalen,sendbuf); 
                break;
            case 0x04:
                ret=Modbus_Function_4(pdata,datalen,sendbuf);
                break;
            default:
                ret=Modbus_Function_error(pdata,datalen,sendbuf);
                break;
            }
        }
    }
    if (pdata[0]==0)
        return 0;
    return ret;
}

/**************************************
功能：判断接收到的数据包所操作的地址范围是否运行
第一个参数是指向接收到的数据包的指针
第二个参数是该数据包的长度
第三个参数是应许的首地址，
第四个参数是应许操作的尾地址
如果允许则返回1，不允许或CRC效验失败返回0
**************************************/
uint8_t Cmp_RWAddr(uint8_t* pdata,int datalen,uint16_t beginaddr,uint16_t endaddr)
{
     uint16_t crcresult;
     uint8_t  temp[2]={0}; 
     uint16_t tempbeginadress = 0;
     uint16_t tempendadress = 0;

     if (g_modbus_mode==MODBUS_RTU){
         crcresult = getCRC16(pdata,datalen-2);
         temp[1]   = crcresult & 0xff; 
         temp[0]   = (crcresult >> 8) & 0xff; 
         if((pdata[datalen-1] == temp[0])&&(pdata[datalen-2] == temp[1])){//crc校验错误不应答
              tempbeginadress = (pdata[2]<<8) + pdata[3];
              tempendadress = tempbeginadress + pdata[5];
              if(tempbeginadress>=beginaddr && tempendadress<=endaddr){
                  return 1;
              }else
                  return 0;
         }
    }else if (g_modbus_mode==MODBUS_TCP){
        tempbeginadress = (pdata[2]<<8) + pdata[3];
        tempendadress = tempbeginadress + pdata[5];
        if(tempbeginadress>=beginaddr && tempendadress<=endaddr){
            return 1;
        }else
            return 0;
    }

     return 0;
}

//modbus tcp
uint8_t Cmp_RWAddrETH(uint8_t* pdata,int datalen,uint16_t beginaddr,uint16_t endaddr)
{
    uint16_t tempbeginadress = 0;
    uint16_t tempendadress = 0;

    tempbeginadress = (pdata[2]<<8) + pdata[3];
    tempendadress = tempbeginadress + pdata[5];
    if(tempbeginadress>=beginaddr && tempendadress<=endaddr)
    {
        return 1;
    }
    else
        return 0;
}

void Modbus_Function_options(uint8_t cmd,int param1,int param2,void *param3)
{
    if (MODBUS_RTU!=cmd && MODBUS_TCP!=cmd){
        return ;
    }
    g_modbus_mode=cmd;
}
#endif

