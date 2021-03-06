#ifndef MENU_H
#define MENU_H
#include "fcntl.h"
#include "global.h"
#include "lcd_module.h"
#include "message.h"
#include "font.h"
#include "widget.h"
#include "language.h"
#include "drivers.h"

#define MKEY_BTN_UP   ((uint16_t)0x0100)
#define MKEY_BTN_DOWN ((uint16_t)0x0000)

#define IS_VALID_ACTION(act) ((act == MKEY_BTN_UP) || \
                              (act == MKEY_BTN_DOWN))


#define MKEY_NONE     0
#define MKEY_ESC      ((uint16_t)0x0006)
#define MKEY_DESC     ((uint16_t)0x0008) //长按ESC键
#define MKEY_ENTER    ((uint16_t)0x0007)
#define MKEY_UP       ((uint16_t)0x0001)
#define MKEY_DOWN     ((uint16_t)0x0002)
#define MKEY_RIGHT    ((uint16_t)0x0003)
#define MKEY_DRIGHT   ((uint16_t)0x0009)//长按RIGHT键
#define MKEY_LEFT     ((uint16_t)0x0004)
#define MKEY_SHIFT    ((uint16_t)0x0005)

#define IS_VALID_KEY(key) ((key == MKEY_ESC)    || \
                           (key == MKEY_DESC)   || \
                           (key == MKEY_ENTER)  || \
                           (key == MKEY_UP)     || \
                           (key == MKEY_DOWN)   || \
                           (key == MKEY_RIGHT)  || \
                           (key == MKEY_DRIGHT) || \
                           (key == MKEY_LEFT)   || \
                           (key == MKEY_SHIFT))

#define MRET_OK      0
#define MRET_REFRESH 1
#define MRET_EXIT    2


#define MENU_TIMEOUT     240  // 菜单无操作超时时间，单位：0.5s
#define MENU_STARTDELAY  4    // 上电后进入到主菜单前的每个菜单页面停留时间，单位：0.5s




typedef int (*ActionFunc)(void *p);//菜单函数

typedef struct
{
    ActionFunc acMenu; // 当前活动菜单函数
    uint32_t param1;   // 自定参数
    uint32_t MenuTOut; // 菜单超时时间
    uint8_t ArrowPtr;       // 光标计数
    uint8_t SubPtr;         // 子选项光标
    uint8_t Key;            // 当前菜单响应的按键
    uint8_t Tick;           // 计时点
    bool Refresh;      // 刷新
    bool SubRefresh;   // 页面部分刷新标志
    uint8_t title[24]; // 标题缓冲区
    uint8_t buf[80];   // 可作为设置缓冲区
} TMenu, *PMenu;

int RegUIMsg(PProcBuf proc,void *lcd);     /* UI注册函数 */

#endif
