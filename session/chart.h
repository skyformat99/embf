/******************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
 * File name      : chart.h
 * Author         : Linfei
 * Date           : 2011.12.09
 * Description    : Chart CBB接口声明文件，包含了柱状图、曲线图接口声明和显示模
 *                  式等定义
 * Others         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.09 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *------------------------------------------------------------------------------
 ******************************************************************************/
#ifndef __CHART_H__
#define __CHART_H__

#include <stdint.h>
#include "chart_interface.h"

// 只读，勿修改------------------------------------------------------------------
// 图表显示模式
#define   SHOW_DYNAMIC       0x01  // 动态显示数据，坐标刻度根据数据动态变化
#define   SHOW_ALL           0x00  // 显示全部数据
#define   SHOW_X_SEQ         0x04  // X轴刻度为连续数据，只显示首尾和中间值

// 曲线图特有的显示模式
#define   SHOW_SOLID         0x02  // 实心方式显示
#define   SHOW_HOLLOW        0x00  // 空心方式显示

// Chart CBB模块返回值
#define   CHART_PARAM_ERR    (-1)  // 参数错误
#define   CHART_SUCCESS      0     // 绘制成功
// ------------------------------------------------------------------------------


// 用户设定----------------------------------------------------------------------
// 若不使用柱状图绘制函数，将该宏注释掉可以节省RAM
#define USE_HISTOGRAM_DRAW_TOOL

// 若不使用曲线图绘制函数，将该宏注释掉可以节省代码空间
#define USE_CURVEGRAM_DRAW_TOOL



// 曲线图的单个数据占用的像素宽度
#define  CURVE_PIXEL_WIDTH        1
// ------------------------------------------------------------------------------

// 二维向量结构，用于表示坐标(x, y)、长宽(h, w)或者数据范围
typedef struct
{
    uint16_t a;              // 坐标x或者长度h
    uint16_t b;              // 坐标y或者宽度w
} CVector;


// 柱状图属性结构体
typedef struct
{
    CVector  xy;             // 图表的起始坐标
    CVector  hw;             // 图表的长宽
    char     *xtitle;        // x轴数据单位，ASCII码
    char     *ytitle;        // y轴数据单位，ASCII码
    uint16_t *xdotvalue;     // x轴数据列表
    uint16_t xdotnum;        // x轴数据个数
    CVector  yrange;         // y轴数据范围
    uint16_t ystep;          // y轴刻度的步长
    uint8_t  yaccuracy;      // y轴数据精度，该值指示小数位个数，例如0表示没有小数，1表示有1位小数...
    uint8_t  showstyle;      // 显示模式：动态显示或者全部显示
} Histogram;


// 曲线图
typedef struct
{
    CVector  xy;             // 图表的起始坐标
    CVector  hw;             // 图表的长宽
    char     *xtitle;        // x轴数据单位，ASCII码
    char     *ytitle;        // y轴数据单位，ASCII码
    CVector  xrange;         // x轴数据范围
    uint16_t xstep;          // x轴刻度的步长
    CVector  yrange;         // y轴数据范围
    uint16_t ystep;          // y轴刻度的步长
    uint8_t  showstyle;      // 显示模式：动态显示或者全部显示
} Curvegram;


// Chart CBB对外接口声明 --------------------------------------------------------

// 曲线图绘制函数
int32_t chart_draw_curvegram(Curvegram *curveattr, uint16_t *data, uint16_t datanum);

// 柱状图绘制系列函数
int32_t chart_draw_histogram(Histogram *histoattr, uint16_t *data);
void chart_show_pre_hdata(void);
void chart_show_next_hdata(void);
#endif
