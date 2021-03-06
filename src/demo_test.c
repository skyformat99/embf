/****************************************************************************** 
* Copyright (C), 1997-2013, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :demo_test.c
* Author         :wujing    
* Date           :2013-01-26
* Description    :demo及测试示例
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2013-01-26 : 1.0.0 : wujing
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include "demo_test.h"

/******************************************************************************
* Function       :dido_demo_test
* Author         :wujing	
* Date           :2013-3-4
* Description    :DIDO测试demo,同样可以为应用程序开发人员提供使用参考
*                :注意:不同板号的DIDO不同，故用宏定义来区分，功能验证时需单步调试验证
* Calls          :drv_ioctl()、io_ctrl()、drv_read()、drv_open()、drv_close()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :无
*******************************************************************************
* History:        2013-3-4
*------------------------------------------------------------------------------
* 2013-3-4 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t dido_test_buf[8]={0};
uint8_t dido_get_flag = 0;

#if (defined P_P_000071) || (defined P_P_000069)
void dido_demo_test(void)
{
    gt_glp.fdido       = drv_open("DIDO", 0, 0);  //打开设备，可以重复打开，
    
    drv_ioctl(gt_glp.fdido,DIG_OUT1|DO_STATE_HIGH,NULL); //继电器输出1
    drv_ioctl(gt_glp.fdido,DIG_OUT1|DO_STATE_LOW,NULL);  
    drv_ioctl(gt_glp.fdido,DIG_OUT2|DO_STATE_HIGH,NULL); //继电器输出2
    drv_ioctl(gt_glp.fdido,DIG_OUT2|DO_STATE_LOW,NULL);   
    drv_ioctl(gt_glp.fdido,LED_RED|DO_STATE_HIGH,NULL);  //红灯操作
    drv_ioctl(gt_glp.fdido,LED_RED|DO_STATE_LOW,NULL);    
    drv_ioctl(gt_glp.fdido,LED_GREEN|DO_STATE_HIGH,NULL);//绿灯操作
    drv_ioctl(gt_glp.fdido,LED_GREEN|DO_STATE_LOW,NULL);

    drv_ioctl(gt_glp.fdido,POWER_IO_5V|DO_STATE_LOW,NULL); //外设电源控制 LOW为关
    drv_ioctl(gt_glp.fdido,POWER_IO_5V|DO_STATE_HIGH,NULL);

    drv_ioctl(gt_glp.fdido,HMI_A|DO_STATE_HIGH,NULL);     //液晶唤醒输出，初始为高电平
    drv_ioctl(gt_glp.fdido,HMI_A|DO_STATE_LOW,NULL);            
    drv_ioctl(gt_glp.fdido,WIFI_RESET|DO_STATE_HIGH,NULL);//WIFI_RESET输出
    drv_ioctl(gt_glp.fdido,WIFI_RESET|DO_STATE_LOW,NULL); 
    
    io_ctrl(UTILS_IO_LEDGREEN,UTILS_IO_HIGH);            //绿灯操作
    dido_get_flag = io_get(UTILS_IO_LEDGREEN);
    io_ctrl(UTILS_IO_LEDGREEN,UTILS_IO_LOW);
    dido_get_flag = io_get(UTILS_IO_LEDGREEN);
    
    io_ctrl(UTILS_IO_LEDRED,UTILS_IO_HIGH);             //红灯操作
    dido_get_flag = io_get(UTILS_IO_LEDRED);   
    io_ctrl(UTILS_IO_LEDRED,UTILS_IO_LOW);
    dido_get_flag = io_get(UTILS_IO_LEDRED);   
    
    io_ctrl(UTILS_IO_WAKE_DSP_OUT,UTILS_IO_HIGH);        //液晶唤醒输出
    dido_get_flag = io_get(UTILS_IO_WAKE_DSP_OUT);   
    io_ctrl(UTILS_IO_WAKE_DSP_OUT,UTILS_IO_LOW);
    dido_get_flag = io_get(UTILS_IO_WAKE_DSP_OUT);   
    
    io_ctrl(UTILS_IO_PWRIO,UTILS_IO_LOW);    
    dido_get_flag = io_get(UTILS_IO_PWRIO); 
    io_ctrl(UTILS_IO_PWRIO,UTILS_IO_HIGH);              //外设电源控制
    dido_get_flag = io_get(UTILS_IO_PWRIO);   
    
    io_ctrl(UTILS_IO_WIFI_RESET,UTILS_IO_HIGH);         //WIFI_RESET输出
    dido_get_flag = io_get(UTILS_IO_WIFI_RESET); 
    io_ctrl(UTILS_IO_WIFI_RESET,UTILS_IO_LOW);  
    dido_get_flag = io_get(UTILS_IO_WIFI_RESET); 


    drv_read(gt_glp.fdido,dido_test_buf,8);           //多组DI输入读取
    dido_get_flag = io_get(UTILS_IO_WAKE_HMI_IN);                   //DSP唤醒液晶输入管脚状态获取
    
    drv_close(gt_glp.fdido); //关闭设备
    
    io_ctrl(UTILS_IO_LEDRED,UTILS_IO_HIGH);             //测试设备关闭后的红灯操作
    dido_get_flag = io_get(UTILS_IO_LEDRED);              //会返回FF，操作失败
    io_ctrl(UTILS_IO_LEDRED,UTILS_IO_LOW);
    dido_get_flag = io_get(UTILS_IO_LEDRED);  
    
}
#endif
#ifdef P_A_000092
void dido_demo_test(void)
{
    gt_glp.fdido       = drv_open("DIDO", 0, 0);  //打开设备，可以重复打开，
    
    io_ctrl(UTILS_IO_LED_RUN,UTILS_IO_LOW);   //点亮  pass
    io_ctrl(UTILS_IO_LED_RUN,UTILS_IO_HIGH);  //熄灭  pass
    
    drv_read(gt_glp.fdido,dido_test_buf,8);           //多组DI输入读取 pass
    
    drv_ioctl(gt_glp.fdido,DIG_OUT4|DO_STATE_LOW,0); // 继电器编号1  pass
    drv_ioctl(gt_glp.fdido,DIG_OUT4|DO_STATE_HIGH,0); //继电器编号1  pass
    
    drv_ioctl(gt_glp.fdido,DIG_OUT5|DO_STATE_LOW,0);//继电器编号2   pass
    drv_ioctl(gt_glp.fdido,DIG_OUT5|DO_STATE_HIGH,0);//继电器编号2  pass
    
    drv_close(gt_glp.fdido);  //关闭设备后，操作没有响应
    
    io_ctrl(UTILS_IO_LED_RUN,UTILS_IO_LOW);   //点亮  pass
    io_ctrl(UTILS_IO_LED_RUN,UTILS_IO_HIGH);  //熄灭  pass
    
}
#endif

/******************************************************************************
* Function       :eeprom_demo_test
* Author         :wujing	
* Date           :2013-3-4
* Description    :EEPROM测试demo，同时给应用程序提供使用参考
*                :注意:1000字节的读写已经测试过，太耗资源，故改用小容量数组
* Calls          :drv_ioctl()、io_ctrl()、drv_read()、drv_open()、drv_close()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :true:写入与读出数据一致；
*                :false:不一致
*******************************************************************************
* History:        2013-3-4
*------------------------------------------------------------------------------
* 2013-3-4 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t eeprom_test_buf_input[30] = {0};
uint8_t eeprom_test_buf_output[30] = {0};

// EEPROM 1000字节读写已经测过
int32_t eeprom_demo_test(void) //因EEPROM不能重复打开，故最初执行时需判断设备是否打开，如果打开的话先关闭设备，再打开，未打开--直接打开操作
{
    uint16_t i;
    if(0 != gt_glp.feeprom1)//一般测试前设备都是打开的
    {
        drv_close(gt_glp.feeprom1);
        gt_glp.feeprom1  = drv_open("FM31XX0", 0, 0);
    }
    int32_t ret = false;
    for(i=0;i<15;i++)
    {
        eeprom_test_buf_input[i] = 0xAA;
    }    
    for(i=15;i<30;i++)
    {
        eeprom_test_buf_input[i] = 0x55;
    } 
    write_eeprom(0, eeprom_test_buf_input, 30);  //操作接口为utils.c提供，不能直接操作驱动 
    read_eeprom(0, eeprom_test_buf_output, 30);
    
    for(i=0;i<30;i++)
    {
        if(eeprom_test_buf_input[i] != eeprom_test_buf_output[i])
        {
            ret = false;
            break;
        }
    }
    if(30 == i)
    {
        ret = true;
    }
    return ret;    
}
/******************************************************************************
* Function       :flash_demo_test
* Author         :wujing	
* Date           :2013-3-4
* Description    :FLASH测试demo，同时给应用程序提供使用参考
* Calls          :drv_ioctl()、io_ctrl()、drv_read()、drv_open()、drv_close()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :true:写入与读出数据一致；
*                :false:不一致
*******************************************************************************
* History:        2013-3-4
*------------------------------------------------------------------------------
* 2013-3-4 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t flash_test_buf_input[30] = {0};
uint8_t flash_test_buf_output[30] = {0};

int32_t flash_demo_test(void) //因FLASH不能重复打开，故最初执行时需判断设备是否打开，如果打开的话先关闭设备，再打开，未打开--直接打开操作
{
    uint16_t i;
    if(0 != gt_glp.fflash1)//一般测试前设备都是打开的
    {
        drv_close(gt_glp.fflash1);
        gt_glp.fflash1 = drv_open("W25QX0", 0, 0);
    }
    int32_t ret = false;
    for(i=0;i<15;i++)
    {
        flash_test_buf_input[i] = 0xAA;
    }    
    for(i=15;i<30;i++)
    {
        flash_test_buf_input[i] = 0x55;
    } 
    write_eeprom(0, flash_test_buf_input, 30);  //操作接口为utils.c提供，不能直接操作驱动 
    read_eeprom(0, flash_test_buf_output, 30);
    
    for(i=0;i<30;i++)
    {
        if(flash_test_buf_input[i] != flash_test_buf_output[i])
        {
            ret = false;
            break;
        }
    }
    if(30 == i)
    {
        ret = true;
    }
    return ret;    
}
/******************************************************************************
* Function       :rtc_demo_test
* Author         :wujing	
* Date           :2013-3-4
* Description    :rtc测试demo，同时给应用程序提供使用参考,读写RTC，看结果是否符合预期
*                :rtc读写格式为BCD码
* Calls          :drv_read()、drv_open()、drv_close() drv_write()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :
*******************************************************************************
* History:        2013-3-4
*------------------------------------------------------------------------------
* 2013-3-4 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t time_read_buf[6]={0};
uint8_t time_write_buf[6] = {0x13,0x03,0x04,0x13,0x35,0x23};
void r8025_demo_test(void)
{
    if(0 != gt_glp.fflash1)//一般测试前设备都是打开的
    {
        drv_close(gt_glp.frtc);
        gt_glp.frtc = drv_open("RTC0", 0, 0); 
    }
    drv_read(gt_glp.frtc, time_read_buf, 6); 
    drv_write(gt_glp.frtc,time_write_buf,6);
    drv_read(gt_glp.frtc, time_read_buf, 6); 
}
/******************************************************************************
* Function       :adc_demo_test
* Author         :wujing	
* Date           :2013-3-4
* Description    :adc测试demo，同时给应用程序提供使用参考,
* Calls          :drv_ioctl()、io_ctrl()、drv_read()、drv_open()、drv_close()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :
*******************************************************************************
* History:        2013-3-4
*------------------------------------------------------------------------------
* 2013-3-4 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint16_t adbuf[8]  = { 0 };	
void adc_demo_test(void)
{
    if(0 != gt_glp.fadc1)//一般测试前设备都是打开的
    {
        drv_close(gt_glp.fadc1);
        gt_glp.fadc1 = drv_open("ADC3", 0, 0xC3B0); //按需要的通道打开设备 
    }
    drv_read(gt_glp.fadc1, (uint8_t*)adbuf,sizeof(adbuf));
}


/******************************************************************************
* Function       :ads1247_demo_test
* Author         :wujing	
* Date           :2013-3-4
* Description    :ads1247测试demo，同时给应用程序提供使用参考,
*                :ads1247_read_period_proc()需用户调用，建议放入任务while(1)循环中
*                :ads1247为24位ADC，单通道数据需要用4个Byte保存
*                :注：不能每次测试时都关闭、打开设备，这样可能读不到数据，因为ADS1247读取数据需要时间
* Calls          :drv_ioctl()、io_ctrl()、drv_read()、drv_open()、drv_close()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :
*******************************************************************************
* History:        2013-3-4
*------------------------------------------------------------------------------
* 2013-3-4 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint32_t ads1247_data[2] = { 0 };
void ads1247_demo_test(void)
{
//     if(0 != gt_glp.fads1247)//一般测试前设备都是打开的
//     {
//         drv_close(gt_glp.fads1247);
//         gt_glp.fads1247 = drv_open("ADS1247", 0, 0);  
//     }
    drv_read(gt_glp.fads1247, (char *)&ads1247_data, sizeof(ads1247_data));
}
/******************************************************************************
* Function       :usart_can_demo_test
* Author         :wujing	
* Date           :2013-3-4
* Description    :串口、CAN通信测试，同时给应用程序提供使用参考,
*                :注：测试方法为将收到的数据发送回去
* Calls          :drv_ioctl()、io_ctrl()、drv_read()、drv_open()、drv_close()
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :
*******************************************************************************
* History:        2013-3-4
*------------------------------------------------------------------------------
* 2013-3-4 : 1.0.0 : wujing
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t len = 0;
int32_t ret = 0;
char usart_can_buf[64]={0};
uint8_t can_buf[8]={1,2,3,4,5,6,7,8};
t_embf_canmsgrx rxmsg;
t_embf_canmsgrx txmsg;
void usart_can_demo_test(void)
{
    len = drv_read(gt_glp.fserial1,usart_can_buf,64);//串口1测试将收到的数据发送出去
    if (len > 0)
    {
        drv_write(gt_glp.fserial1,usart_can_buf,64);
    } 
    len = drv_read(gt_glp.fserial2,usart_can_buf,64);//串口2测试将收到的数据发送出去
    if (len > 0)
    {
        drv_write(gt_glp.fserial2,usart_can_buf,64);
    } 
    len = drv_read(gt_glp.fserial3,usart_can_buf,64);//串口3测试将收到的数据发送出去
    if (len > 0)
    {
        drv_write(gt_glp.fserial3,usart_can_buf,64);
    } 

    ret = drv_read(gt_glp.fdcan1,&rxmsg,sizeof(rxmsg));//CAN1测试
    if(ret >0)
    {
        t_embf_canmsgtx txmsg =
        {
            .StdId = 0x00,
            .ExtId = 0x01,
            .IDE = ((uint32_t)0x00000004),
            .RTR = ((uint32_t)0x00000000),
            .DLC = 8,
        };       
        memcpy(txmsg.Data,can_buf,8);
        drv_write(gt_glp.fdcan1,&txmsg,sizeof(t_embf_canmsgtx));
    }
    
    ret = drv_read(gt_glp.fdcan2,&rxmsg,sizeof(rxmsg));//CAN2测试
    if(ret >0)
    {
        t_embf_canmsgtx txmsg =
        {
            .StdId = 0x00,
            .ExtId = 0x1234,
            .IDE = ((uint32_t)0x00000004),
            .RTR = ((uint32_t)0x00000000),
            .DLC = 8,
        };       
        memcpy(txmsg.Data,can_buf,8);
        drv_write(gt_glp.fdcan2,&txmsg,sizeof(t_embf_canmsgtx));
    }   
}

