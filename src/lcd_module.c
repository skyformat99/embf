/****************************************************************************** 
* Copyright (C), 1997-2010, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :lcd_module.c
* Author         :llemmx
* Date           :2011-05-03
* Description    :本模块主要实现液晶的通用绘图层，将液晶驱动独立出来。
* Interface      :clear - 清屏
                  cleararea - 区域清屏
                  setbacklight - 软件对比度调节
                  writechar - 写字符
                  writestr - 写ascii字符串
                  writestru - 写混合语言字符串
                  showimage - 显示图片
                  drawline - 画直线
                  drawrect - 画矩形
                  drawrectarc - 画圆角矩形
                  setfontcolor - 设置字体颜色
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include "lcd_module.h"

#define LCD_DRV_SUPPORT

#define LCD_CMD 0x01
#define LCD_DAT 0x02

uint8_t g_front=0xFFU,g_backend=0x00U;   //wujing 2012.11.23 for MISRA-2004
void LCD_ClearRAM(void);
void LCD_ClearArea(uint8_t x,uint8_t y,uint8_t width,uint8_t height,uint8_t gray);
uint16_t FindFont4Str(const char dat[],uint16_t font);
void LCD_setfontcolor(uint8_t front,uint8_t backend);
void LCD_render(uint8_t w,uint8_t h,uint8_t data[]);
void LCD_render1(uint8_t w,uint8_t h,uint8_t data[]);
void LCD_writechar(uint16_t row,uint16_t col,const char dat[],uint16_t font);
void LCD_writestr(uint16_t row,uint16_t col,const char dat[],uint8_t len,uint16_t font);
void LCD_getfontbysize(uint16_t size,uint16_t *font,uint8_t fontid);
void LCD_writestru(uint16_t x,uint16_t y,const char dat[],uint8_t len,uint8_t size);
void LCD_write_str(TLCDWStrParam *param);
void LCD_drawline(uint8_t x,uint8_t y,uint8_t len,uint8_t gray,uint8_t flag);
void LCD_drawrect(uint8_t x,uint8_t y,uint8_t w,uint8_t h);
void LCD_drawrectarc(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t sd, uint8_t solid);
void LCD_showimage(uint8_t row,uint8_t col,uint8_t width,uint8_t high,const uint8_t *bmp,uint8_t gray,uint8_t flag);
void LCD_setbacklight(uint16_t dat);
/******************************************************************************
* Function       :LCD_ClearRAM
* Author         :llemmx
* Date           :2011-05-03
* Description    :清屏
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :无
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_ClearRAM(void)
{
    (void)drv_ioctl(gt_glp.flcd, (int32_t)LM_CLR_SCREEN, NULL);
}

/******************************************************************************
* Function       :LCD_ClearArea
* Author         :llemmx
* Date           :2011-05-03
* Description    :区域清屏
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 液晶屏幕的列坐标
                  y - 液晶屏幕的行坐标
                  width - 需要清屏的宽度
                  height - 需要清屏的高度
                  gray - 清除后屏幕的灰度
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_ClearArea(uint8_t x,uint8_t y,uint8_t width,uint8_t height,uint8_t gray)
{
	GData gdat;  //wujing 2012.11.23 change for failure of compling c file by C++ TEST
	gdat.x = x;
	gdat.y = y;
	gdat.width = width;
	gdat.height = height;
	gdat.gray = gray;
	gdat.dat = NULL;

    (void)drv_write(gt_glp.flcd, &gdat, width*height);
}

/******************************************************************************
* Function       :FindFont4Str
* Author         :llemmx
* Date           :2011-05-03
* Description    :通过字体确定字库指针
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :font - 字体
* Output         :dat - 字体指针
* Return         :如果找到相关字体则返回其索引，否则返回0xFFFF
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint16_t FindFont4Str(const char dat[],uint16_t font)
{
    const TFont *l_font;
    uint16_t ret,font_size,right;
    uint16_t pos,left;
    
    uint32_t idx,l_id;
    
    if (FONT_GB12x12==font){
        l_font=TFGB12x12;
    }else if (FONT_GB16x16==font){
        l_font=TFGB16x16;
    }else if (FONT_GB24x24==font){
        l_font=TFGB24x24;
    }else if (FONT_UTF82B5x8==font){
        l_font=TFU2S5x8;
    }else if (FONT_UTF82B8x16==font){
        l_font=TFU2S8x16;
    }else if (FONT_UTF82B12x24==font){
        l_font=TFU2S12x24;
    }else if (FONT_UTF82B6x12==font){
        l_font=TFU2S6x12;
    }else if (FONT_UTF82B17x40==font){
        l_font=TFU2S17x40;
    }else{
        l_font=NULL;
    }
    
    if (l_font!=NULL){
        font_size=getfontsize(font);
        pos=0;
        left=0;
        idx=0;
        right= (font_size == 0 ? 0 : font_size - 1);
        
        l_id=(dat[0]<<8) | dat[1];        
        do{
            if (left>right)
            {
                ret=0;
                break;
            }            
            pos=(left+right)/2;
            idx=(l_font[pos].name[0]<<8) | l_font[pos].name[1];
            
            if (idx>l_id)
            {
                right= (pos > 0 ? pos - 1 : 0);
                if(right == 0 && left == 0)
                {
                    idx=(l_font[0].name[0]<<8) | l_font[0].name[1];
                    break;
                }
            }
            else
            {
                left=pos+1;
            }
        }while(l_id!=idx);
        
        if (l_id==idx)
        {
            ret=pos;
        }
        else
        {
            ret=0;
        }
    }else{
        ret=0;
    }
    
    return ret;
}
/******************************************************************************
* Function       :LCD_setfontcolor
* Author         :llemmx
* Date           :2011-05-03
* Description    :设置字体颜色
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :front - 设置字体前景色
                  backend - 设置字体背景色
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_setfontcolor(uint8_t front,uint8_t backend)
{
    g_front=front;
    g_backend=backend;
}

/******************************************************************************
* Function       :LCD_render
* Author         :llemmx
* Date           :2011-05-03
* Description    :字体抗锯齿渲染
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :w - 宽
                  h - 高
* Output         :data - 需要渲染的字体数据
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_render(uint8_t w,uint8_t h,uint8_t data[])
{
    uint8_t i,j;
    uint8_t tlx,tly,trix,triy;//,blx,bly,brx,bry;
    uint8_t brender;
    
    for (i=1U;i<(h-1U);++i){
        for (j=1U;j<(w-1U);++j){
            if ((data[(i*w)+j]==0xFFU) || (data[(i*w)+j]==170U)){
                tlx=j-1U;
                tly=i-1U;
                trix=j+1U;
                triy=i-1U;
                /*blx=j-1;
                bly=i+1;
                brx=j+1;
                bry=i+1;*/
                if ((data[(tly*w)+tlx]==0xFFU) || (data[(tly*w)+tlx]==170U)){
                    brender=0U;
                    if (data[((i-1U)*w)+j]==0U){
                        data[((i-1U)*w)+j]=75U;
                        brender=1U;
                    }
                    if (data[((i*w)+j)-1U]==0U){
                        data[((i*w)+j)-1U]=75U;
                        brender=1U;
                    }
                    
                    if (brender != 0U){
                        data[(i*w)+j]=170U;
                        data[(tly*w)+tlx]=170U;
                    }
                }
                
                if ((data[(triy*w)+trix]==0xFFU) || (data[(triy*w)+trix]==170U)){
                    brender=0U;
                    if (data[((i-1U)*w)+j]==0U){
                        data[((i-1U)*w)+j]=75U;
                        brender=1U;
                    }
                    if (data[((i*w)+j)+1U]==0U){
                        data[((i*w)+j)+1U]=75U;
                        brender=1U;
                    }
                    
                    if (brender != 0U){
                        data[(i*w)+j]=170U;
                        data[(triy*w)+trix]=170U;
                    }
                }
                
                /*if (data[bly*w+blx]==0xFF){
                    brender=0;
                    if (data[i*w+j-1]==0){
                        data[i*w+j-1]=100;
                        brender=1;
                    }
                    if (data[(i+1)*w+j]==0){
                        data[(i+1)*w+j]=100;
                        brender=1;
                    }
                    
                    if (brender){
                        data[i*w+j]=150;
                        data[bly*w+blx]=150;
                    }
                }
                
                if (data[brx*w+bry]==0xFF){
                    brender=0;
                    if (data[i*w+j+1]==0){
                        data[i*w+j+1]=100;
                        brender=1;
                    }
                    if (data[(i+1)*w+j]==0){
                        data[(i+1)*w+j]=100;
                        brender=1;
                    }
                    
                    if (brender){
                        data[i*w+j]=150;
                        data[brx*w+bry]=150;
                    }
                }*/
            }
        }
    }
}

/******************************************************************************
* Function       :LCD_render1
* Author         :llemmx
* Date           :2011-05-03
* Description    :字体抗锯齿渲染 - 二级渲染
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :front - 设置字体前景色
                  backend - 设置字体背景色
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_render1(uint8_t w,uint8_t h,uint8_t data[])
{
    uint8_t i,j;
    uint8_t tlx,tly,trix,triy;
    //uint8_t brender;
    
    for (i=1U;i<(h-1U);++i){
        for (j=1U;j<(w-1U);++j){
            if (data[(i*w)+j]==170U){
                tlx=j-1U;
                tly=i-1U;
                trix=j+1U;
                triy=i-1U;
                if (data[(tly*w)+tlx]==75U){
                    //brender=0;
                    if (data[((i-1U)*w)+j]==0U){
                        data[((i-1U)*w)+j]=64U;
                        //brender=1;
                    }
                    if (data[((i*w)+j)-1U]==0U){
                        data[((i*w)+j)-1U]=64U;
                        //brender=1;
                    }
                }
                
                if (data[(triy*w)+trix]==75U){
                    //brender=0;
                    if (data[((i-1U)*w)+j]==0U){
                        data[((i-1U)*w)+j]=64U;
                        //brender=1;
                    }
                    if (data[(i*w)+j+1U]==0U){
                        data[(i*w)+j+1U]=64U;
                        //brender=1;
                    }
                }
                
                if ((data[((i-1U)*w)+j]==170U) && (data[((i*w)+j)-1U]==170U)){
                    if ((data[((i-2U)*w)+j]<64U) && (data[((i*w)+j)-2U]<64U))
                    {
                        data[(i*w)+j]=120U;
                    }
                }
                if ((data[((i-1U)*w)+j]==170U) && (data[((i*w)+j)+1U]==170U)){
                    if ((data[((i-2U)*w)+j]<64U) && (data[(i*w)+j+2U]<64U))
                    {
                        data[(i*w)+j]=120U;
                    }
                }
                if ((data[((i+1U)*w)+j]==170U) && (data[((i*w)+j)-1U]==170U)){
                    if ((data[((i+2U)*w)+j]<64U) && (data[((i*w)+j)-2U]<64U))
                    {
                        data[(i*w)+j]=120U;
                    }
                }
                if ((data[((i+1U)*w)+j]==170U) && (data[(i*w)+j+1U]==170U)){
                    if ((data[((i+2U)*w)+j]<64U) && (data[(i*w)+j+2U]<64U))
                    {
                        data[(i*w)+j]=120U;
                    }
                }
            }
        }
    }
}

uint8_t g_fbuf[700];
/******************************************************************************
* Function       :LCD_writechar
* Author         :llemmx
* Date           :2011-05-03
* Description    :向指定位置写一个字符
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :row - 行
                  col - 列
                  dat - 字符数据
                  font - 字体
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_writechar(uint16_t row,uint16_t col,const char dat[],uint16_t font)
{
	uint8_t err_flag = 0U;// add for MISRA-2004
    uint8_t p,w,h,p1,w1,c,revise=0U;
    uint16_t index,y1;
    const char *farr=NULL;
    uint8_t render;

    if ((dat!=NULL) && (font<=FONT_LAST))
    {
    
		memset(g_fbuf,(int32_t)g_backend,700U);

		index=(uint16_t)dat[0]-0x20U;
		render=0U;
		switch(font){

		case FONT_ASCII12x24:
			index=index*36U;
			farr=(char*)ASCII_12x24;
			render=1U;
			break;
		case FONT_ASCII8x16:
			index=(uint16_t)(index<<4);
			farr=(char*)ASCII_8x16;
			revise=1U;
			render=1U;
			break;
		case FONT_ASCII5x8:
			index*=5U;
			farr=(char*)ASCII_5x8;
			break;
		case FONT_ASCII6x12:
			index*=12U;
			farr=(char*)ASCII_6x12;
			break;
		case FONT_ASCII17x40:
			index*=85U;
			farr=(char*)ASCII_17x40;
			render=1U;
			break;
		case FONT_GB12x12:
			index=FindFont4Str(dat,FONT_GB12x12);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFGB12x12[index].data;
			index=0U;
			break;
		case FONT_GB16x16:
			index=FindFont4Str(dat,FONT_GB16x16);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFGB16x16[index].data;
			index=0U;
			break;
		case FONT_GB24x24:
			index=FindFont4Str(dat,FONT_GB24x24);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFGB24x24[index].data;
			index=0U;
			break;
		case FONT_UTF82B5x8:
			index=FindFont4Str(dat,FONT_UTF82B5x8);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFU2S5x8[index].data;
			index=0U;
			break;
		case FONT_UTF82B8x16:
			index=FindFont4Str(dat,FONT_UTF82B8x16);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFU2S8x16[index].data;
			index=0U;
			break;
		case FONT_UTF82B12x24:
			index=FindFont4Str(dat,FONT_UTF82B12x24);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFU2S12x24[index].data;
			index=0U;
			break;
		case FONT_UTF82B6x12:
			index=FindFont4Str(dat,FONT_UTF82B6x12);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFU2S6x12[index].data;
			index=0U;
			break;
		case FONT_UTF82B17x40:
			index=FindFont4Str(dat,FONT_UTF82B17x40);
			if (0xFFFFU==index)
			{
				err_flag = 1U;
				break ;
			}
			farr=TFU2S17x40[index].data;
			index=0U;
			break;
		default:
			p1=0U;
			break;
		}
		if(0U == err_flag)
		{
			p1=(FontInfo[font] & 0xFFU)>>3;
			if ((FontInfo[font] & 7U)!=0U)
			{
				++p1;
			}
			w1=(FontInfo[font]>>8) & 0xFFU;

			for (p=0U;p<p1;++p){
				for (w=0U;w<w1;++w){
					y1=index+((uint16_t)p*(uint16_t)w1)+(uint16_t)w;
					c=(uint8_t)farr[y1];
					for (h=0U;h<8U;++h){
						y1=(((uint16_t)((uint16_t)p<<3)+h)*(w1+revise))+w;
						g_fbuf[y1]=(((uint8_t)(c<<h)&0x80U)!=0U) ? g_front : g_backend;
					}
				}
			}

			w=((FontInfo[font]>>8) & 0xFFU)+revise;
			h=FontInfo[font] & 0xFFU;
			if (render != 0U){
				LCD_render(w,h,g_fbuf);//render
				LCD_render1(w,h,g_fbuf);//render
			}

			GData gdat;  //wujing 2012.11.23 change for failure of compling c file by C++ TEST
			gdat.x = row;
			gdat.y = col;
			gdat.width = w;
			gdat.height = h;
			gdat.gray = (uint8_t)NULL;
			gdat.dat = g_fbuf;

			(void)drv_write(gt_glp.flcd, &gdat, gdat.width*gdat.height);
		}
	}
}

/******************************************************************************
* Function       :LCD_writestr
* Author         :llemmx
* Date           :2011-05-03
* Description    :写字符串
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :row - 行
                  col - 列
                  dat - 字符数据
                  len - 字符串长度
				  font - 字体
* Output         :无
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_writestr(uint16_t row,uint16_t col,const char dat[],uint8_t len,uint16_t font)
{
    uint8_t o=0U,i,f=0U;
    o=(FontInfo[font]>>8) & 0xFFU;
    f= FontInfo[font]     & 0xFFU;
    if (o==f)
    {
        f=2U;
    }
    else
    {
        f=1U;
    }
    if ((font==FONT_ASCII5x8) || (font==FONT_ASCII8x16))
    {
        ++o;
    }
    for (i=0U;i<len;i+=f){
        if (dat[i]==(char)'\0')
        {
            break ;
        }
        LCD_writechar(row,col,&dat[i],font);
        row+=o;
    }
}

/******************************************************************************
* Function       :LCD_getfontbysize
* Author         :llemmx
* Date           :2011-05-03
* Description    :写字符串
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :size - 字体尺寸
                  fontid - 语言
* Output         :font - 字库指针
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_getfontbysize(uint16_t size,uint16_t *font,uint8_t fontid)
{
    switch(fontid){
    case FONT_EN:
        if (size==FONT_SIZE_6){
            *font=FONT_ASCII5x8;
        }else if (size==FONT_SIZE_9){
            *font=FONT_ASCII6x12;
        }else if (size==FONT_SIZE_11){
            *font=FONT_ASCII8x16;
        }else if (size==FONT_SIZE_18){
            *font=FONT_ASCII12x24;
        }else if (size==FONT_SIZE_22){
            *font=FONT_ASCII17x40;
        }else{// do nothing for MISRA-2004
        }
        break;
    case FONT_UTF2:
        if (FONT_SIZE_6==size){
            *font=FONT_UTF82B5x8;
        }else if (FONT_SIZE_9==size){
            *font=FONT_UTF82B6x12;
        }else if (FONT_SIZE_11==size){
            *font=FONT_UTF82B8x16;
        }else if (FONT_SIZE_18==size){
            *font=FONT_UTF82B12x24;
        }else if (FONT_SIZE_22==size){
            *font=FONT_UTF82B17x40;
        }else{// do nothing for MISRA-2004
        }
        break;
    case FONT_ZH_CN:
        if (size==FONT_SIZE_9){
            *font=FONT_GB12x12;
        }else if (size==FONT_SIZE_11){
            *font=FONT_GB16x16;
        }else if (size==FONT_SIZE_18){
            *font=FONT_GB24x24;
        }
        else{// do nothing for MISRA-2004
        }
        break;
    default:
        break;
    }
}

/******************************************************************************
* Function       :LCD_writestru
* Author         :llemmx
* Date           :2011-05-03
* Description    :显示混合语言字符串
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  dat - 字符串
                  len - 字符串长度
                  size - 字体尺寸
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_writestru(uint16_t x,uint16_t y,const char dat[],uint8_t len,uint8_t size)
{
    uint8_t  i,fontid;
    uint16_t font;
    uint16_t add;
    
    for (i=0U;i<len;){
        if (dat[i]==(char)'\0')
        {
            break ;
        }
        //字符识别
        if ((len-i)>1U){//识别混合字符
            if ((dat[i]>0x9FU) && (dat[i+1U]>0x9FU)){//中文(A0H~FEH)
                add=2U;
                fontid=FONT_ZH_CN;
            }else{//英文
                add=1U;
                fontid=FONT_EN;
            }
        }else{//识别单字节字体
            add=1U;
            fontid=FONT_EN;
        }
        LCD_getfontbysize((uint16_t)size,&font,fontid);
        LCD_writechar(x,y,&dat[i],font);
        x+=(FontInfo[font]>>8) & 0xFFU;
        if ((font==FONT_ASCII5x8) || (font==FONT_ASCII8x16))
        {
            ++x;
        }
        i+=add;
    }
}

/******************************************************************************
* Function       :LCD_writestrug
* Author         :llemmx
* Date           :2011-05-03
* Description    :支持混合语言字符串反显
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  dat - 字符串
                  len - 字符串长度
                  size - 字体尺寸
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
#if 0
void LCD_writestrug(uint16_t x,uint16_t y,const char *dat,uint8_t len,uint8_t size,uint8_t index)
{
    uint8_t  i,fontid=0,bf=0,bg=0;
    uint16_t font;
    uint16_t add;
    
    for (i=0;i<len;){
        if (dat[i]=='\0')
            return ;
        //字符识别
        if ((len-i)>1){//识别混合字符
            if (dat[i]>0x9F && dat[i+1]>0x9F){//中文(A0H~FEH)
                add=2;
                fontid=FONT_ZH_CN;
            }else{//英文
                add=1;
                fontid=FONT_EN;
            }
        }else{//识别单字节字体
            add=1;
            fontid=FONT_EN;
        }
        LCD_getfontbysize(size,&font,fontid);
        if (index!=0 && (i+1)==index){
            bf=g_front;
            bg=g_backend;
            g_front=g_backend;
            g_backend=bf;
        }
        LCD_writechar(x,y,dat+i,font);
        if (index!=0 && (i+1)==index){
            g_front=bf;
            g_backend=bg;
        }
        x+=(FontInfo[font]>>8) & 0xFF;
        if (font==FONT_ASCII5x8 || font==FONT_ASCII8x16 || font==FONT_UTF82B5x8)
            ++x;
        i+=add;
    }
}
#endif
/******************************************************************************
* Function       :LCD_write_str
* Author         :llemmx
* Date           :2012-09-22
* Description    :支持混合语言字符串反显
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  dat - 字符串
                  len - 字符串长度
                  size - 字体尺寸
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_write_str(TLCDWStrParam *param)
{
    uint8_t  i,fontid=0U,bf=0U,bg=0U,l_uchar;
    uint16_t font;
    uint16_t add=1U;
    
    for (i=0U;i<param->len;){
        if (param->dat[i]!=(char)'\0')
        {
			//字符识别
			if ((param->len-i)>1U){//识别混合字符
				if (LCD_FONT_EAST==param->code){
					//中文这里均判断是否为GB2312编码
					if (((uint8_t)param->dat[i]>=0xA1U) && ((uint8_t)param->dat[i+1U]>0x9FU)){//中文(A0H~FEH)
						add=2U;
						fontid=FONT_ZH_CN;
					}else{//英文
						add=1U;
						fontid=FONT_EN;
					}
				}else{
					l_uchar=(uint8_t)param->dat[i]; //wujing 2012.12.21
					//西方文字均使用UTF-8编码
					if (l_uchar<128U){
						add=1U;
						fontid=FONT_EN;
					}else if ((l_uchar & 0xE0U)==0xC0U ){//UTF8-2B
						add=2U;
						fontid=FONT_UTF2;
					}else{// do nothing
					}
				}
			}else{//识别单字节字体
				add=1U;
				fontid=FONT_EN;
			}
			LCD_getfontbysize((uint16_t)param->size,&font,fontid);
			if ((param->index!=0U) && ((i+1U)==param->index)){
				bf=g_front;
				bg=g_backend;
				g_front=g_backend;
				g_backend=bf;
			}
			LCD_writechar(param->x,param->y,param->dat+i,font);
			if ((param->index!=0U) && ((i+1U)==param->index)){
				g_front=bf;
				g_backend=bg;
			}
			param->x+=(FontInfo[font]>>8) & 0xFFU;
			if ((font==FONT_ASCII5x8) || (font==FONT_ASCII8x16) || (font==FONT_UTF82B5x8))
			{
				++param->x;
			}
			i+=add;
        }
		else	   //wujing 2012.12.04 
		{
			break;
		}
    }
}
/******************************************************************************
* Function       :LCD_drawline
* Author         :llemmx
* Date           :2011-05-03
* Description    :画线
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  gray - 灰度
                  len - 线长
                  flag - 线的方向标志
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_drawline(uint8_t x,uint8_t y,uint8_t len,uint8_t gray,uint8_t flag)
{
    //u16 i;
    GData gdat;
    
    gdat.gray=gray;
    gdat.dat=NULL;
    if ((LCD_DRAW_HORI & flag)!=0U){
        gdat.x = x;
        gdat.y = y;
        gdat.width = len;
        gdat.height = 1U;
        (void)drv_write(gt_glp.flcd, &gdat, (uint32_t)len);
    }else if ((LCD_DRAW_VERT & flag)!=0U){
        gdat.x = x;
        gdat.y = y;
        gdat.width = 1U;
        gdat.height = len;
        (void)drv_write(gt_glp.flcd, &gdat, (uint32_t)len);
    }else
    {
    	//do nothing for MISRA-2004
    }
}

/******************************************************************************
* Function       :LCD_drawrect
* Author         :llemmx
* Date           :2011-05-03
* Description    :画矩形框
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  w - 宽
                  h - 高
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_drawrect(uint8_t x,uint8_t y,uint8_t w,uint8_t h)
{
    LCD_drawline(x,y,h,0xFFU,LCD_DRAW_VERT);
    LCD_drawline(x+w,y,h,0xFFU,LCD_DRAW_VERT);
    LCD_drawline(x,y,w,0xFFU,LCD_DRAW_HORI);
    LCD_drawline(x,y+h,w,0xFFU,LCD_DRAW_HORI);
}

/******************************************************************************
* Function       :LCD_drawrectarc
* Author         :llemmx
* Date           :2011-05-03
* Description    :画圆角矩形框
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :x - 列坐标
                  y - 行坐标
                  w - 宽
                  h - 高
                  sd - 灰度值
				  solid - 是否填充为实心
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_drawrectarc(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t sd, uint8_t solid)
{
    LCD_drawline(x, y + 2U, h - (2U*2U), sd, LCD_DRAW_VERT);
    LCD_drawline((x + w) - 1U, y + 2U, h - (2U*2U), sd, LCD_DRAW_VERT);
    LCD_drawline(x + 2U, y, w - (2U*2U), sd, LCD_DRAW_HORI);
    LCD_drawline(x + 2U, (y + h) - 1U,w - (2U*2U), sd, LCD_DRAW_HORI);
    
    if(solid == 0U)
    {
        LCD_drawline(x + 1U, y + 1U, 1U, sd, LCD_DRAW_VERT);
        LCD_drawline((x + w) - 2U, y + 1U, 1U, sd, LCD_DRAW_VERT);
        LCD_drawline(x + 1U, (y + h) - 2U, 1U, sd, LCD_DRAW_HORI);
        LCD_drawline((x + w) - 2U, (y + h) - 2U, 1U, sd, LCD_DRAW_HORI);
    }
    else
    {
        uint8_t i;
        LCD_drawline(x + 1U, y + 1U, w - 2U, sd, LCD_DRAW_HORI);
        LCD_drawline(x + 1U, (y + h) - 2U,w - 2U, sd, LCD_DRAW_HORI);
        if(h > 4U)
        {
			for(i = 0U; i < (h - 4U); i++)
			{
				LCD_drawline(x, y + 2U + i, w, sd, LCD_DRAW_HORI);
			}
        }
    }
}

/******************************************************************************
* Function       :LCD_showimage
* Author         :llemmx
* Date           :2011-05-03
* Description    :显示图片
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :row - 行坐标
                  col - 列坐标
                  width - 宽
                  high - 高
                  bmp - 位图数据指针
                  gray - 灰度
                  flag - 标志
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_showimage(uint8_t row,uint8_t col,uint8_t width,uint8_t high,const uint8_t *bmp,uint8_t gray,uint8_t flag)
{
    GData gdat;

    gdat.x      = row;
    gdat.y      = col;
    gdat.width  = width;
    gdat.height = high;
    gdat.gray   = gray;
    gdat.dat    = bmp;
    
    (void)drv_write(gt_glp.flcd, &gdat, width*high);
}

/******************************************************************************
* Function       :LCD_setbacklight
* Author         :llemmx
* Date           :2011-05-03
* Description    :adjust contrast (default 26)
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :dat - 对比度值
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
void LCD_setbacklight(uint16_t dat)
{
    (void)drv_ioctl(gt_glp.flcd, LM_SET_CONTRAST, &dat);
}

/******************************************************************************
* Function       :LCD_open
* Author         :llemmx
* Date           :2011-05-03
* Description    :LCD模块入口函数
* Calls          :无
* Table Accessed :无
* Table Updated  :无
* Input          :lcd - 实体指针
* Return         :无
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
bool LCD_open(TLCDModule *lcd)
{
    lcd->clear       =LCD_ClearRAM;
    lcd->cleararea   =LCD_ClearArea;
    lcd->setbacklight=LCD_setbacklight;
    lcd->writechar   =LCD_writechar;
    lcd->writestr    =LCD_writestr;
    lcd->writestru   =LCD_writestru;
    lcd->showimage   =LCD_showimage;
    lcd->drawline    =LCD_drawline;
    lcd->setfontcolor=LCD_setfontcolor;
    lcd->drawrect    =LCD_drawrect;
    lcd->drawrectarc =LCD_drawrectarc;
    lcd->write_str   =LCD_write_str;
    return true;
}

