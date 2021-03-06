/******************************************************************************
 * Copyright (C), 1997-2011, SUNGROW POWER SUPPLY CO., LTD.
 * File name      : chart.c
 * Author         : Linfei
 * Date           : 2011.12.09
 * Description    : 定义了柱状图、曲线图绘制函数
 * Others         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.09 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *------------------------------------------------------------------------------
 ******************************************************************************/
#include "string.h"
#include "stdio.h"
#include "chart.h"
#include "math.h"

/*lint -e526 */
/*lint -e628 */


// 柱状图中的最大柱子个数
#define MAX_HISTOGRAM_NUM     20

// 宽度调节分量，除
#define WIDTH_ADJUST_DIV      3

// 宽度上限分量，除
#define WIDTH_MAX_DIV         9

// 刻度长度，单位：像素
#define CALIBRATION_LEN       2

// 坐标轴与刻度的间距，单位：像素
#define X_CALIBRATION_MARGIN  2
#define Y_CALIBRATION_MARGIN  4

// 坐标轴与边框的纵向间距
#define Y_TITLE_MARGIN        (ASCII_HIGH + 5)

// y轴单位和坐标系图形的间距，单位：像素
#define Y_TITLE_ADJUST        (ASCII_HIGH + 4)

// x轴标题高度调节参数
#define X_TITLE_ADJUST        6

// Y轴刻度值显示位置上调像素
#define Y_RANGE_UP_ADJUST     ((ASCII_HIGH>>1) - 1)

// 擦除某处数据的标志
#define  DATA_ERASE           1
#define  DATA_SHOW            0


#ifdef USE_HISTOGRAM_DRAW_TOOL
// 柱状图显示数据缓冲区
typedef struct
{
    CVector  filter[MAX_HISTOGRAM_NUM];    // （x轴像素坐标，y轴数据值）
    uint16_t histonum;                     // 数据个数，即柱子个数
    uint16_t show_idx;                     // 前一个显示的数据所在位置，范围：1 ~ MAX_HISTOGRAM_NUM
    uint16_t high;                         // y轴高度，单位：像素
    CVector  xy;                           // 坐标系原点坐标
    CVector  yrange;                       // 数据范围
} HistoData;

HistoData hdata = { 0 };
#endif

// 内部函数声明----------------------------------------------------------------------------
#if defined (USE_CURVEGRAM_DRAW_TOOL) || defined (USE_HISTOGRAM_DRAW_TOOL)
static void chart_draw_right_line(uint16_t x, uint16_t y, uint16_t width, uint16_t thick);
static void chart_draw_up_line(uint16_t x, uint16_t y, uint16_t high, uint16_t thick);
void data_dump_preprocess(char *destbuf, uint8_t accuracy, uint16_t data);
#endif

#ifdef USE_HISTOGRAM_DRAW_TOOL
static  uint16_t chart_hanalysis(CVector *dest, uint16_t *sourcea, \
                uint16_t *sourceb, uint16_t sourcenum, CVector yrange);
static void chart_show_hdata(uint16_t index, uint8_t erase);
#endif

#ifdef USE_CURVEGRAM_DRAW_TOOL
static uint16_t chart_canalysis(const uint16_t *data, const uint16_t datanum);
#endif
// ----------------------------------------------------------------------------------------


/********************************************************************************
 * Function       : chart_show_next_hdata
 * Author         : Linfei
 * Date           : 2011.12.09
 * Description    : 显示下一个柱子的数据
 * Calls          : chart_show_hdata
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.09 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
#ifdef USE_HISTOGRAM_DRAW_TOOL
void chart_show_next_hdata(void)
{
    if(hdata.histonum == 0)
    {
        return;
    }
    
    // 前一数据擦除
    if(hdata.show_idx != 0)
    {
        chart_show_hdata(hdata.show_idx, DATA_ERASE);
        hdata.show_idx = hdata.show_idx%hdata.histonum + 1;
    }
    else
    {
        hdata.show_idx = 1;
    }
    
    // 显示新数据
    chart_show_hdata(hdata.show_idx, DATA_SHOW);
}
#endif

/********************************************************************************
 * Function       : chart_show_pre_hdata
 * Author         : Linfei
 * Date           : 2011.12.09
 * Description    : 显示前一个柱子的数据
 * Calls          : chart_show_hdata
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.09 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
#ifdef USE_HISTOGRAM_DRAW_TOOL
void chart_show_pre_hdata(void)
{    
    if(hdata.histonum == 0)
    {
        return;
    }
    
    if(hdata.show_idx != 0)
    {
        // 前一数据擦除
        chart_show_hdata(hdata.show_idx, DATA_ERASE);
        
        if(hdata.show_idx > 1)
        {
            hdata.show_idx--;
        }
        else
        {
            hdata.show_idx = hdata.histonum;
        }
    }
    else
    {
        hdata.show_idx = 1;
    }
    
    // 显示新数据
    chart_show_hdata(hdata.show_idx, DATA_SHOW);
}
#endif


/********************************************************************************
 * Function       : chart_draw_curvegram
 * Author         : Linfei
 * Date           : 2011.12.08
 * Description    : 曲线图绘制函数
 * Calls          : chart_draw_up_line，chart_draw_right_line，chart_hanalysis
 *                  chart_draw_str
 * Input          : curveattr：曲线图属性，显示位置、大小、刻度范围、显示模式等
 *                  data：曲线数据
 *                  datanum：数据个数
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.08 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
#ifdef USE_CURVEGRAM_DRAW_TOOL
int32_t chart_draw_curvegram(Curvegram *curveattr, uint16_t *data, uint16_t datanum)
{
    if(curveattr == NULL || data == NULL || datanum == 0)
    {
        return CHART_PARAM_ERR;
    }

    if(curveattr->xrange.a >= curveattr->xrange.b || \
        curveattr->yrange.a >= curveattr->yrange.b || \
        curveattr->xstep == 0 || curveattr->ystep == 0 || \
        curveattr->hw.a == 0 || curveattr->hw.b == 0 || \
        curveattr->xtitle == NULL || curveattr->ytitle == NULL)
    {
        return CHART_PARAM_ERR;
    }
    
    if((curveattr->xrange.b - curveattr->xrange.a)%curveattr->xstep != 0 || \
        (curveattr->yrange.b - curveattr->yrange.a)%curveattr->ystep != 0)
    {
        return CHART_PARAM_ERR;
    }

    
    // 显示刻度值用
    char tmpbuf[10] = { 0 };
    
    // 计算原点坐标(x0, y0)，横纵坐标长度赋初值(high, width)，下面会调整
    uint16_t x0 = curveattr->xy.a;
    uint16_t y0 = curveattr->xy.b + curveattr->hw.a;
    uint16_t high = curveattr->hw.a;
    uint16_t width = curveattr->hw.b;
/*    
    // 画边框
    chart_draw_up_line(x0, y0, high, 0);
    chart_draw_right_line(x0, y0, width, 0);
    chart_draw_up_line(x0 + width, y0, high, 0);
    chart_draw_right_line(x0, y0 - high, width, 0);
*/    
    // 数据范围调整------------------------------------------------------
    CVector xrange = curveattr->xrange;
    CVector yrange = curveattr->yrange;
    if((curveattr->showstyle & SHOW_DYNAMIC) != 0)
    {
        uint16_t max;         // 最大数据值
        max = chart_canalysis(data, datanum);
        
        if(max != curveattr->yrange.a && max < curveattr->yrange.b)
        {
            yrange.b = max;
        }
        
        if((yrange.b - yrange.a)%curveattr->ystep != 0)
        {
            yrange.b += curveattr->ystep - ((yrange.b - yrange.a)%curveattr->ystep);
        }
    }
    uint8_t xdotnum = (uint8_t)((xrange.b - xrange.a)/curveattr->xstep);
    uint8_t ydotnum = (uint8_t)((yrange.b - yrange.a)/curveattr->ystep);
    // -------------------------------------------------------------------

    // 绘制坐标系---------------------------------------------------------
    sprintf(tmpbuf, "%d", yrange.b);
    x0 += strlen(tmpbuf)*ASCII_WIDTH + (Y_CALIBRATION_MARGIN<<1);
    // 预留高度给底部x轴刻度值显示
    y0 -= Y_TITLE_MARGIN;
    
    // 底部x轴刻度和顶部y轴单位 
    high -= Y_TITLE_MARGIN<<1;
    
    // 刻度像素值形式步长
    width = datanum * CURVE_PIXEL_WIDTH;
    uint16_t xdotstep = (curveattr->xstep * width) / (xrange.b - xrange.a);
    uint16_t ydotstep = (curveattr->ystep * high) / (yrange.b - yrange.a);
    if(xdotstep < 1 || ydotstep < 1)
    {
        return CHART_PARAM_ERR;
    }
    high = ydotstep * ydotnum;
    
    // 绘制坐标轴线和单位
    chart_draw_up_line(x0, y0, high, 0);
    chart_draw_right_line(x0, y0, width, 0);
    chart_draw_str((x0 + width + Y_CALIBRATION_MARGIN)/ASCII_PIXEL_RATIO, y0 - X_TITLE_ADJUST, \
                        curveattr->xtitle, strlen(curveattr->xtitle), FONT_SIZE_1);
    chart_draw_str((x0 - Y_CALIBRATION_MARGIN)/ASCII_PIXEL_RATIO, y0 - high - Y_TITLE_ADJUST, \
                        curveattr->ytitle, strlen(curveattr->ytitle), FONT_SIZE_1);
    
    // 绘制y轴刻度
    uint16_t i = 0;
    uint16_t j = 0;
    while(i <= high)
    {
        chart_draw_right_line(x0 - CALIBRATION_LEN, y0 - i, CALIBRATION_LEN, 0);
        if((j == 0) || (j == (ydotnum>>1)) || (j == ydotnum))
        {
            sprintf(tmpbuf, "%d", yrange.a + j*curveattr->ystep);
            chart_draw_str((x0 - Y_CALIBRATION_MARGIN - strlen(tmpbuf)*ASCII_WIDTH)/ASCII_PIXEL_RATIO, \
                                y0 - i - Y_RANGE_UP_ADJUST, tmpbuf, strlen(tmpbuf), FONT_SIZE_1);
        }
        i += ydotstep;
        j++;
    }

    // 绘制x轴刻度
    i = 0;
    j = 0;
    while(i <= width)
    {
        chart_draw_up_line(x0 + i, y0 + CALIBRATION_LEN, CALIBRATION_LEN, 0);
        if((j == 0) || (j == (xdotnum>>1)) || (j == xdotnum))
        {
            sprintf(tmpbuf, "%d", xrange.a + j*curveattr->xstep);
            // 居中显示刻度值，以刻度线为中轴
            chart_draw_str((x0 + i - strlen(tmpbuf)*(ASCII_WIDTH>>1))/ASCII_PIXEL_RATIO, \
                                y0 + CALIBRATION_LEN + X_CALIBRATION_MARGIN, \
                                        tmpbuf, strlen(tmpbuf), FONT_SIZE_1);
        }
        i += xdotstep;
        j++;
    }
    // -------------------------------------------------------------------
    
    // 执行曲线绘图
    for(i = 0; i < datanum; i ++)
    {
        for(j = 0; j < CURVE_PIXEL_WIDTH; j++)
        {
            if(data[i] > yrange.a)
            {
                // 正常
                if(data[i] <= yrange.b)
                {
                    if((curveattr->showstyle & SHOW_SOLID) != 0)
                    {
                        // 实心面积图
                        chart_draw_up_line(x0 + i*CURVE_PIXEL_WIDTH + j, y0, \
                            (uint16_t)(((data[i] - yrange.a) * high) \
                            / (yrange.b - yrange.a)), 0);
                    }
                    else
                    {
                        // 空心虚线图
                        chart_draw_dot(x0 + i*CURVE_PIXEL_WIDTH + j, \
                            y0 - (uint16_t)(((data[i] - yrange.a) * high) \
                            / (yrange.b - yrange.a)));
                    }
                }
                else // 出错
                {
                    if((curveattr->showstyle & SHOW_SOLID) != 0)
                    {
                        chart_draw_dot(x0 + i*CURVE_PIXEL_WIDTH + j, y0 - high);
                    }
                    else
                    {
                        chart_draw_up_line(x0 + i*CURVE_PIXEL_WIDTH + j, y0, high, 0);
                    }
                }
            }
        }
    }
    return CHART_SUCCESS;
}
#endif


/********************************************************************************
 * Function       : chart_draw_histogram
 * Author         : Linfei
 * Date           : 2011.12.09
 * Description    : 柱状图绘制函数
 * Calls          : chart_draw_up_line，chart_draw_right_line，chart_hanalysis
 *                  chart_draw_str
 * Input          : histoattr：柱状图属性，显示位置、大小、刻度范围、显示模式等
 *                  data：柱状图数据
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.09 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
#ifdef USE_HISTOGRAM_DRAW_TOOL
int32_t chart_draw_histogram(Histogram *histoattr, uint16_t *data)
{
    if(histoattr == NULL || data == NULL)
    {
        return CHART_PARAM_ERR;
    }

    if(histoattr->yrange.a >= histoattr->yrange.b || \
        histoattr->ystep == 0 || \
        histoattr->hw.a == 0 || histoattr->hw.b == 0 || \
        histoattr->xtitle == NULL || histoattr->ytitle == NULL)
    {
        return CHART_PARAM_ERR;
    }
    
    if((histoattr->yrange.b - histoattr->yrange.a)%histoattr->ystep != 0)
    {
        return CHART_PARAM_ERR;
    }
    
    // 显示刻度值用
    char tmpbuf[10] = { 0 };
    
    // 计算原点坐标(x0, y0)，横纵坐标长度(high, width)
    uint16_t x0 = histoattr->xy.a;
    uint16_t y0 = histoattr->xy.b + histoattr->hw.a;
    uint16_t high = histoattr->hw.a;
    uint16_t width = histoattr->hw.b;
    
    // 绘制边框
    chart_draw_up_line(x0, y0, high, 0);
    chart_draw_right_line(x0, y0, width, 0);
    chart_draw_up_line(x0 + width, y0, high, 0);
    chart_draw_right_line(x0, y0 - high, width, 0);
    
    // 找出数据最大值，并复制数据
    uint16_t max; // 最大数据值
    hdata.histonum = histoattr->xdotnum;
    max = chart_hanalysis(hdata.filter, histoattr->xdotvalue, \
                            data, hdata.histonum, histoattr->yrange);
    
    // 生成纵坐标动态数据范围
    hdata.yrange = histoattr->yrange;
    if((histoattr->showstyle & SHOW_DYNAMIC) != 0)
    {
        if(max != histoattr->yrange.a && max < histoattr->yrange.b)
        {
            hdata.yrange.b = max;
        }
        else
        {
            hdata.yrange.b = histoattr->yrange.b;
        }
        
        if((hdata.yrange.b - hdata.yrange.a)%histoattr->ystep != 0)
        {
            hdata.yrange.b += histoattr->ystep - \
                    ((hdata.yrange.b - hdata.yrange.a)%histoattr->ystep);
        }
    }
    uint8_t ydotnum = (uint8_t)((hdata.yrange.b - hdata.yrange.a)/histoattr->ystep);
    
    
    // 绘制坐标系-------------------------------------------
    sprintf(tmpbuf, "%d", hdata.yrange.b);
    if(strlen(histoattr->ytitle) < strlen(tmpbuf))
    {
        x0 += strlen(tmpbuf)*ASCII_WIDTH + (Y_CALIBRATION_MARGIN<<1);
        width -= (strlen(tmpbuf) + strlen(histoattr->xtitle))*ASCII_WIDTH + \
                                                      (Y_CALIBRATION_MARGIN<<2);
    }
    else
    {
        x0 += strlen(histoattr->ytitle)*ASCII_WIDTH + (Y_CALIBRATION_MARGIN<<1);
        width -= (strlen(histoattr->ytitle) + \
             strlen(histoattr->xtitle))*ASCII_WIDTH + (Y_CALIBRATION_MARGIN<<2);
    }
    y0 -= Y_TITLE_MARGIN;
    high -= Y_TITLE_MARGIN<<1;
    
    
    // 刻度像素值形式步长
    uint16_t ydotstep = (histoattr->ystep * high) / (hdata.yrange.b - hdata.yrange.a);
    uint16_t xdotstep;
    if(hdata.histonum == 0)
    {
        xdotstep = width;
    }
    else
    {
        xdotstep = (uint16_t)(width/hdata.histonum);
        width = xdotstep * hdata.histonum;
    }
    if(xdotstep < 1 || ydotstep < 1)
    {
        return CHART_PARAM_ERR;
    }
    high = ydotstep * ydotnum;

    // 绘制坐标轴线和单位
    chart_draw_up_line(x0, y0, high, 0);
    chart_draw_right_line(x0, y0, width, 0);
    chart_draw_str((x0 + width + Y_CALIBRATION_MARGIN)/ASCII_PIXEL_RATIO, \
                   y0 - X_TITLE_ADJUST, histoattr->xtitle, \
                     strlen(histoattr->xtitle), FONT_SIZE_1);
    chart_draw_str((x0 - Y_CALIBRATION_MARGIN - \
            strlen(histoattr->ytitle)*ASCII_WIDTH)/ASCII_PIXEL_RATIO, \
            y0 - high - Y_TITLE_ADJUST, histoattr->ytitle, \
            strlen(histoattr->ytitle), FONT_SIZE_1);
    // 绘制y轴刻度
    uint16_t i = 0;
    uint16_t j = 0;
    while(i <= high)
    {
        chart_draw_right_line(x0, y0 - i, CALIBRATION_LEN, 0);
        if((j == 0) || (j == (ydotnum>>1)) || (j == ydotnum))
        {
            sprintf(tmpbuf, "%d", hdata.yrange.a + j*histoattr->ystep);
            chart_draw_str(((x0 - Y_CALIBRATION_MARGIN) - \
                strlen(tmpbuf)*ASCII_WIDTH)/ASCII_PIXEL_RATIO, \
                y0 - i - Y_RANGE_UP_ADJUST, tmpbuf, strlen(tmpbuf), FONT_SIZE_1);
        }
        i += ydotstep;
        j++;
    }

    // 绘制x轴刻度
    i = xdotstep;
    while(i <= width)
    {
        chart_draw_up_line(x0 + i, y0, CALIBRATION_LEN, 0);
        i += xdotstep;
    }
    // -------------------------------------------------------
    
    // 确定柱子的宽度
    // 柱子宽度默认为x轴步长的2/WIDTH_ADJUST_DIV
    // 且<=width/WIDTH_MAX_DIV/2
    uint8_t thick = (xdotstep>>1)/WIDTH_ADJUST_DIV;
    if(thick > (width>>1)/WIDTH_MAX_DIV)
    {
        thick = (width>>1)/WIDTH_MAX_DIV;
    }
    
    // 显示某一柱子的数据值时用
    hdata.high = high;
    hdata.xy.a = x0;
    hdata.xy.b = y0;
    
    // 绘制柱状图
    for(i = 0; i < hdata.histonum; i++)
    {
        sprintf(tmpbuf, "%d", hdata.filter[i].a);       // 填入坐标值
        hdata.filter[i].a = (xdotstep>>1) + i*xdotstep; // 填入像素值

        // 字符刻度与像素刻度成1：ASCII_PIXEL_RATIO比例
        if((histoattr->showstyle & SHOW_X_SEQ) != 0)
        {
            if((i == 0) || (i == (hdata.histonum>>1)) || (i == hdata.histonum - 1))
            {
                chart_draw_str((x0 + hdata.filter[i].a - \
                    strlen(tmpbuf)*ASCII_WIDTH/2)/ASCII_PIXEL_RATIO, \
                    y0 + CALIBRATION_LEN + X_CALIBRATION_MARGIN, \
                    tmpbuf, strlen(tmpbuf), FONT_SIZE_1);
            }
        }
        else
        {
            chart_draw_str((x0 + hdata.filter[i].a - \
                strlen(tmpbuf)*ASCII_WIDTH/2)/ASCII_PIXEL_RATIO, \
                y0 + X_CALIBRATION_MARGIN + CALIBRATION_LEN, \
                tmpbuf, strlen(tmpbuf), FONT_SIZE_1);
        }
        // 数据值*纵向高度像素/刻度范围
        if(hdata.filter[i].b > hdata.yrange.a)
        {
            if(hdata.filter[i].b <= hdata.yrange.b)
            {
                chart_draw_up_line(x0 + hdata.filter[i].a, y0, \
                    (((hdata.filter[i].b - hdata.yrange.a) * high) \
                    / (hdata.yrange.b - hdata.yrange.a)), thick);
            }
            else
            {
                // 超范围数据以空心柱显示
                chart_draw_up_line(x0 + hdata.filter[i].a - thick - 1, y0, high, 0);
                chart_draw_up_line(x0 + hdata.filter[i].a + thick + 1, y0, high, 0);
                chart_draw_right_line(x0 + hdata.filter[i].a - thick - 1, \
                                                      y0 - high, (thick<<1) + 1, 0);
            }
        }
    }
    return CHART_SUCCESS;
}
#endif

/********************************************************************************
 * Function       : chart_show_hdata
 * Author         : Linfei
 * Date           : 2011.12.08
 * Description    : 绘制指定柱子的数据
 * Calls          : chart_draw_str
 * Input          : index：柱子序号
 *                  erase：擦除显示还是正常显示
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.08 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
#ifdef USE_HISTOGRAM_DRAW_TOOL
static void chart_show_hdata(uint16_t index, uint8_t erase)
{
    char tmpbuf[10] = { 0 };
    if(index > MAX_HISTOGRAM_NUM || index < 1)
    {
        return;
    }
    index--;
    sprintf(tmpbuf, "%d", hdata.filter[index].b);
    
    if(erase == DATA_ERASE)
    {
        uint8_t len = strlen(tmpbuf);
        while(len != 0)
        {
            tmpbuf[len - 1] = 0x20;
            len--;
        }
    }

    uint16_t showx = hdata.xy.a + hdata.filter[index].a - \
                            strlen(tmpbuf)*(ASCII_WIDTH>>1);
    uint16_t showy = hdata.xy.b - Y_TITLE_ADJUST;
    if(hdata.filter[index].b <= hdata.yrange.b && hdata.filter[index].b >= hdata.yrange.a)
    {
        showy -= ((hdata.filter[index].b - hdata.yrange.a) * hdata.high) \
                    / (hdata.yrange.b - hdata.yrange.a);
    }
    else if(hdata.filter[index].b > hdata.yrange.b)
    {
        showy -= hdata.high;
    }
    // 防止第一个数据显示与y轴发生重叠
    if(showx < hdata.xy.a + ASCII_WIDTH)
    {
        showx = hdata.xy.a + ASCII_WIDTH;
    }
    chart_draw_str(showx/ASCII_PIXEL_RATIO, showy, tmpbuf, strlen(tmpbuf), FONT_SIZE_1);
}
#endif


/********************************************************************************
 * Function       : chart_hanalysis
 * Author         : Linfei
 * Date           : 2011.12.08
 * Description    : 柱状图数据分析，根据显示模式选择是否剔除下限值，并找出最大值
 * Calls          : None
 * Input          : sourcea：x轴用户源数据
 *                  sourceb：y轴用户源数据
 *                  sourcenum：用户数据个数
 *                  style：数据显示模式，即分析模式
 * Output         : dest：根据分析模式分析所得的数据
 *                  destnum：分析所得的数据个数
 * Return         : 返回数据最大值，出错返回0
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.08 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
#ifdef USE_HISTOGRAM_DRAW_TOOL 
static  uint16_t chart_hanalysis(CVector *dest, uint16_t *sourcea, \
                    uint16_t *sourceb, uint16_t sourcenum, CVector yrange)
{
    if(dest == NULL || sourcea == NULL || sourceb == NULL)
    {
        return 0;
    }
    
    uint16_t i;
    uint16_t max = yrange.a;    
    
    // 找出最大数据值
    for(i = 0; i < sourcenum; i++)
    {
        if(max < sourceb[i])
        {
            max = sourceb[i];
        }
        
        // 单纯复制数据
        dest[i].a = sourcea[i];
        dest[i].b = sourceb[i];
    }
    return max;
}
#endif
/********************************************************************************
 * Function       : chart_canalysis
 * Author         : Linfei
 * Date           : 2011.12.08
 * Description    : 曲线图数据分析，找出最大值
 * Calls          : None
 * Input          : data：用户数据
 *                  datanum：用户数据个数
 * Output         : None
 * Return         : 返回数据最大值，出错返回0
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.08 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
#ifdef USE_CURVEGRAM_DRAW_TOOL
static uint16_t chart_canalysis(const uint16_t *data, const uint16_t datanum)
{
    if(data == NULL || datanum == 0)
    {
        return 0;
    }
    uint16_t i;
    uint16_t max = 0;    // 最大数据值
    for(i = 0; i < datanum; i++)
    {
        if(max < data[i])
        {
            max = data[i];
        }
    }
    return max;
}
#endif

// 基本图元绘制、字符处理函数----------------------------------------------------------
#if defined (USE_CURVEGRAM_DRAW_TOOL) || defined (USE_HISTOGRAM_DRAW_TOOL)


void data_dump_preprocess(char *destbuf, uint8_t accuracy, uint16_t data)
{
    if(destbuf == NULL)
    {
        return;
    }
    if(accuracy == 0)
    {
        sprintf(destbuf, "%d", data);
    }
    else
    {
        sprintf(destbuf, "%.2f", data / (float)pow(10, accuracy));
    }
}

/********************************************************************************
 * Function       : chart_draw_right_line
 * Author         : Linfei
 * Date           : 2011.12.08
 * Description    : 以(x, y)为基点，Y = y为中轴，向右绘制粗细为thick的直线
 * Calls          : chart_draw_line
 * Input          : x：基点横坐标
 *                  y：基点纵坐标
 *                  len：直线长度
 *                  thick：直线粗细程度
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.08 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
static void chart_draw_right_line(uint16_t x, uint16_t y, uint16_t len, uint16_t thick)
{
    uint16_t i;
    chart_draw_line(x, y, x + len, y);
    if(thick != 0)
    {
        for(i = 1; i <= thick; i++)
        {
            chart_draw_line(x, y - i, x + len, y - i);
            chart_draw_line(x, y + i, x + len, y + i);
        }
    }
}

/********************************************************************************
 * Function       : chart_draw_up_line
 * Author         : Linfei
 * Date           : 2011.12.08
 * Description    : 以(x, y)为基点，X = x为中轴，向上绘制粗细为thick的直线
 * Calls          : chart_draw_line
 * Input          : x：基点横坐标
 *                  y：基点纵坐标
 *                  len：直线长度
 *                  thick：直线粗细程度
 * Output         : None
 * Return         : None
 *******************************************************************************
 * History        :
 *------------------------------------------------------------------------------
 * 2011.12.08 : 1.0.0 : Linfei
 * Modification   : 初始代码编写。
 *********************************************************************************/
static void chart_draw_up_line(uint16_t x, uint16_t y, uint16_t len, uint16_t thick)
{
    uint16_t i;
    chart_draw_line(x, y, x, y - len);
    if(thick != 0)
    {
        for(i = 1; i <= thick; i++)
        {
            chart_draw_line(x - i, y, x - i, y - len);
            chart_draw_line(x + i, y, x + i, y - len);
        }
    }
}
#endif
// --------------------------------------------------------------------------------


