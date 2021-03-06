/****************************************************************************** 
* Copyright (C), 1997-2010, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :widget.c
* Author         :llemmx
* Date           :2011-05-03
* Description    :主要实现一些控件功能，如label,input,progress,arrow等
* Interface      :widget_setlcd - 选择对应的LCD屏幕
                  widget_label - 标签控件
                  widget_progress - 进度条控件
                  widget_arrow - 箭头控件
                  widget_input - 输入控件
                  widget_setlangcoo - 多语言坐标切换控件
* Others         :无
*******************************************************************************
* History:        
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 初稿
*------------------------------------------------------------------------------
*------------------------------------------------------------------------------
* 2012.09.19 : 1.0.1 : Linfei
* Modification   : 添加了IP输入控件，时间输入控件，普通输入控件扩展了符号以及小数
*                  点支持，添加了菜单项绘制控件
*------------------------------------------------------------------------------
******************************************************************************/
#include <stdio.h>
#include "widget.h"
#include "image.h"

uint32_t lg_langcoo[10];
TLCDModule *lg_LCD;
PProcBuf m_PMsgQ;

/******************************************************************************
* Function       :getfontbysize
* Author         :llemmx
* Date           :2011-05-03
* Description    :通过尺寸选择字体
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :size - 字体尺寸
                  lang - 语言
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint16_t getfontbysize(uint16_t size,uint8_t lang)
{
    uint16_t ret=0;
    
    switch(lang){
    case LANG_EN:
        if (size==FONT_SIZE_9)
            ret=FONT_ASCII6x12;
        else if (size==FONT_SIZE_11)
            ret=FONT_ASCII8x16;
        else if (size==FONT_SIZE_18)
            ret=FONT_ASCII12x24;
        else if (size==FONT_SIZE_22)
            ret=FONT_ASCII17x40;
        break;
    case LANG_ZH_CN:
        if (size==FONT_SIZE_9)
            ret=FONT_GB12x12;
        else if (size==FONT_SIZE_11)
            ret=FONT_GB16x16;
        else if (size==FONT_SIZE_18)
            ret=FONT_GB24x24;
        break;
    }
    return ret;
}

/******************************************************************************
* Function       :getstrbyzh
* Author         :llemmx
* Date           :2011-05-03
* Description    :语言映射
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :str - 字符串
                  lang - 语言
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
char *getstrbyzh(char *str,uint8_t lang)
{
    for (uint8_t i=0;i<STR_LAST+1 && lang!=LANG_ZH_CN;++i){
        if (strcmp(str,MenuStr_zh[i])==0){
            if (lang==LANG_EN)
                return (char*)MenuStr_en[i];
        }
    }
    return str;
}

/******************************************************************************
* Function       :widget_setlangcoo
* Author         :llemmx
* Date           :2011-05-03
* Description    :set new label coordinates,only once
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  lang - 语言
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void widget_setlangcoo(uint16_t x,uint16_t y,uint8_t lang)
{
    if (lang>=LANG_ZH_CN && lang<=LANG_DE){
        lg_langcoo[lang]=(x<<16) | y;
    }
}

/******************************************************************************
* Function       :widget_label
* Author         :llemmx
* Date           :2011-05-03
* Description    :标签控件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  str - 要显示的字符串
                  size - 字符串长度
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void widget_label(uint16_t x,uint16_t y,char *str,uint16_t size)
{
    char *stra;
    TLCDWStrParam l_lp;
    uint8_t current_lang = get_language();    
    stra=getstrbyzh(str,current_lang);
    l_lp.dat=stra;
    l_lp.len=strlen(stra);
    l_lp.size=size;
    l_lp.index=0;
    if (current_lang<=4){
        l_lp.code=LCD_FONT_EAST;
    }else{
        l_lp.code=LCD_FONT_WEST;
    }
    
    if (lg_langcoo[current_lang]>0){
        l_lp.x=(lg_langcoo[current_lang]>>16) & 0xFFFF;
        l_lp.y= lg_langcoo[current_lang]      & 0xFFFF;
        lg_LCD->write_str(&l_lp);
        lg_langcoo[current_lang]=0;
    }else{
        l_lp.x=x;
        l_lp.y=y;
        lg_LCD->write_str(&l_lp);
    }
}

/******************************************************************************
* Function       :widget_progress
* Author         :llemmx
* Date           :2011-05-03
* Description    :进度条控件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  w - 宽
                  key - 按键
* Output         :value - 进度条值
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void widget_progress(uint16_t x,uint16_t y,uint16_t w,uint16_t key,uint16_t *value,uint8_t jump)
{
    uint8_t flash=1;
    if (key==0 && *value<=100)
        return ;
    
    if ((*value)>100 && MKEY_UP==key){
        flash=0;
    }
    
    if (MKEY_DOWN==key && (*value)>0){
        if ((*value)>=jump){
            *value-=jump;
        }else{
            *value=0;
        }
    }else if (MKEY_UP==key && (*value)<100){
        *value+=jump;
        if (*value>100){
            *value=100;
        }
    }
    
    if (flash){
        lg_LCD->drawline(x,y,*value,0x28,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+1,*value,0x40,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+2,*value,0x58,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+3,*value,0x70,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+4,*value,0x88,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+5,*value,0xA0,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+6,*value,0xB8,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+7,*value,0xD0,LCD_DRAW_HORI);
        lg_LCD->drawline(x,y+8,*value,0xF8,LCD_DRAW_HORI);
    }

    //lg_LCD->drawline(x+(*value),y,9,0,LCD_DRAW_VERT);
    if (*value<100 && (100-*value)>=jump){
        lg_LCD->drawline(x+*value,y,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+1,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+2,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+3,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+4,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+5,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+6,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+7,jump+1,0,LCD_DRAW_HORI);
        lg_LCD->drawline(x+*value,y+8,jump+1,0,LCD_DRAW_HORI);
    }
}

/******************************************************************************
* Function       :widget_arrow
* Author         :llemmx
* Date           :2011-05-03
* Description    :指针控件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :ar - 指针结构体
* Return         :0表示跳转失败，其它未具体位置数据
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint8_t widget_arrow(TArrow *ar)
{
    if (ar->key==0)
        return 0;
    
    if (MKEY_DOWN==ar->key)
    {
        if(ar->dir == 1)
        {
            if (ar->value<ar->num)
            {
                lg_LCD->writestr(ar->x,ar->y+ar->w*(ar->value-1)," ",1,FONT_ASCII12x24);
                ++ar->value;
            }
            else
            {
                lg_LCD->writestr(ar->x,ar->y+ar->w*(ar->num-1)," ",1,FONT_ASCII12x24);
                ar->value=1;
            }
        }
        if(ar->dir == 2)
        {
            if (ar->value<ar->num)
            {
                lg_LCD->writestr(ar->x+ar->zw*((ar->value-1)%2),ar->y+ar->w*((ar->value-1)/2)," ",1,FONT_ASCII12x24);
                ++ar->value;
            }
            else
            {
                lg_LCD->writestr(ar->x+ar->zw*((ar->value-1)%2),ar->y+ar->w*((ar->value-1)/2)," ",1,FONT_ASCII12x24);
                ar->value=1;
            }
        }
    }
    else if (MKEY_UP==ar->key)
    {
        if(ar->dir == 1)
        {
            if (ar->value>1)
            {
                lg_LCD->writestr(ar->x,ar->y+ar->w*(ar->value-1)," ",1,FONT_ASCII12x24);
                --ar->value;
            }
            else
            {
                lg_LCD->writestr(ar->x,ar->y+ar->w*(ar->value-1)," ",1,FONT_ASCII12x24);
                ar->value=ar->num;
            }
        }
          
        if(ar->dir == 2)
        {
            if (ar->value>1)
            {
                lg_LCD->writestr(ar->x+ar->zw*((ar->value-1)%2),ar->y+ar->w*((ar->value-1)/2)," ",1,FONT_ASCII12x24);
                --ar->value;
            }
            else
            {
                lg_LCD->writestr(ar->x+ar->zw*((ar->value-1)%2),ar->y+ar->w*((ar->value-1)/2)," ",1,FONT_ASCII12x24);
                ar->value=ar->num;
            }
        }
    }
    else if (MKEY_ENTER==ar->key)
    {
        return ar->value;
    }
    else
    {
        return 0;
    }
    
    if(ar->dir == 1)
    {
       lg_LCD->showimage(ar->x,ar->y+ar->w*(ar->value-1),9,16,Image_Arrow,0,LCD_DRAW_GRAY);
    }
    
    if(ar->dir == 2)//z字形
    {
       
       lg_LCD->showimage(ar->x+ar->zw*((ar->value-1)%2),ar->y+ar->w*((ar->value-1)/2),9,16,Image_Arrow,0,LCD_DRAW_GRAY);
    }
    
    return 0;
}

/******************************************************************************
* Function       :xdigitvalue
* Author         :llemmx
* Date           :2011-05-03
* Description    :字符转bin格式数据
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :isdigit - 字符
* Return         :-1表示转换失败，其它为具体数值
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
static int32_t xdigitvalue(char isdigit)
{
    if (isdigit >= '0' && isdigit <= '9' )
        return isdigit - '0';
    if (isdigit >= 'a' && isdigit <= 'f')
        return 10 + isdigit - 'a';
    return -1;
}

/******************************************************************************
* Function       :digitvalue
* Author         :llemmx
* Date           :2011-05-03
* Description    :字符转bin格式数据
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :isdigit - 字符
* Return         :-1表示转换失败，其它为具体数值
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
static int32_t digitvalue(char isdigit)
{
    if (isdigit >= '0' && isdigit <= '9' )
        return isdigit - '0';
    else
        return -1;
}

/******************************************************************************
* Function       :strtou32
* Author         :llemmx
* Date           :2011-05-03
* Description    :字符串转32位整形
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :str - 字符串
* Output         :value  转换输出的值
* Return         :-1表示转换失败，0表示转换成功
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t strtou32(const char *str,uint32_t *value)
{
    int32_t i;

    *value = 0;

    if((str[0]=='0') && (str[1]=='x')) {
        /* hexadecimal mode */
        str += 2;
        
        while(*str != '\0') {
            if((i = xdigitvalue(*str)) < 0)
                return -1;
            
            *value = (*value << 4) | (unsigned int)i;

            str++;
        }
    } else {
        /* decimal mode */
        while(*str != '\0') {
            if((i = digitvalue(*str)) < 0)
                return -1;
            
            *value = (*value * 10) + (unsigned int)i;

            str++;
        }
    }

    return 0;
}

// 3位字符形式数字转16进制
uint16_t str3tohex(uint8_t *arr)
{
    uint16_t hex;
    hex=(arr[0] - '0')*100+(arr[1] - '0')*10+arr[2] - '0';
    return hex;
}


/******************************************************************************
* Function       :RunInput
* Author         :llemmx
* Date           :2011-05-03
* Description    :字符串转32位整形
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :msg - 菜单消息
* Output         :无
* Return         :0表示拦截传入的消息，其它数值表示消息将传递到另一层
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t RunInput(PMsg msg)
{
    char buf[32] = { 0 };
    int32_t tmp = 0;
    TInputBox *box=(TInputBox *)msg->Object;
    uint8_t width = strlen(box->buf);
    TLCDWStrParam l_lp;

    uint8_t current_lang = get_language();    
    
    if (current_lang<=4){
        l_lp.code=LCD_FONT_EAST;
    }else{
        l_lp.code=LCD_FONT_WEST;
    }
    
    switch(msg->Message){
    //秒计时
    case MSG_TYPE_TIMER:
        if (box->timeover)
        {
            if (!--box->timeover)
            {
                l_lp.x=box->x;
                l_lp.y=box->y;
                l_lp.dat=box->back;
                l_lp.len=width;
                l_lp.size=box->font;
                l_lp.index=0;
                lg_LCD->write_str(&l_lp);
                //lg_LCD->writestru(box->x,box->y,box->back,width,box->font);
                PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-3);
                return 1;
            }
        }
        sprintf(buf,"%s",box->buf);
        if (box->flash){
            buf[box->ptr]=' ';
        }
        l_lp.x=box->x;
        l_lp.y=box->y;
        l_lp.dat=buf;
        l_lp.len=width;
        l_lp.size=box->font;
        l_lp.index=box->ptr+1;
        lg_LCD->write_str(&l_lp);
        //lg_LCD->writestrug(box->x,box->y,buf,width,box->font,box->ptr+1);
        box->flash=!box->flash;
        break;
    case MSG_TYPE_BUTTON:
        //gt_glp.MenuTOut = MENU_TIMEOUT;
        if(IS_VALID_ACTION(msg->Param1) && IS_VALID_KEY(msg->Param2))
        {
            switch(msg->Param2)
            {
            case MKEY_DOWN:
                if (box->symbol && box->ptr==0)
                {
                    if (box->buf[0]=='+')
                    {
                        box->buf[0]='-';
                    }
                    else
                    {
                        box->buf[0]='+';
                    }
                }
                else
                {
                    if (box->buf[box->ptr]>='9')
                    {
                        box->buf[box->ptr]='0';
                    }
                    else
                    {
                        box->buf[box->ptr]++;
                    }
                }
                box->flash=0;
                break;
            case MKEY_RIGHT:
                box->ptr = (box->ptr + 1)%width;
                if(box->ptr == box->width - box->point + box->symbol)
                {
                    box->ptr = (box->ptr + 1)%width;
                }
                box->flash=0;
                break;
            case MKEY_ENTER:
                sprintf(buf,"%s",box->buf);
                if(box->point != 0)
                {
                    tmp = width - box->point - 1;
                    while(tmp < width - 1)
                    {
                        buf[tmp] = buf[tmp + 1];
                        tmp++;
                    }
                    buf[tmp] = 0;
                }
                strtou32(buf+box->symbol,(uint32_t *)&tmp);
                if(box->symbol && buf[0]=='-')
                {
                    tmp*=-1;
                }
                
                l_lp.x=box->x;
                l_lp.y=box->y;
                l_lp.len=width;
                l_lp.size=box->font;
                l_lp.index=0;
                if (tmp>=box->min && tmp<=box->max)
                {
                    box->def = tmp;
                    l_lp.dat=box->buf;
                    lg_LCD->write_str(&l_lp);
                    //lg_LCD->writestru(box->x, box->y, box->buf, width, box->font);
                    PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,1);
                    //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,1);
                }
                else
                {
                    l_lp.dat=box->back;
                    lg_LCD->write_str(&l_lp);
                    //lg_LCD->writestru(box->x, box->y, box->back, width, box->font);
                    PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-1);
                    //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-1);
                }
                return 1;
            case MKEY_ESC:
                l_lp.x=box->x;
                l_lp.y=box->y;
                l_lp.len=width;
                l_lp.size=box->font;
                l_lp.index=0;
                l_lp.dat=box->back;
                lg_LCD->write_str(&l_lp);
                //lg_LCD->writestru(box->x,box->y,box->back,width,box->font);
                PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-2);
                //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-2);
                return 1;
            default:
                box->flash=1;
            }
            sprintf(buf,"%s",box->buf);
            if (box->flash)
            {
                buf[box->ptr]=' ';
            }
            l_lp.x=box->x;
            l_lp.y=box->y;
            l_lp.len=width;
            l_lp.size=box->font;
            l_lp.index=box->ptr+1;
            l_lp.dat=buf;
            lg_LCD->write_str(&l_lp);
            //lg_LCD->writestrug(box->x,box->y,buf,width,box->font,box->ptr+1);
        }
        break;
    }
    msg->Message=0;
    return 0;
}

int32_t RunIPInput(PMsg msg)
{
    char buf[32] = { 0 };
    uint8_t i = 0;
    TLCDWStrParam l_lp;
    
    TIPInputBox *box=(TIPInputBox *)msg->Object;
    switch(msg->Message)
    {
    //秒计时
    case MSG_TYPE_TIMER:
        if (box->timeover)
        {
            if (!--box->timeover)
            {
                sprintf(box->buf,"%03d.%03d.%03d.%03d",box->ip[0], box->ip[1], box->ip[2], box->ip[3]);
                lg_LCD->writestr(box->x,box->y,box->buf,15,box->font);
                PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-3);
                //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-3);
                return 1;
            }
        }
        sprintf(buf,"%s",box->buf);
        if(box->flash)
        {
            buf[box->ptr + (box->ptr/3)]=' ';
        }
        l_lp.x=box->x;
        l_lp.y=box->y;
        l_lp.len=15;
        l_lp.size=box->font;
        l_lp.index=box->ptr + (box->ptr/3)+1;
        l_lp.dat=buf;
        lg_LCD->write_str(&l_lp);
        //lg_LCD->writestrug(box->x,box->y,buf,15,box->font,box->ptr + (box->ptr/3)+1);
        box->flash=!box->flash;
        break;
    case MSG_TYPE_BUTTON:
        //gt_glp.MenuTOut = MENU_TIMEOUT;
        if(IS_VALID_ACTION(msg->Param1) && IS_VALID_KEY(msg->Param2))
        {
            switch(msg->Param2)
            {
            case MKEY_DOWN:
                box->buf[box->ptr + (box->ptr/3)] = '0' + (box->buf[box->ptr + (box->ptr/3)] - '0' + 1)%10;
                box->flash=0;
                break;
            case MKEY_RIGHT:
                box->ptr = (box->ptr + 1)%12;
                box->flash=0;
                break;
            case MKEY_ENTER:
                for(i = 0; i < 4; i++)
                {
                    if(str3tohex((uint8_t *)&box->buf[i<<2]) > 255)
                    {
                        break;
                    }
                }
                if(i == 4)
                {
                    for(i = 0; i < 4; i++)
                    {
                        box->ip[i] = str3tohex((uint8_t *)&box->buf[i<<2]);
                    }
                    PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,1);
                    //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,1);
                }
                else
                {
                    // 出错保留原值
                    sprintf(box->buf,"%03d.%03d.%03d.%03d",box->ip[0], box->ip[1], box->ip[2], box->ip[3]);
                    PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-1);
                    //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-1);
                }
				l_lp.x=box->x;	   //2012.11.13 by wujing,use write_str() instead of writestr()  
		        l_lp.y=box->y;
		        l_lp.len=15;
		        l_lp.size=box->font;
		        l_lp.dat=box->buf;
		        lg_LCD->write_str(&l_lp);
                //lg_LCD->writestr(box->x,box->y,box->buf,15,box->font);
                return 1;
            case MKEY_ESC:
                sprintf(box->buf,"%03d.%03d.%03d.%03d",box->ip[0], box->ip[1], box->ip[2], box->ip[3]);
				l_lp.x=box->x;	   //2012.11.13 by wujing,use write_str() instead of writestr()  
		        l_lp.y=box->y;
		        l_lp.len=15;
		        l_lp.size=box->font;
		        l_lp.dat=box->buf;
		        lg_LCD->write_str(&l_lp);
                //lg_LCD->writestr(box->x,box->y,box->buf,15,box->font);
                PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-2);
                //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-2);
                return 1;
            default:
                box->flash=1;
            }
            sprintf(buf,"%s",box->buf);
            if (box->flash)
            {
                buf[box->ptr]=' ';
            }
            l_lp.x=box->x;
            l_lp.y=box->y;
            l_lp.len=15;
            l_lp.size=box->font;
            l_lp.index=box->ptr + (box->ptr/3)+1;
            l_lp.dat=buf;
            lg_LCD->write_str(&l_lp);
            //lg_LCD->writestrug(box->x,box->y,buf,15,box->font,box->ptr + (box->ptr/3)+1);
        }
        break;
    }
    msg->Message=0;
    return 0;
}

// 时间合法性检测
// mark：2=年月日，1=时分秒，3=年月日时分秒全部检查
// time格式：年月日时分秒
bool checktime(uint8_t *time, uint8_t mark)
{
    if(mark == 0 || mark > 3)
    {
        return false;
    }
    // 年月日检测
    if((mark & 2) != 0)
    {
        if(time[0] > 99)                   //0~99
        {
            return false;
        }
        if((time[1] > 12) || (time[1] == 0))     //1~12
        {
            return false;
        }
        if(time[2] == 0)                   //1~31
        {
            return false;
        }
        //月大31天，2月正常28天，遇润年29天，其余月小30天
        if (time[1] == 1 || time[1] == 3 || \
            time[1] == 5 || time[1] == 7 || \
            time[1] == 8 || time[1] == 10 || \
            time[1] == 12)
        {
            if (time[2] > 31)
            {
                return false;
            }
        }
        
        if (time[1] == 4 || time[1] == 6 || \
            time[1] == 9 || time[1] == 11)
        {
            if (time[2] > 30)
            {
                return false;
            }
        }
        
        if (time[1] == 2)
        {
            if (time[0] % 4 == 0 && time[2] > 29)
            {
                return false;
            }
            if (time[0] % 4 != 0 && time[2] > 28)
            {
                return false;
            }
        }
    }
    
    if((mark & 1) != 0)
    {
        if(time[3] > 23)                   //0~23
        {
            return false;
        }
        if(time[4] > 59)                   //0~59
        {
            return false;
        }
        if(time[5] > 59)                   //0~59
        {
            return false;
        }
    }
    return true;
}

int32_t RunTimeInput(PMsg msg)
{
    char buf[32] = { 0 };
    uint8_t i = 0;
    TLCDWStrParam l_lp;
    
    TTInputBox *box=(TTInputBox *)msg->Object;
    uint8_t width = strlen(box->buf);
    
    switch(msg->Message)
    {
    //秒计时
    case MSG_TYPE_TIMER:
        if (box->timeover)
        {
            if (!--box->timeover)
            {
                if(box->mark == 2)
                {
                    sprintf(box->buf,"%02d/%02d/%02d",box->time[0], box->time[1], box->time[2]);
                }
                else
                {
                    sprintf(box->buf,"%02d:%02d:%02d",box->time[0], box->time[1], box->time[2]);
                }
                lg_LCD->writestr(box->x,box->y,box->buf,width,box->font);
                PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-3);
                //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-3);
                return 1;
            }
        }
        sprintf(buf,"%s",box->buf);
        if(box->flash)
        {
            buf[box->ptr]=' ';
        }
        l_lp.x=box->x;
        l_lp.y=box->y;
        l_lp.len=width;
        l_lp.size=box->font;
        l_lp.index=box->ptr+1;
        l_lp.dat=buf;
        lg_LCD->write_str(&l_lp);
        //lg_LCD->writestrug(box->x,box->y,buf,width,box->font,box->ptr+1);
        box->flash=!box->flash;
        break;
    case MSG_TYPE_BUTTON:
        //gt_glp.MenuTOut = MENU_TIMEOUT;
        if(IS_VALID_ACTION(msg->Param1) && IS_VALID_KEY(msg->Param2))
        {
            switch(msg->Param2)
            {
            case MKEY_DOWN:
                box->buf[box->ptr] = '0' + (box->buf[box->ptr] - '0' + 1)%10;
                box->flash=0;
                break;
            case MKEY_RIGHT:
                box->ptr = (box->ptr + 1)%width;
                if((box->ptr + 1) % 3 == 0 && box->ptr != width - 1)
                {
                    box->ptr = (box->ptr + 1)%width;
                }
                box->flash=0;
                break;
            case MKEY_ENTER:
                if(box->mark == 2)
                {
                    i = 0;
                }
                else
                {
                    i = 3;
                }
                buf[i] = (box->buf[0] - '0')*10 + box->buf[1] - '0';
                buf[i + 1] = (box->buf[3] - '0')*10 + box->buf[4] - '0';
                buf[i + 2] = (box->buf[6] - '0')*10 + box->buf[7] - '0';
                if(checktime((uint8_t *)buf, box->mark) == true)
                {
                    box->time[0] = buf[i];
                    box->time[1] = buf[i + 1];
                    box->time[2] = buf[i + 2];
                    PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,1);
                    //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,1);
                }
                else
                {
                    // 出错保留原值
                    if(box->mark == 2)
                    {
                        sprintf(box->buf,"%02d/%02d/%02d",box->time[0], box->time[1], box->time[2]);
                    }
                    else
                    {
                        sprintf(box->buf,"%02d:%02d:%02d",box->time[0], box->time[1], box->time[2]);
                    }
                    PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-1);
                    //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-1);
                }
                lg_LCD->writestr(box->x,box->y,box->buf,width,box->font);
                return 1;
            case MKEY_ESC:
                if(box->mark == 2)
                {
                    sprintf(box->buf,"%02d/%02d/%02d",box->time[0], box->time[1], box->time[2]);
                }
                else
                {
                    sprintf(box->buf,"%02d:%02d:%02d",box->time[0], box->time[1], box->time[2]);
                }
                lg_LCD->writestr(box->x,box->y,box->buf,width,box->font);
                PostMessage(m_PMsgQ,MSG_TYPE_CMD,News_Exit_Input,-2);
                //ConstructMSG(msg,MSG_TYPE_CMD,News_Exit_Input,-2);
                return 1;
            default:
                box->flash=1;
            }
            sprintf(buf,"%s",box->buf);
            if (box->flash)
            {
                buf[box->ptr]=' ';
            }
            l_lp.x=box->x;
            l_lp.y=box->y;
            l_lp.len=strlen(buf);
            l_lp.size=box->font;
            l_lp.index=box->ptr+1;
            l_lp.dat=buf;
            lg_LCD->write_str(&l_lp);
            //lg_LCD->writestrug(box->x,box->y,buf,strlen(buf),box->font,box->ptr+1);
        }
        break;
    }
    msg->Message=0;
    return 0;
}

/******************************************************************************
* Function       :widget_input
* Author         :llemmx
* Date           :2011-05-03
* Description    :输入控件
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :box - 输入控制结构体
* Output         :无
* Return         :返回消息推出值
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
int32_t widget_input(TInputBox *box)
{
    int idrun,ret=0;
    char fbuf[16];
    TLCDWStrParam l_lp;
    

    if(box->point == 0)
    {
        if(box->symbol)
        {
            sprintf(fbuf, "%%+0%dd", box->width + 1);
        }
        else
        {
            sprintf(fbuf, "%%0%dd", box->width);
        }
        sprintf(box->buf,fbuf,box->def);
    }
    else
    {
        if(box->symbol)
        {
            sprintf(fbuf, "%%+0%d.%df", box->width + 2, box->point);
        }
        else
        {
            sprintf(fbuf, "%%0%d.%df", box->width + 1, box->point);
        }
        switch(box->point)
        {
        case 1:
            sprintf(box->buf,fbuf,box->def / 10.0);
            break;
        case 2:
            sprintf(box->buf,fbuf,box->def / 100.0);
            break;
        case 3:
            sprintf(box->buf,fbuf,box->def / 1000.0);
            break;
        case 4:
            sprintf(box->buf,fbuf,box->def / 10000.0);
            break;
        default:
            return -1;
        }
    }
    sprintf(box->back, "%s", box->buf);
    
    l_lp.x=box->x;
    l_lp.y=box->y;
    l_lp.len=box->width+2;
    l_lp.size=box->font;
    l_lp.index=box->ptr+1;
    l_lp.dat=box->buf;
    lg_LCD->write_str(&l_lp);
    //lg_LCD->writestrug(box->x,box->y,box->buf,box->width+2,box->font,box->ptr+1);
    idrun=RegMsgProc(m_PMsgQ,RunInput);
    ret=DoMsgProcess(m_PMsgQ,box,News_Exit_Input);
    UnRegMsgProc(m_PMsgQ,idrun);
    //lg_LCD->setfontcolor(0xFF,0x00);
    return ret;
}

// IP地址输入控件
int32_t widget_ipinput(TIPInputBox *box)
{
    int idrun,ret=0;
    TLCDWStrParam l_lp;
    
    sprintf(box->buf,"%03d.%03d.%03d.%03d",box->ip[0], box->ip[1], box->ip[2], box->ip[3]);
    l_lp.x=box->x;
    l_lp.y=box->y;
    l_lp.len=15;
    l_lp.size=box->font;
    l_lp.index=box->ptr+1;
    l_lp.dat=box->buf;
    lg_LCD->write_str(&l_lp);
    //lg_LCD->writestrug(box->x,box->y,box->buf,15,box->font,box->ptr+1);
    idrun=RegMsgProc(m_PMsgQ,RunIPInput);
    ret=DoMsgProcess(m_PMsgQ,box,News_Exit_Input);
    UnRegMsgProc(m_PMsgQ,idrun);
    //lg_LCD->setfontcolor(0xFF,0x00);
    return ret;
}

// Time输入控件
int32_t widget_timeinput(TTInputBox *box)
{
    int idrun,ret=0;
    TLCDWStrParam l_lp;
    
    if(box->mark == 2)
    {
        sprintf(box->buf,"%02d/%02d/%02d",box->time[0], box->time[1], box->time[2]);
    }
    else if(box->mark == 1)
    {
        sprintf(box->buf,"%02d:%02d:%02d",box->time[0], box->time[1], box->time[2]);
    }
    else
    {
        return ret;
    }
    l_lp.x=box->x;
    l_lp.y=box->y;
    l_lp.len=strlen(box->buf);
    l_lp.size=box->font;
    l_lp.index=box->ptr+1;
    l_lp.dat=box->buf;
    lg_LCD->write_str(&l_lp);
    //lg_LCD->writestrug(box->x,box->y,box->buf,strlen(box->buf),box->font,box->ptr+1);
    idrun=RegMsgProc(m_PMsgQ,RunTimeInput);
    ret=DoMsgProcess(m_PMsgQ,box,News_Exit_Input);
    UnRegMsgProc(m_PMsgQ,idrun);
    //lg_LCD->setfontcolor(0xFF,0x00);
    return ret;
}



// 菜单项绘制控件
void widget_ItemSelect(MItem *mitem)
{
    uint8_t i;
    if(mitem == NULL)
    {
        return;
    }
    if(mitem->item == NULL || mitem->cur >= mitem->num)
    {
        return;
    }
    
    if(mitem->init == 1)
    {
        lg_LCD->drawrectarc(mitem->x - 10, 
                mitem->y - 2 + mitem->cur*mitem->dis,
                mitem->rect_w, mitem->rect_h, 0x60, 1);
        for(i = 0; i < mitem->num; i++)
        {
            if(i == mitem->cur)
            {
                lg_LCD->setfontcolor(0xff, 0x60);
            }
            widget_label(mitem->x, mitem->y + i*mitem->dis, 
                (char *)MenuStr_zh[mitem->item[i]], mitem->font);
            if(i == mitem->cur)
            {
                lg_LCD->setfontcolor(0xff, 0x00);
            }
        }
    }
    else
    {
        i = mitem->cur;
        if(i > 0)
        {
            i--;
        }
        else
        {
            i = mitem->num - 1;
        }
        lg_LCD->drawrectarc(mitem->x - 10, 
                mitem->y - 2 + i*mitem->dis,
                mitem->rect_w, mitem->rect_h, 0, 1);
        widget_label(mitem->x, mitem->y + i*mitem->dis, 
            (char *)MenuStr_zh[mitem->item[i]], mitem->font);
        lg_LCD->drawrectarc(mitem->x - 10, 
                mitem->y - 2 + mitem->cur*mitem->dis,
                mitem->rect_w, mitem->rect_h, mitem->bcolor, 1);
        lg_LCD->setfontcolor(0xff, mitem->bcolor);
        widget_label(mitem->x, mitem->y + mitem->cur*mitem->dis, 
            (char *)MenuStr_zh[mitem->item[mitem->cur]], mitem->font);
        lg_LCD->setfontcolor(0xff, 0x00);
    }
}

/******************************************************************************
* Function       :widget_setlcd
* Author         :llemmx
* Date           :2011-05-03
* Description    :选择需要操作的lcd
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :lcd - 实例
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void widget_setlcd(PProcBuf pb,void *lcd)
{
    lg_LCD=(TLCDModule*)lcd;
    m_PMsgQ=pb;
}
