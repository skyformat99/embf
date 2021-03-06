#ifndef LCDMODULE_H
#define LCDMODULE_H

#include "fcntl.h"
#include "global.h"
#include "drivers.h"
#include "font.h"
#include "language.h"


#define LCD_DRAW_BIT   0x01U
#define LCD_DRAW_GRAY  0x02U
#define LCD_DRAW_HORI  0x04U //����
#define LCD_DRAW_VERT  0x08U //����
#define LCD_DRAW_STAIR 0x10U //�𽥻Ҷ�

#define LCD_FONT_WEST 0U
#define LCD_FONT_EAST 1U

typedef struct {
    uint16_t x;//
    uint16_t y;
    const char *dat;//string data
    uint8_t len;//string length
    uint8_t size; //font size
    uint8_t index;//Anti-significant control, string index
    uint8_t code; //Font encoding mode, 0 or 1
}TLCDWStrParam;

typedef struct {
    void (*clear)(void);
    void (*cleararea)(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
    void (*setbacklight)(uint16_t );
    void (*writechar)(uint16_t ,uint16_t ,const char [],uint16_t );
    void (*writestr)(uint16_t ,uint16_t ,const char [],uint8_t ,uint16_t );
    void (*writestru)(uint16_t ,uint16_t ,const char [],uint8_t ,uint8_t );
    void (*write_str)(TLCDWStrParam *param);
    void (*showimage)(uint8_t ,uint8_t ,uint8_t ,uint8_t ,const uint8_t *,uint8_t ,uint8_t );
    void (*drawline)(uint8_t ,uint8_t ,uint8_t ,uint8_t ,uint8_t );
    void (*drawrect)(uint8_t,uint8_t,uint8_t,uint8_t);
    void (*drawrectarc)(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
    void (*setfontcolor)(uint8_t,uint8_t);
}TLCDModule;

extern TLCDModule gLCD;

bool LCD_open(TLCDModule *lcd);

#endif

