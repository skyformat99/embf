/********************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD. 
 * File name      :power_meter
 * Author         :Li Ying
 * Date           :2011.08.04
 * Description    :电能计量模块，用于ATT7022B的数据校准和读写
 * Others         :None
 *-------------------------------------------------------------------------------
 * 2011-11-01 : 1.0.0 : xusa
 * Modification   :  代码整理
 ********************************************************************************/
#include "drv_power_meter.h"

//wujing 2013.02.23 用EMBF1.1.1改造驱动     
GPIO_CTL  ATT7022B_CS;
GPIO_CTL  ATT7022B_SCK;
GPIO_CTL  ATT7022B_MOSI;
GPIO_CTL  ATT7022B_MISO;

#define ATT7022B_SPI_CS_LOW             (GPIO_ResetBits(ATT7022B_CS.port, ATT7022B_CS.pin))
#define ATT7022B_SPI_CS_HIGH            (GPIO_SetBits(ATT7022B_CS.port, ATT7022B_CS.pin))

#define ATT7022B_SPI_SCK_LOW            (GPIO_ResetBits(ATT7022B_SCK.port, ATT7022B_SCK.pin))
#define ATT7022B_SPI_SCK_HIGH           (GPIO_SetBits(ATT7022B_SCK.port, ATT7022B_SCK.pin))

#define ATT7022B_SPI_MOSI_LOW           (GPIO_ResetBits(ATT7022B_MOSI.port, ATT7022B_MOSI.pin))
#define ATT7022B_SPI_MOSI_HIGH          (GPIO_SetBits(ATT7022B_MOSI.port, ATT7022B_MOSI.pin))

#define ATT7022B_SPI_MISO_STATUS        (GPIO_ReadInputDataBit(ATT7022B_MISO.port, ATT7022B_MISO.pin))



uint8_t   readpower_switch = 0xAA;               //liy;2011.10.15  0xaa 检测数据功能开启  0x55 检测数据功能关闭

#define HARMONIC_SWITCH 0x55 //liy;2011.10.15  0xaa 谐波检测功能开启  0x55 谐波检测功能关闭

void att7022b_delay(uint32_t nCount)
{
    while(nCount > 0U)
    {
    	nCount--;
    }
}

/***********************************************************************************
 * Function       : ATT7022B_Select
 * Author         : Xu Shun'an
 * Date           : 2011.11.01
 * Description    : ATT7022B片选函数: 选择ATT7022B
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None 
 ***********************************************************************************/ 
void ATT7022B_Select()    
{
	//GPIO_ResetBits(ATT7022B_CS_PORT, ATT7022B_CS_PIN);
    ATT7022B_SPI_CS_LOW;
}


/***********************************************************************************
 * Function       : ATT7022B_Deselect
 * Author         : Xu Shun'an
 * Date           : 2011.11.01
 * Description    : ATT7022B片选函数: 释放ATT7022B 
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None 
 ***********************************************************************************/
void ATT7022B_Deselect()  
{
	//GPIO_SetBits(ATT7022B_CS_PORT, ATT7022B_CS_PIN);
    ATT7022B_SPI_CS_HIGH;
}

/***********************************************************************************
 * Function       : Write_ATT7022B
 * Author         : Xu Shun'an
 * Date           : 2011.11.01
 * Description    : ATT7022B写寄存器数据函数 
 * Calls          : None
 * Input          : command：寄存器地址，data：写入寄存器的数据
 * Output         : None
 * Return         : None 
 ***********************************************************************************/
void Write_ATT7022B(uint8_t command, uint32_t data)
{
    uint8_t i=0;
    
 	OSSchedLock();	
    
    //GPIO_ResetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);
    ATT7022B_SPI_SCK_LOW;
    
    ATT7022B_Deselect();	 
	ATT7022B_Select();	   
    
	command=command|0x80; 

    for(i = 0; i < 8; i++)
    {
        //GPIO_SetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);	
        ATT7022B_SPI_SCK_HIGH;      
        
        if(command&0x80)
        {
           // GPIO_SetBits(ATT7022B_DIN_PORT, ATT7022B_DIN_PIN);    //DIN=1  这里IN和OUT是相对ATT7022B来讲 DIN即发送数据到ATT7022B  DOUT从ATT7022B中发送数据到CPU
           ATT7022B_SPI_MOSI_HIGH; 
        }
        else
        {
           // GPIO_ResetBits(ATT7022B_DIN_PORT, ATT7022B_DIN_PIN);
           ATT7022B_SPI_MOSI_LOW;           
        }
        
        //GPIO_ResetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);	
        ATT7022B_SPI_SCK_LOW;
        
        command=command << 1;
    }
 
    for(i=0; i<24; i++)
    {
        //GPIO_SetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);
        ATT7022B_SPI_SCK_HIGH; 

        if(data&0x800000)
        {
            //GPIO_SetBits(ATT7022B_DIN_PORT, ATT7022B_DIN_PIN); 
            ATT7022B_SPI_MOSI_HIGH;
        }
        else
        {
            //GPIO_ResetBits(ATT7022B_DIN_PORT, ATT7022B_DIN_PIN);
            ATT7022B_SPI_MOSI_LOW;
        }

        //GPIO_ResetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);
        ATT7022B_SPI_SCK_LOW;        

        data=data << 1;
    }
 
    //OSTimeDly(20); //延时10ms  //此处关调度后，用OSTimeDlay()延时是无效的，暂时未出问题，不改动
    att7022b_delay(478260);//不开预取：37ms，开预取:8ms，均测试通过，可用
    ATT7022B_Deselect(); 
    
    OSSchedUnlock();

}

/***********************************************************************************
 * Function       : Read_ATT7022B
 * Author         : Xu Shun'an
 * Date           : 2011.11.01
 * Description    : ATT7022B读寄存器数据函数 
 * Calls          : None
 * Input          : command：寄存器地址
 * Output         : None
 * Return         : 所读寄存器的数据 
 ***********************************************************************************/
uint32_t Read_ATT7022B(uint8_t command)
{
    uint8_t   i   = 0;
    uint16_t  m   = 1500;
    uint32_t data = 0;
    
    OSSchedLock();
    
	//GPIO_ResetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);
    ATT7022B_SPI_SCK_LOW;
    
	ATT7022B_Deselect();
    
    ATT7022B_Select();

	for(i = 0; i < 8; i++)
    {
        //GPIO_SetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);  
        ATT7022B_SPI_SCK_HIGH;
        if(command&0x80)
        {
        	//GPIO_SetBits(ATT7022B_DIN_PORT, ATT7022B_DIN_PIN);
            ATT7022B_SPI_MOSI_HIGH;
        }
        else
        {
        	//GPIO_ResetBits(ATT7022B_DIN_PORT, ATT7022B_DIN_PIN); 
            ATT7022B_SPI_MOSI_LOW;
        }
       
        //GPIO_ResetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN); 
        ATT7022B_SPI_SCK_LOW;
        
        command=command<<1;
	}
    
    while(m > 0)   //延时3us
    {
       m--;
    }
    
    for(i = 0; i < 24; i++)
    {
        data=data<<1;

        //GPIO_SetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);
        ATT7022B_SPI_SCK_HIGH;
        
    //  if(GPIO_ReadInputDataBit(ATT7022B_DOUT_PORT, ATT7022B_DOUT_PIN))
        if(ATT7022B_SPI_MISO_STATUS)
        {
            data=data|0x01;
        }
        
        //GPIO_ResetBits(ATT7022B_SCLK_PORT, ATT7022B_SCLK_PIN);
        ATT7022B_SPI_SCK_LOW;
    }
 
	ATT7022B_Deselect(); 
    
    OSSchedUnlock();

	return(data);
}




/*************************************************************************************
 * Function       : ATT7022B_Data_Caculate
 * Author         : Xu Shun'an
 * Date           : 2011.11.01
 * Description    : ATT7022B数据处理函数，将寄存器的数据提取后转换为所需的实际值
 * Calls          : None
 * Input          : None
 * Output         : None
 * Return         : None 
 *************************************************************************************/
void  ATT7022B_Data_Caculate(void *arg)
{
	  GATTPara *set_data = (GATTPara*)arg;
	  uint32_t i = 0, j = 0;
    float    tmpf = 0;
    
    uint32_t Ia = 0;    // A相电流测量值
    uint32_t Ua = 0;    // A相电压测量值
    uint32_t Pa = 0;    // A相有功功率测量值	
    uint32_t Qa = 0;    // A相无功功率测量值
    uint32_t Sa = 0;    // A相视在功率测量值
    uint32_t Paf= 0;    // A相功率因数测量值
    uint32_t Ib = 0;    // B相电流测量值
    uint32_t Ub = 0;    // B相电压测量值	
    uint32_t Pb = 0;    // B相有功功率测量值
    uint32_t Qb = 0;    // B相无功功率测量值
    uint32_t Sb = 0;    // B相视在功率测量值 
    uint32_t Pbf= 0;    // B相功率因数测量值
    uint32_t Ic = 0;    // C相电流测量值
    uint32_t Uc = 0;    // C相电压测量值	
    uint32_t Pc = 0;    // C相有功功率测量值	
    uint32_t Qc = 0;    // C相无功功率测量值
    uint32_t Sc = 0;    // C相视在功率测量值  
    uint32_t Pcf= 0;    // C相功率因数测量值
  
    uint32_t St = 0;    // 合相视在功率测量值
    uint32_t Pt = 0;    // 合相有功功率测量值
    uint32_t Qt = 0;    // 合相无功功率测量值
    uint32_t Pf= 0;     // 合相功率因数测量值
    
    uint32_t Sa_Harmonic= 0;  // A相视在功率谐波测量值
    uint32_t Sb_Harmonic= 0;  // B相视在功率谐波测量值
    uint32_t Sc_Harmonic= 0;  // C相视在功率谐波测量值
  
 
    
    //全波数据检测
    if (readpower_switch == 0x55)
	{
	  readpower_switch = 0xAA;
	}
	else if (readpower_switch == 0xAA)
    {
        if (set_data->Read_Meter_flag == 0xAA)
        {
            i   = Read_ATT7022B(r_URmsa);    // A相电压，单位为0.1V
            Ua = (i/1024)*10/8;  
                  
        
            i     = Read_ATT7022B(r_IRmsa);  // A相电流，单位为0.1A
            Ia = ((i/1024)*10/8);	

            i = Read_ATT7022B(r_Pa);         // A相有功功率，i*(3200/EC)/2^8,EC=400，单位为W
            if(i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pa = i/32;
           

            set_data->EPa = Read_ATT7022B(r_EPa);    // A相有功电能，单位为
        
            i = Read_ATT7022B(r_Qa);       // A相无功功率，单位为Var
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Qa = i/32; 
                      
            /* B相参数 */
            i   = Read_ATT7022B(r_URmsb);
            Ub = (i/1024)*10/8;    
                 
            i     = Read_ATT7022B(r_IRmsb);
            Ib = ((i/1024)*10/8);
          
            
            i = Read_ATT7022B(r_Pb);	  
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pb = i/32; 
        
            set_data->EPb = Read_ATT7022B(r_EPb); 
        
            i = Read_ATT7022B(r_Qb);      
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Qb = i/32;	
        
            /* C相参数 */
            i   = Read_ATT7022B(r_URmsc);
            Uc = (i/1024)*10/8;
        
            i     = Read_ATT7022B(r_IRmsc);
            Ic = ((i/1024)*10/8);
        
            i = Read_ATT7022B(r_Pc); 
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pc = i/32;
        
            set_data->EPc = Read_ATT7022B(r_EPc);
            
            i = Read_ATT7022B(r_Qc); 
            if (i&0x800000)     //转换为负数
            {
                i^=0xffffff;
                i+=1;
            }
            Qc = i/32;
        
            /* 读取合相参数 */
            i = Read_ATT7022B(r_Pt);    // 合相有功功率 ， i*(3200/EC)/2^6,EC=400
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pt = i/8;             
        
            i = Read_ATT7022B(r_Qt);     //合相无功功率， i*(3200/EC)/2^6,EC=400
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Qt = i/8;              
        
            i = Read_ATT7022B(r_St);  //合相视在功率， i*(3200/EC)/2^6,EC=400
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            St = i/8;   
        
            i = Read_ATT7022B(r_Pft); //合相功率因数，单位为0.001
            if (i &0x800000) 
            {
                i ^= 0xffffff;
                i += 1;
                tmpf = i;
            }

            tmpf = i*100 / 8 / 1024 / 1024;

            Pf = tmpf*10;
       
            if (Pf > 1000)
            {
                Pf = 1000;
            } 
                
            /* 分相功率因数 */		
            i = Read_ATT7022B(r_Pfa); //A相功率因数	
            if (i &0x800000) //liy;2012.1.14 (i &0x8000000)改为(i &0x8000000) liy;2011.9.5  增加读功率因数 功率因数放大1000倍
            {
                i ^= 0xffffff;
								i += 1;
								tmpf = i;
            }
            tmpf = i*100 / 8 / 1024 / 1024;

            Paf = tmpf*10;
           
            if (Paf > 1000)
            {
                Paf = 1000;
            }
           
            i = Read_ATT7022B(r_Pfb); //B相功率因数	
            if (i &0x800000) //liy;2012.1.14 (i &0x8000000)改为(i &0x8000000) liy;2011.9.5  增加读功率因数 功率因数放大1000倍
            {
                i^= 0xffffff;
                i+= 1;
                tmpf = i; 
            }
                tmpf = i*100 / 8 / 1024 / 1024;

            Pbf = tmpf*10;
           
            if (Pbf > 1000)
            {
                Pbf = 1000;
            }

            //C相功率因数
            i = Read_ATT7022B(r_Pfc); //C相功率因数 功率因数放大1000倍

            if (i &0x800000) //liy;2012.1.14 (i &0x8000000)改为(i &0x8000000)	   //C相功率因数 功率因数放大1000倍
            {
                i^= 0xffffff;
                i+= 1;
                tmpf = i;
            }
            tmpf = i*100 / 8 / 1024 / 1024;
            Pcf = tmpf*10;
            if (Pcf > 1000)
            {
                Pcf = 1000;
            } 
        
        
            /* 分相视在功率, 功率*(3200/EC)/2^8   Pa*8/256 */       
            j = Read_ATT7022B(r_Sa);
            if (j&0x8000000)
            {
                j^=0xffffff;
                j+=1;
            }
            j  = j/32; 
            Sa = j;

            j = Read_ATT7022B(r_Sb);      
            if (j&0x8000000)
            {
                j^=0xffffff;
                j+=1;
            }
            j  = j/32;  
            Sb = j;

            j = Read_ATT7022B(r_Sc);  
            if (j&0x8000000)
            {
                j^=0xffffff;
                j+=1;
            }
            j  = j/32; 
            Sc = j;
			
			if(Sa > 0)
			{
				Paf=Pa*1000/Sa;
				if(Paf>1000)
				{
					Paf=1000;
				}
			}
			else
			{
				Paf = 0;
			}
			
			if(Sb > 0)
			{
				Pbf=Pb*1000/Sb;
				if(Pbf>1000)
				{
					Pbf=1000;
				}
			}
			else
			{
				Pbf = 0;
			}
			
			if(Sc > 0)
			{
				Pcf=Pc*1000/Sc;
				if(Pcf>1000)
				{
					Pcf=1000;
				}
			}
			else
			{
				Pcf = 0;
			}
			
			if(St > 0)
			{
				Pf=Pt*1000/St;
				if(Pf>1000)
				{
					Pf=1000;
				}
			}
			else
			{
				Pf = 0;
			}
			
			set_data->ACPowFlag = Read_ATT7022B(r_PFlag);  //读有功和无功功率的方向

            set_data->PosEpt_last = set_data->PosEpt;    //记录上次的正向有功电能  
            
            set_data->PosEpt = Read_ATT7022B(r_PosEpt);  //正相有功电能，,此时读的为脉冲数          
            set_data->NegEpt = Read_ATT7022B(r_NegEpt);  //反相有功电能，,此时读的为脉冲数

            set_data->PosEqt_last = set_data->PosEqt;    //记录上次的正向无功电能       
            set_data->PosEqt=Read_ATT7022B(r_PosEqt);  //正相无功电能，,此时读的为脉冲数          
            set_data->NegEqt=Read_ATT7022B(r_NegEqt);  //反相无功电能，,此时读的为脉冲数
        
            if(set_data->next_day_flag == 1)  //到了第二天或要求恢复出厂值
            {
                if((set_data->PosEpt_last == set_data->PosEpt))  //若要求正向有功电能清零命令后,正向有电能一直保持不变则上传能量为0,因为此时正向有功电能无法清零
                {
                    set_data->EPt=0;         //上传天发电为0 
                }
                else              //清零后
                {
                    set_data->next_day_flag = 0; //说明正向有功电能有变化了,
                }
            }
            
            else
            {
                if(set_data->PosEpt >= set_data->NegEpt)
                {
                    set_data->EPt=set_data->PosEpt-set_data->NegEpt; 
                }
                else
                {
                    set_data->EPt=0; 
                }
            }
						
            if(set_data->next_day_Eqtflag == 1)  //liy;2011.9.28 无功电能通知准备清0的标示 到了第二天或要求恢复出厂值
            {
                if((set_data->PosEqt_last==set_data->PosEqt))  //若要求正向有功电能清零命令后,正向有电能一直保持不变则上传能量为0,因为此时正向有功电能无法清零
                {
                    set_data->Eqt=0;         //上传天发电为0 
                }
                else              //清零后
                {
                    set_data->next_day_Eqtflag = 0; //说明正向有功电能有变化了,
                }
            }
            else
            {   
                if(set_data->PosEqt>=set_data->NegEqt)
                {
                    set_data->Eqt=(set_data->PosEqt-set_data->NegEqt)*CT_RATE; 
                }
                else
                {
                    set_data->Eqt=0; 
                }
            }
        
            i = Read_ATT7022B(r_Freq); //线频率，放大100倍
            
            set_data->dis_Fre = (i/1024)*100/8;
                 
            set_data->dis_Ia = Ia*CT_RATE;
            set_data->dis_Ib = Ib*CT_RATE;
            set_data->dis_Ic = Ic*CT_RATE;

            set_data->dis_Va = Ua;
            set_data->dis_Vb = Ub;
            set_data->dis_Vc = Uc;

            set_data->dis_Pa = Pa*CT_RATE;
            set_data->dis_Pb = Pb*CT_RATE;
            set_data->dis_Pc = Pc*CT_RATE;
            set_data->dis_Pt = Pt*CT_RATE;
            
            set_data->dis_EPt = set_data->EPt*CT_RATE;

            //合相和分相无功功率W
            set_data->dis_Qa = Qa*CT_RATE;
            set_data->dis_Qb = Qb*CT_RATE;
            set_data->dis_Qc = Qc*CT_RATE;
            set_data->dis_Qt = Qt*CT_RATE;
            set_data->dis_Sa = Sa*CT_RATE;
            set_data->dis_Sb = Sb*CT_RATE;
            set_data->dis_Sc = Sc*CT_RATE;
            set_data->dis_St = St*CT_RATE;
            set_data->dis_Pfa = Paf;
            set_data->dis_Pfb = Pbf;
            set_data->dis_Pfc = Pcf;
            set_data->dis_Pft = Pf;
        }

        /* 谐波检测功能 */
        if((set_data->Calibration_flag == 0x55) && (set_data->Read_Meter_flag == 0x55 ) && (HARMONIC_SWITCH == 0xAA ))
        {
            i   = Read_ATT7022B(r_URmsa);
            Ua = (i/1024)*10/8;            
        
            i = Read_ATT7022B(r_IRmsa);
            Ia = ((i/1024)*10/8);	  
            
            i = Read_ATT7022B(r_Pa);       
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pa = i/32;	      
        
            i = Read_ATT7022B(r_Qa);       
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Qa = i/32;

            i   = Read_ATT7022B(r_URmsb);
            Ub = (i/1024)*10/8;     
         
            i  = Read_ATT7022B(r_IRmsb);	
            Ib =((i/1024)*10/8);
         
            i = Read_ATT7022B(r_Pb);	
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pb = i/32; 
         
            i = Read_ATT7022B(r_Qb);    
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Qb = i/32;
         
            i = Read_ATT7022B(r_URmsc);
            Uc = (i/1024)*10/8;   
         
            i = Read_ATT7022B(r_IRmsc);
            Ic = ((i/1024)*10/8);
         
            i = Read_ATT7022B(r_Pc);       
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pc = i/32;
         
            i = Read_ATT7022B(r_Qc);   
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Qc = i/32;
        
            i = Read_ATT7022B(r_Pt); 
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Pt=i/8;
        
            i = Read_ATT7022B(r_Qt);  
            if (i&0x800000)
            {
                i^=0xffffff;
                i+=1;
            }
            Qt=i/8;
        
            i = Pa*Pa*4+Qa*Qa*4;
            Sa_Harmonic =  sqrt((double)i);
        
            i = Pb*Pb*4+Qb*Qb*4;
            Sb_Harmonic =  sqrt((double)i);
        
            i = Pc*Pc*4+Qc*Qc*4;
            Sc_Harmonic =  sqrt((double)i);
        
            set_data->Ua_Harmonic = Ua;
            set_data->Ua_Harmonic = Ub;
            set_data->Ua_Harmonic = Uc;
        
            set_data->Pa_Harmonic = Pa*CT_RATE;
            set_data->Pb_Harmonic = Pb*CT_RATE;
            set_data->Pc_Harmonic = Pc*CT_RATE;
        
            if (set_data->dis_Va > 0)
            {
                set_data->Ua_HarmRate = (uint16_t)((double)(set_data->Ua_Harmonic*10000)/set_data->dis_Va);
            }
            else
            {
                set_data->Ua_HarmRate = 0;
            }
            
            if (set_data->dis_Vb > 0)
            {
                set_data->Ub_HarmRate = (uint16_t)((double)(set_data->Ub_Harmonic*10000)/set_data->dis_Vb);
            }
            else
            {
                set_data->Ub_HarmRate = 0;
            }
            
            if (set_data->dis_Vc > 0)
            {
                set_data->Uc_HarmRate = (uint16_t)((double)(set_data->Uc_Harmonic*10000)/set_data->dis_Vc);
            }
            else
            {
                set_data->Uc_HarmRate = 0;
            }
            
           
            
            if ((set_data->Ua_Harmonic > 0) && (set_data->dis_Ia > 0))
            {
                set_data->Ia_HarmRate = (uint16_t)(((double)(Sa_Harmonic*1000)/set_data->Ua_Harmonic*1000/set_data->dis_Ia/0.866));
            }
            else
            {
                set_data->Ia_HarmRate = 0;
            }
            
            if ((set_data->Ub_Harmonic > 0) && (set_data->dis_Ib > 0))
            {
                set_data->Ib_HarmRate = (uint16_t)(((double)(Sb_Harmonic*1000)/set_data->Ub_Harmonic*1000/set_data->dis_Ib/0.866));
            }
            else
            {
                set_data->Ib_HarmRate = 0;
            }
            
            if ((set_data->Uc_Harmonic > 0) && (set_data->dis_Ic > 0))
            {
                set_data->Ic_HarmRate = (uint16_t)(((double)(Sc_Harmonic*1000)/set_data->Uc_Harmonic*1000/set_data->dis_Ic/0.866));
            }
            else
            {
                set_data->Ic_HarmRate = 0;
            }
        }
        
        /* 谐波与全波检测切换功能，首次不读数据 */
        if(set_data->Read_Meter_flag == 0x55)
        {
            readpower_switch = 0x55;
            set_data->Read_Meter_flag = 0xAA;
            set_data->Read_Meter_count = 0;
            Write_ATT7022B(w_EnLineFreq, 0x007812);
            Write_ATT7022B(w_EnHarmonic, 0x0055AA);
            Write_ATT7022B(w_SelectPQSU, 0x001229);
        }

        else  if((0x55 == set_data->Calibration_flag)&&(0xAA == set_data->Read_Meter_flag)&&(0xAA == HARMONIC_SWITCH ) ) 
        {
            if(set_data->Read_Meter_count < 9)
            {
                set_data->Read_Meter_count++;
            }
        
            else
            {
                readpower_switch = 0x55;
                set_data->Read_Meter_flag = 0x55;
                Write_ATT7022B(w_EnLineFreq, 0x007812);
                Write_ATT7022B(w_EnHarmonic, 0x0055AA);
                Write_ATT7022B(w_SelectPQSU, 0x001228);
            }
        }
    }
    
    set_data->dis_Qa_PC = set_data->dis_Qa;  //lixia;2011.12.28
	set_data->dis_Qb_PC = set_data->dis_Qb;
	set_data->dis_Qc_PC = set_data->dis_Qc;
	set_data->dis_Qt_PC = set_data->dis_Qt;

	if((set_data->ACPowFlag&0x80)!=0) //说明合相无功功率为负//lixia;2011.10.12   dis_Qb
	{
			set_data->dis_Qt=set_data->dis_Qt^0xffffffff;
			set_data->dis_Qt+=1;							  //显示取补码
	}
	if((set_data->ACPowFlag&0x10)!=0) //说明A相无功功率为负//lixia;2011.10.12   dis_Qa
	{
		 set_data->dis_Qa=set_data->dis_Qa^0xffffffff;
		 set_data->dis_Qa+=1;							  //显示取补码
	}
	if((set_data->ACPowFlag&0x20)!=0) //说明B相无功功率为负//lixia;2011.10.12   dis_Qb
	{
		 set_data->dis_Qb=set_data->dis_Qb^0xffffffff;
		 set_data->dis_Qb+=1;							  //显示取补码
	}
    if((set_data->ACPowFlag&0x40)!=0) //说明C相无功功率为负//lixia;2011.10.12   dis_Qc
	{
		 set_data->dis_Qc=set_data->dis_Qc^0xffffffff;
		 set_data->dis_Qc+=1;							  //显示取补码
	}
    
}
/***********************************************************************************
 * Function       : ATT7022B_open
 * Author         : wujing
 * Date           : 2012.11.29
 * Description    : ATT7022B初始化函数，
 * Calls          : 无
 * Input          : 无
 				  : 无
 * Output         : None
 * Return         : 返回：DSUCCESS 
 ***********************************************************************************/
int32_t ATT7022B_open(int32_t flag, int32_t mode)
{
    device_gpio_config(DEVICE_ID_ATT7022B, TRUE);//初始化IO口
    
    ATT7022B_CS   =  get_gpio_ctl_attr(DEVICE_ID_ATT7022B, PIN_GPIO_CS);
    ATT7022B_SCK  =  get_gpio_ctl_attr(DEVICE_ID_ATT7022B, PIN_GPIO_SCK);
    ATT7022B_MOSI =  get_gpio_ctl_attr(DEVICE_ID_ATT7022B, PIN_GPIO_MOSI);
    ATT7022B_MISO =  get_gpio_ctl_attr(DEVICE_ID_ATT7022B, PIN_GPIO_MISO);
    
    return DSUCCESS;
}

/***********************************************************************************
 * Function       : ATT7022B_write
 * Author         : wujing
 * Date           : 2012.11.29
 * Description    : 向ATT7022B写数据，command-寄存器地址的指针，data-写入寄存器的数据
 * Calls          : Write_ATT7022B()
 * Input          : mode:
 * Output         : None
 * Return         : 成功返回传入的命令字，失败返回OPFAULT 
 ***********************************************************************************/
int32_t ATT7022B_write(const uint8_t *command, uint32_t data)
{
    int32_t ret = OPFAULT;
    if(command != NULL)
    {
	    Write_ATT7022B(*(uint8_t*)command,data);
        ret = (int32_t)data;
    }
    return ret;
}
/***********************************************************************************
 * Function       : ATT7022B_read
 * Author         : wujing
 * Date           : 2012.11.29
 * Description    : 读ATT7022B数据，arg-存储变量的GATTPara类型的指针
 * Calls          : 
 * Input          : buf - GATTPara类型的指针
 * Output         : None
 * Return         : 成功返回1，失败返回OPFAULT 
 ***********************************************************************************/
int32_t ATT7022B_read(void *arg, uint32_t count)
{
    int32_t ret = OPFAULT;
    if(arg != NULL)
    {
		ATT7022B_Data_Caculate(arg);
        ret = 1;
    }
    return ret;
}
/***********************************************************************************
 * Function       : ATT7022B_ioctl
 * Author         : wujing
 * Date           : 2012.11.29
 * Description    : 读ATT7022B数据，arg-存储变量的GATTPara类型的指针
 * Calls          : 
 * Input          : buf - GATTPara类型的指针
 * Output         : None
 * Return         : 成功返回DSUCCESS，失败返回OPFAULT 
 ***********************************************************************************/
int32_t ATT7022B_ioctl(uint32_t cmd, void *arg)
{
    int32_t ret = OPFAULT;  
    GATTPara *set_data = (GATTPara*)arg;

    if (set_data!= NULL)
    {
		if(1 == set_data->next_day_flag)	 //next_day_flag为1时，
		{
		 	set_data->PosEpt2 = Read_ATT7022B(r_PosEpt2); //正相有功电能，读后清零,此时读的为脉冲数
		 	set_data->NegEpt2 = Read_ATT7022B(r_NegEpt2); //反相有功电能，读后清零,此时读的为脉冲数			
			set_data->PosEqt2 = Read_ATT7022B(r_PosEqt2); //正相无功电能，读后清零,此时读的为脉冲数
		 	set_data->NegEqt2 = Read_ATT7022B(r_NegEqt2); //反相无功电能，读后清零,此时读的为脉冲数
			set_data->next_day_flag = 1;//1.9
			set_data->next_day_Eqtflag = 1; // 无功电能通知准备清0的标示 上面液晶通知第二天到了或恢复出厂值了
			return DSUCCESS;
		}
		
		if(0 == set_data->ATT7022B_Data_Adjust_flag)
		{
			/**********有功功率增益**********/
	        Write_ATT7022B(w_PgainA0, set_data->PgainA0);      
		    Write_ATT7022B(w_PgainB0, set_data->PgainB0);           
		    Write_ATT7022B(w_PgainC0, set_data->PgainC0);    
		 										
	        Write_ATT7022B(w_PgainA1, set_data->PgainA0);       
		    Write_ATT7022B(w_PgainB1, set_data->PgainB0);       
		    Write_ATT7022B(w_PgainC1, set_data->PgainC0);   
		  
	        /**********电压增益*************/
		    Write_ATT7022B(w_UgainA, set_data->UgainA);
	   	    Write_ATT7022B(w_UgainB, set_data->UgainB);  
		    Write_ATT7022B(w_UgainC, set_data->UgainC); 
		   
	        /**********电流增益*************/ 
		    Write_ATT7022B(w_IgainA, set_data->IgainA);
		    Write_ATT7022B(w_IgainB, set_data->IgainB); 
		    Write_ATT7022B(w_IgainC, set_data->IgainC); 
	        
			/**********无功功率增益**********/
			Write_ATT7022B(w_PhsregA0, set_data->QgainA0);
	        Write_ATT7022B(w_PhsregB0, set_data->QgainB0); 
	        Write_ATT7022B(w_PhsregC0, set_data->QgainC0);
			ret = DSUCCESS;
		}
		else if(1 == set_data->ATT7022B_Data_Adjust_flag)
		{
			if(0 == set_data->CorrParaPow)	
			{
				/**********有功功率增益**********/
	            Write_ATT7022B(w_PgainA0, set_data->PgainA0);        
		        Write_ATT7022B(w_PgainB0, set_data->PgainB0);           
	            Write_ATT7022B(w_PgainC0, set_data->PgainC0);        							
	            
                Write_ATT7022B(w_PgainA1, set_data->PgainA0); 
		        Write_ATT7022B(w_PgainB1 ,set_data->PgainB0);           
		        Write_ATT7022B(w_PgainC1, set_data->PgainC0);      
	            
	            /**********电压增益*************/
	            Write_ATT7022B(w_UgainA, set_data->UgainA);     
		        Write_ATT7022B(w_UgainB, set_data->UgainB); 
		        Write_ATT7022B(w_UgainC, set_data->UgainC);  
		        
	            /**********电流增益************/
	            Write_ATT7022B(w_IgainA, set_data->IgainA);
		        Write_ATT7022B(w_IgainB, set_data->IgainB);
		        Write_ATT7022B(w_IgainC, set_data->IgainC); 
			    ret = DSUCCESS;		
			}
			else if(1 == set_data->CorrParaPow)
			{
	            /**********无功功率增益**********/
	            Write_ATT7022B(w_PhsregA0, set_data->QgainA0);
	            Write_ATT7022B(w_PhsregB0, set_data->QgainB0);
	            Write_ATT7022B(w_PhsregC0, set_data->QgainC0);
				ret = DSUCCESS;			
			}
			else
			{
				//do nothing for MISRA-2004
			}		
		}
    }
	if(cmd == THREE_PHASE_THREE_WIRE)
	{
		Write_ATT7022B(w_UADCPga, 0x465501);  // 电压AD通道放大倍数4       
	    Write_ATT7022B(w_HFConst, 12);        // 12电压通道ADC放大4倍，则   4*5760000000*0.648*0.648*0.1*0.1*21/(220*400*400)=57.7 对应21欧姆电阻  脉冲常数定下后应该此值影响不大                             
	    Write_ATT7022B(w_Istartup, 0x00087E); // 启动电流	
	    Write_ATT7022B(w_EAddMode, 0x000000); // 000001对应三相四线制用,0000002对应三相三线制用,默认为2

        ret = DSUCCESS;
	}
	else if(cmd == THREE_PHASE_FOUR_WIRE)
	{
		Write_ATT7022B(w_UADCPga, 0x465501);   // 电压AD通道放大倍数4       
		Write_ATT7022B(w_HFConst, 12);         // 12电压通道ADC放大4倍，则4*5760000000*0.648*0.648*0.1*0.1*21/(220*400*400)=57.7 对应21欧姆电阻  脉冲常数定下后应该此值影响不大                             
		Write_ATT7022B(w_Istartup,0x00087E);   // 启动电流	
		Write_ATT7022B(w_EAddMode,0x000001);   // 000001对应三相四线制用, 0000002对应三相三线制用, 默认为2
		
        ret = DSUCCESS;
	}
	return ret;
}


