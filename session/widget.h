//ver:1.0.1
#ifndef WIDGET_H_
#define WIDGET_H_
#include "fcntl.h"
#include "font.h"
#include "language.h"
#include "lcd_module.h"
#include "menu.h"
#include "message.h"

typedef struct {
    uint16_t x;    //坐标
    uint16_t y;
    uint16_t w;    //间距
    uint16_t zw;   //间距
    uint16_t num;  //次数
    uint16_t value;//当前值
    uint16_t key;  //当前按键
    uint16_t dir;  //方向(1=横向，0=纵向，2=Z字形)
}TArrow;

typedef struct
{
    uint16_t x;        // 菜单项首坐标
    uint16_t y;        // 菜单项首坐标
    uint8_t *item;     // 菜单项数组指针，数据为MenuStr_zh下标
    uint8_t dis;       // 菜单项间距
    uint8_t num;       // 菜单项个数
    uint8_t cur;       // 当前选中菜单项
    uint8_t init;      // 初始标志
    uint8_t rect_w;   // 矩形阴影宽度
    uint8_t rect_h;   // 矩形阴影高度
    uint8_t font;     // 字体大小
    uint8_t bcolor;   // 背景色
} MItem;

typedef struct {
    int def;          //当前显示值
    int max;          //最大值
    int min;          //最小值
    uint8_t  x;       //坐标
    uint8_t  y;
    uint8_t  width;   //输入宽度
    uint16_t font;    //指定字体
    uint8_t  point:2; //定点小数位置
    uint8_t  symbol:1;//符号输入
    uint8_t  ptr:5;   //当前闪烁位置
    uint8_t  timeover;//超时时间
    uint8_t  flash;   //默认闪动
    char     buf[16]; //显示缓冲
    char     back[16];  // 备份缓冲
}TInputBox;

typedef struct {
    uint8_t  *ip;     //当前显示值
    uint8_t  x;       //坐标
    uint8_t  y;
    uint8_t  ptr;     //当前闪烁位置
    uint8_t  timeover;//超时时间
    uint8_t  flash;   //默认闪动
    char     buf[16]; //显示缓冲
    uint16_t font;    //指定字体
}TIPInputBox;

typedef struct {
    uint8_t  *time;     //当前显示值
    uint8_t  mark;      // 标志：2=年月日 1=时分秒
    uint8_t  x;         //坐标
    uint8_t  y;
    uint8_t  ptr;       //当前闪烁位置
    uint8_t  timeover;  //超时时间
    uint8_t  flash;     //默认闪动
    char     buf[16];   //显示缓冲
    uint16_t font;      //指定字体
}TTInputBox;

void widget_setlcd(PProcBuf pb,void *lcd);
void widget_ItemSelect(MItem *mitem);
void widget_label(uint16_t x,uint16_t y,char *str,uint16_t size);
void widget_progress(uint16_t x,uint16_t y,uint16_t w,uint16_t key,uint16_t *value,uint8_t jump);
uint8_t widget_arrow(TArrow *ar);
int32_t widget_input(TInputBox *box);
int32_t widget_ipinput(TIPInputBox *box);//ip输入控件
int32_t widget_timeinput(TTInputBox *box);//时间输入控件
void widget_setlangcoo(uint16_t x,uint16_t y,uint8_t lang);//多语言坐标设定

#endif
