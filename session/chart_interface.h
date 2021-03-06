/******************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
 * File name      : chart_interface.h
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
#ifndef __CHART_INTERFACE_H__
#define __CHART_INTERFACE_H__
#include "lcd_module.h"
#include "font.h"

// 字体大小定义
#define FONT_SIZE_1  FONT_SIZE_6


// 字符长宽
#define  ASCII_WIDTH              5
#define  ASCII_HIGH               9

// 字符坐标与像素坐标的比例
#define  ASCII_PIXEL_RATIO        1

// Chart CBB底层接口声明 --------------------------------------------------------
void chart_draw_dot(uint16_t x, uint16_t y);
void chart_draw_str(uint16_t x,uint16_t y,const char *str,uint16_t len,uint16_t size);
void chart_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

#endif

