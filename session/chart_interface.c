/******************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
 * File name      : chart_interface.c
 * Author         : Linfei
 * Date           : 2011.12.07
 * Description    : 底层绘图接口，用户实现，供Chart CBB模块调用
 * Others         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.07 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *------------------------------------------------------------------------------
 ******************************************************************************/
#include "chart_interface.h"
#include "global.h"
extern TLCDModule m_lcd;
/********************************************************************************
 * Function       : chart_draw_dot
 * Author         : Linfei
 * Date           : 2011.12.07
 * Description    : 画点接口，用户实现
 * Calls          : None
 * Input          : x：绘制点的x坐标
 *                  y：绘制点的y坐标
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.07 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
void chart_draw_dot(uint16_t x, uint16_t y)
{
	m_lcd.drawline(x, y, 1, 0xFF, LCD_DRAW_VERT);
}

/********************************************************************************
 * Function       : chart_draw_line
 * Author         : Linfei
 * Date           : 2011.12.07
 * Description    : 直线绘制接口，用户实现
 * Calls          : None
 * Input          : x1：绘制直线的起始x坐标
 *                  y1：绘制直线的起始y坐标
 *                  x2：绘制直线的终止x坐标
 *                  y2：绘制直线的终止y坐标
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.07 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
void chart_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    if(x1 == x2)      //画竖线
	{
	    if(y1 > y2)
        {
		    m_lcd.drawline(x1, y2, y1 - y2 + 1, 0xFF, LCD_DRAW_VERT);	
        }
		else
        {
		    m_lcd.drawline(x1, y1, y2 - y1 + 1, 0xFF, LCD_DRAW_VERT);
        }    
	}
	else if(y1 == y2) //画横线
	{
	    if(x1 < x2)
        {
		    m_lcd.drawline(x1, y1, x2 - x1 + 1, 0xFF, LCD_DRAW_HORI);
        }
		else
        {
		    m_lcd.drawline(x2, y1, x1 - x2 + 1, 0xFF, LCD_DRAW_HORI);
        }    
	}
}

/********************************************************************************
 * Function       : chart_draw_str
 * Author         : Linfei
 * Date           : 2011.12.07
 * Description    : 字符串绘制接口，用户实现
 * Calls          : None
 * Input          : x：绘制字符串的起始x坐标
 *                  y：绘制字符串的起始y坐标
 *                  str：待绘制的字符串
 *                  len：字符串长度
 *                  size：绘制直线的终止y坐标
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.07 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
void chart_draw_str(uint16_t x,uint16_t y,const char *str,uint16_t len,uint16_t size)
{
	m_lcd.writestru(x - (len - 1), y, str, len, size);
}
