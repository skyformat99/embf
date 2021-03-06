#ifndef FONT_H_
#define FONT_H_

#include "global.h"

#define FONT_ASCII12x24  0U
#define FONT_ASCII8x16   1U
#define FONT_ASCII5x8    2U
#define FONT_GB12x12     3U
#define FONT_GB16x16     4U
#define FONT_GB24x24     5U
#define FONT_ASCII6x12   6U
#define FONT_ASCII17x40  7U
#define FONT_UTF82B5x8   8U
#define FONT_UTF82B8x16  9U
#define FONT_UTF82B12x24 10U
#define FONT_UTF82B6x12  11U
#define FONT_UTF82B17x40 12U
#define FONT_LAST FONT_UTF82B17x40

#define FONT_SIZE_6     6U  //5x8
#define FONT_SIZE_9     9U  //6x12  12x12
#define FONT_SIZE_11    11U //8x16  16x16
#define FONT_SIZE_18    18U //12x24 24x24
#define FONT_SIZE_22    22U //17x40

#define FONT_ZH_CN 0x01U //简体中文
#define FONT_ZH_TW 0x02U //繁体中文
#define FONT_EN    0x03U //英文字体
#define FONT_FR    0x04U //法文字体
#define FONT_SPAN  0x05 //西班字体
#define FONT_KON   0x06 //韩文
#define FONT_JPN   0x07 //日文
#define FONT_DE    0x08 //德文

#define FONT_UTF2  0x09U //UTF-8 2B

typedef struct {
    char *name;
    char *data;
}TFont;

extern uint16_t const FontInfo[];  //字体信息文件
extern uint8_t const ASCII_17x40[];//17x40 英文字体
extern uint8_t const ASCII_12x24[];//12x24 英文字体
extern uint8_t const ASCII_8x16[]; //8x16 英文字体
extern uint8_t const ASCII_6x12[]; //6x12 英文字体
extern uint8_t const ASCII_5x8[];  //5x8  英文字体

extern TFont const TFU2S5x8[];     //5x8 utf8 2b
extern TFont const TFU2S8x16[];    //8x16 utf8 2b
extern TFont const TFU2S12x24[];   //12x24 utf8 2b
extern TFont const TFU2S6x12[];    //6x12 utf8 2b
extern TFont const TFU2S17x40[];   //17x40 utf8 2b

extern TFont const TFGB12x12[];    //12x12 中文字体
extern TFont const TFGB16x16[];    //16x16 中文字体
extern TFont const TFGB24x24[];    //24x24 中文字体

uint16_t getfontsize(uint16_t flag);

#endif
