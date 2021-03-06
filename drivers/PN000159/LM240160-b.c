#include <stdbool.h>
#include "lm240160.h"

/*========================控制信号================================*/
//片选
#define LM_CS_LOW       (PORT_LM_CS->BSRRH  |= PIN_LM_CS)
#define LM_CS_HIGH      (PORT_LM_CS->BSRRL  |= PIN_LM_CS)
//写
#define LM_WR_LOW       (PORT_LM_WR->BSRRH  |= PIN_LM_WR)
#define LM_WR_HIGH      (PORT_LM_WR->BSRRL  |= PIN_LM_WR)
//读
#define LM_RD_LOW       (PORT_LM_RD->BSRRH  |= PIN_LM_RD)
#define LM_RD_HIGH      (PORT_LM_RD->BSRRL  |= PIN_LM_RD)
//复位
#define LM_RST_LOW      (PORT_LM_RST->BSRRH |= PIN_LM_RST)
#define LM_RST_HIGH     (PORT_LM_RST->BSRRL |= PIN_LM_RST)
//背光
#define LM_LED_LOW      (PORT_LM_LED->BSRRH |= PIN_LM_LED)
#define LM_LED_HIGH     (PORT_LM_LED->BSRRL |= PIN_LM_LED)
//数据or命令
#define LM_SEL_CMD      (PORT_LM_CD->BSRRH  |= PIN_LM_CD)
#define LM_SEL_DAT      (PORT_LM_WR->BSRRL  |= PIN_LM_CD)
/*===========================END=================================*/

/*====================调用lm_send和lm_get的参数==================*/
#define LM_CMD              1U
#define LM_DAT              2U
/*============================END================================*/

/*===========================命令字==============================*/
#define LM_CMD_CLR_EXT      0X30U        /*ext = 0 */
#define LM_CMD_SET_EXT      0X31U        /*ext = 1 */
#define LM_CMD_DISON        0XAFU        /*开显示 */
#define LM_CMD_DISOFF       0XAEU        /*关显示 */
#define LM_CMD_DISNOR       0XA6U        /*正显示(灰度级0->黑) */
#define LM_CMD_DISINV       0XA7U        /*负显示(灰度级0->白) */
#define LM_CMD_COMSCN       0XBBU        /*common scan */
#define LM_CMD_DISCTRL      0XCAU        /*display control */
#define LM_CMD_SLPIN        0X95U        /*sleep in */
#define LM_CMD_SLPOUT       0X94U        /*sleep out */
#define LM_CMD_LASET        0X75U        /*line address set */
#define LM_CMD_CASET        0X15U        /*column address set */
#define LM_CMD_DATSDR       0XBCU        /*data scan direction */
#define LM_CMD_RAMWR        0X5CU        /*ram write */
#define LM_CMD_RAMRD        0X5DU        /*ram read */
#define LM_CMD_PTLIN        0XA8U
#define LM_CMD_PTLOUT       0XA9U
#define LM_CMD_RMWIN        0XE0U        /*进入修改模式 */
#define LM_CMD_RMWOUT       0XEEU        /*退出修改模式 */
#define LM_CMD_ASCSET       0XAAU
#define LM_CMD_SCSTART      0XABU
#define LM_CMD_OSCON        0XD1U        /*oscillation on */
#define LM_CMD_OSCOFF       0XD2U        /*oscillation off */
#define LM_CMD_PWRCTRL      0X20U        /*power control */
#define LM_CMD_VOLCTRL      0X81U        /*electronic volume control */
#define LM_CMD_VOLUP        0XD6U        /*increment electronic */
#define LM_CMD_VOLDOWN      0XD7U        /*decrement electronic */
#define LM_CMD_EPSRRD1      0X7CU
#define LM_CMD_EPSRRD2      0X7DU
#define LM_CMD_NOP          0X25U        /*no operation */
#define LM_CMD_ANASET       0X32U

/*=========================对齐方式==================================*/
#define ALIGN_LEFT      1U
#define ALIGN_RIGHT     2U
/*============================END=====================================*/


static bool lm_opened = false;
static GPIO_InitTypeDef GPIO_InitStructure;
static bool data_out;//指示当前数据端口的方向


/* 内部函数声明 */
void lm_gpio_init(void);
void LM_PUT(uint8_t data);
uint8_t LM_GET(void);
void lm_send(uint32_t type, uint8_t dat);
uint8_t lm_get(uint32_t type);
void lm_disp_on(void);
void lm_disp_off(void);
void lm_init(void);
int32_t lm_sel_rect(uint8_t left, uint8_t right, uint8_t bottom, uint8_t top);
void lm_setback(uint16_t dat);
int32_t lm_draw_dot(uint8_t x, uint8_t y, uint8_t gray);
int32_t lm_open(int32_t flag, int32_t mode);
static uint8_t align(uint8_t type, uint8_t raw);




/* 将一个字节数据dat放到数据总线上 */
void LM_PUT(uint8_t data)
{
    uint32_t l_dat=0;
    l_dat=(data>>6) & 0x3;
    l_dat=(l_dat<<7) | (data & 0x3F);
    PORT_LM_DAT->ODR &= ~0X01BFU;
    PORT_LM_DAT->ODR |= l_dat;
}
/* 从数据总线上取一个数据赋给dat */
uint8_t LM_GET(void)
{
    uint8_t data = 0U;
    uint32_t l_dat = PORT_LM_DAT->IDR;

    data = l_dat & 0x3F;
    data = data | ((l_dat>>1) & 0xC0);
    return data;
}




void delayms(uint16_t times)
{
    while(times > 0U)
    {
        times--;
    }
}


void LCD_ResetPin_Init(void)
{
    // PF9
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin =  PIN_LM_RST;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(PORT_LM_RST, &GPIO_InitStructure);
    GPIO_SetBits(PORT_LM_RST, PIN_LM_RST);    
}


void LCD_Reset(void)
{
    LM_RST_LOW;
    delayms(50000U);
    delayms(50000U);
    LM_RST_HIGH;
}

/* 初始化I/O端口 */
void lm_gpio_init(void)
{
    LCD_ResetPin_Init();
    LCD_Reset();
    
    /* GPIO set */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin =  PIN_LM_CD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(PORT_LM_CD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  PIN_LM_CS;
    GPIO_Init(PORT_LM_CS, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  PIN_LM_WR;
    GPIO_Init(PORT_LM_WR, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  PIN_LM_RD;
    GPIO_Init(PORT_LM_RD, &GPIO_InitStructure);

    LM_CS_HIGH;
    LM_RD_HIGH;
    LM_WR_HIGH;

    GPIO_InitStructure.GPIO_Pin =  PIN_LM_LED;
    GPIO_Init(PORT_LM_LED, &GPIO_InitStructure);
    LM_LED_HIGH;
}


/* 发送一个字节(命令或数据) */
void lm_send(uint32_t type, uint8_t dat)
{
    if(data_out == false)
    {
        //配置端口为输出
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_Pin  = PIN_LM_DAT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_Init(PORT_LM_DAT, &GPIO_InitStructure);
        data_out = true;
    }

    if(type == LM_CMD)
    {
        LM_SEL_CMD;
    }
    else
    {
        LM_SEL_DAT;
    }

    LM_PUT(dat);

    LM_CS_LOW;
    LM_WR_LOW;
    LM_WR_HIGH;
    LM_CS_HIGH;
}

/* 读取一个字节(寄存器或数据) */
uint8_t lm_get(uint32_t type)
{
    if(data_out == true)
    {
        /* 配置数据端口为输入 */
        GPIO_InitStructure.GPIO_Pin  = PIN_LM_DAT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(PORT_LM_DAT, &GPIO_InitStructure);
        data_out = false;
    }

    uint8_t dat;

    if(type == LM_CMD)
    {
        LM_SEL_CMD;
    }
    else
    {
        LM_SEL_DAT;
    }

    LM_CS_LOW;
    LM_RD_LOW;
    for(int32_t i = 0; i < 100; i++)     /*必须延时!! */
    {
    }
    dat = LM_GET();
    LM_RD_HIGH;
    LM_CS_HIGH;

    return dat;
}

/* 开显示 */
void lm_disp_on(void)
{
    lm_send(LM_CMD, LM_CMD_CLR_EXT);
    lm_send(LM_CMD, LM_CMD_DISON);
}

/* 关显示 */
void lm_disp_off(void)
{
    lm_send(LM_CMD, LM_CMD_CLR_EXT);
    lm_send(LM_CMD, LM_CMD_DISOFF);
}

/*初始化LM240160 */
void lm_init(void)
{

    /* 开背光 */
    LM_LED_LOW;

    /*ext = 0 */
    lm_send(LM_CMD, LM_CMD_CLR_EXT);

    /*sleep out */
    lm_send(LM_CMD, LM_CMD_SLPOUT);

    /*oscillation on */
    lm_send(LM_CMD, LM_CMD_OSCON);

    /*power control */
    lm_send(LM_CMD, LM_CMD_PWRCTRL);
    lm_send(LM_DAT, 0X08U);  /*booster on */

    /*power control */
    lm_send(LM_CMD, LM_CMD_PWRCTRL);
    lm_send(LM_DAT, 0X0BU);  /*booster on, voltage on, voltage follower on */

    /* 反色显示 */
    lm_send(LM_CMD, LM_CMD_DISINV);

    /* electronic volume control(optimum LCD supply voltage) */
    lm_send(LM_CMD, LM_CMD_VOLCTRL);
    lm_send(LM_DAT, 0X0aU);
    lm_send(LM_DAT, 0X04U);

    /* display control */
    lm_send(LM_CMD, LM_CMD_DISCTRL);
    lm_send(LM_DAT, 0X04U);  /* cld=1:clock devided by 2 */
    lm_send(LM_DAT, 0X27U);  /* 160/4-1=0x27 */
    lm_send(LM_DAT, 0X00U);  /* line cycle = 1 */

    /* common scan */
    lm_send(LM_CMD, LM_CMD_COMSCN);
    lm_send(LM_DAT, 0X01U);  /* 行扫描:0->79,159->80 */

    /* data scan direction */
    lm_send(LM_CMD, LM_CMD_DATSDR);
    lm_send(LM_DAT, 0X01U);  /* 原点在左上角,按列扫描 */
    lm_send(LM_DAT, 0X00U);  /* CLR=0: pixel arrangement is:P1,P2,P3 */
    lm_send(LM_DAT, 0X02U);  /* 32 gray scale, 3byte 3pixel mode */

    /* ext=1 */
    lm_send(LM_CMD, LM_CMD_SET_EXT);

    /* analog circuit set */
    lm_send(LM_CMD, LM_CMD_ANASET);
    lm_send(LM_DAT, 0X05U);  /* oscillator frequency = 19.3KHz */
    lm_send(LM_DAT, 0X00U);  /* frequency on booster capacitor = 3KHz */
    lm_send(LM_DAT, 0X02U);  /* LCD bias ratio = 1/12 */

    lm_disp_on();
}

/* 把地址范围定为:列left/3->right/3,行bottom->top */
int32_t lm_sel_rect(uint8_t left, uint8_t right, uint8_t bottom, uint8_t top)
{
    int32_t ret = OPFAULT;
    /* 检查是否满足0<=left<=right<LM_COLUMN_NUM, 0<=bottom<=top<LM_ROW_NUM */
    if((left <= right) && (bottom <= top))
    {
        if((right <= LM_COLUMN_NUM) && (top <= LM_ROW_NUM))
        {
            lm_send(LM_CMD, LM_CMD_CLR_EXT);

            lm_send(LM_CMD, LM_CMD_CASET);
            lm_send(LM_DAT, left / 3U);
            lm_send(LM_DAT, right / 3U);

            lm_send(LM_CMD, LM_CMD_LASET);
            lm_send(LM_DAT, bottom);
            lm_send(LM_DAT, top);
            ret = DSUCCESS;
        }
    }
    return ret;
}

/*设置对比度,经测试,55~60比较合适 */
/*传入的参数是0%~100%所以，这里要这算到40个点 */
void lm_setback(uint16_t dat)
{
    uint16_t value;
    lm_send(LM_CMD, LM_CMD_CLR_EXT);
    lm_send(LM_CMD, LM_CMD_VOLCTRL);
    value = (dat * 40U) / 100U;
    value += 0xFBU; /* 211+40 */
    lm_send(LM_DAT, (uint8_t)(value & 0x3fU));
    lm_send(LM_DAT, (uint8_t)((value >> 6) & 0x07U));
}

/* 画点 */
int32_t lm_draw_dot(uint8_t x, uint8_t y, uint8_t gray)
{
    uint8_t buf[3];

    /* 选中点所在的格子 */
    (void)lm_sel_rect(x, x, y, y);
    lm_send(LM_CMD, LM_CMD_RMWIN);

    (void)lm_get(LM_DAT);     /* dummy read */
    /* 为了效率,不用循环 */
    buf[0] = lm_get(LM_DAT);
    buf[1] = lm_get(LM_DAT);
    buf[2] = lm_get(LM_DAT);

    buf[x%3U] = gray;

    lm_send(LM_DAT, buf[0]);
    lm_send(LM_DAT, buf[1]);
    lm_send(LM_DAT, buf[2]);

    lm_send(LM_CMD, LM_CMD_RMWOUT);
    return DSUCCESS;
}

/* 打开设备 */
int32_t lm_open(int32_t flag, int32_t mode)
{
    int32_t ret = OPFAULT;
    if(lm_opened != true)
    {
        lm_gpio_init();
        lm_init();
        lm_opened = true;
        ret = DSUCCESS;
    }
    return ret;
}

static uint8_t align(uint8_t type, uint8_t raw)
{
    uint8_t ret = 0U;
    if(type == ALIGN_RIGHT)
    {
        /* 将raw对齐到右边的P0列 */
        ret = ((raw % 3U) == 0U) ? raw : ((raw - (raw % 3U)) + 3U);
    }
    else
    {
        /* 将raw对齐到左边的P2列 */
        ret = ((raw % 3U) == 2U) ? raw : ((raw - (raw % 3U)) - 1U);
    }
    return ret;
}

/* 传进的buf须是指向GData结构体的指针 */
int32_t lm_write(const uint8_t buf[], uint32_t count)
{
    int32_t ret = OPFAULT;
    if((lm_opened == true) && (buf != NULL))
    {
        GData *gdat = (GData *)buf;
        /* 获得矩形包围框参数 */
        /* 左上角(rx1,ry1),右下角(rx2,ry2),高h,宽w */
        uint8_t rx1 = (uint8_t)(gdat->x);
        uint8_t rx2 = (uint8_t)(gdat->x + (gdat->width - 1U));
        uint8_t ry1 = (uint8_t)(gdat->y);
        uint8_t ry2 = (uint8_t)(gdat->y + (gdat->height - 1U));
        uint8_t w = (uint8_t)(gdat->width);
        uint8_t h = (uint8_t)(gdat->height);

        /* 确保该矩形在屏幕坐标范围内 */
        if((rx1 <= LM_COLUMN_NUM) && (rx2 <= LM_COLUMN_NUM) &&
           (ry1 <= LM_ROW_NUM) && (ry2 <= LM_ROW_NUM))
        {
            char *dat = gdat->dat;
            uint8_t gray = gdat->gray;

            if((dat == NULL) || (count >= (w * h)))
            {
                /*传入的数据需能填满矩形 */
                /* 如果dat == NULL,则用gdat->gray填充矩形 */

                /*========================================================= */
                /*LM240160每三列像素为一组 */
                /*示例:设x1=1,rx2=10,列编号如下 */
                /*  0   1   2 | 3   4   5 | 6   7   8 | 9   10  11 |  12 */
                /*      rx1     left               right     rx2 */
                /*left是x1右边最靠近x1的P0像素,如果x1就是P0,则left=rx1 */
                /*right是x2左边最靠近x2的P2像素,如果x2就是P2,则right=rx2 */
                /*========================================================= */
                uint8_t left  = align(ALIGN_RIGHT, rx1);
                uint8_t right = align(ALIGN_LEFT, rx2);

                /* 每一行最开始的像素在dat数组中的下标 */
                /* 循环不变量外提,提高循环的速度 */
                uint32_t base;

                /*======================================================= */
                /*1.当x2=0,1时,right=-1(0xff) */
                /*2.当x1=238,239时,left=LM_COLUMN_NUM */
                /*3.矩形宽小于6 */
                /*上面这三种情况都比较特殊,直接用打点法 */
                /*一般情况下,先用打点法画左右两边不对齐的竖条 */
                /*然后再画中间对齐的矩形 */
                /*======================================================= */
                if((left != LM_COLUMN_NUM) && (right != 0xffU) && (w > 6U))
                {
                    uint8_t i, j;
                    /*左边的竖条 */
                    if(rx1 != left)
                    {
                        /*如果dat不为空,则按传入的数据画图 */
                        /*把这个判断放在外面,虽然冗余代码增多,但是可以 */
                        /*大大减少在循环内的判断,效率高 */
                        if(dat != NULL)
                        {
                            for(i = ry1; i <= ry2; ++i)       /*i扫描行 */
                            {
                                base = ((i - ry1) * w) - rx1;      /*base=每一行开始像素在dat中的下标 */
                                for(j = rx1; j < left; ++j)/*j扫描x1到left-1的列 */
                                {
                                    (void)lm_draw_dot(j, i, dat[base + j]);
                                }
                            }
                        }
                        /*如果dat为空,则按gray填充矩形 */
                        else
                        {
                            for(i = ry1; i <= ry2; ++i)
                            {
                                for(j = rx1; j < left; ++j)
                                {
                                    (void)lm_draw_dot(j, i, gray);
                                }
                            }
                        }
                    }
                    /*右边的竖条 */
                    if(rx2 != right)
                    {
                        if(dat != NULL)
                        {
                            for(i = ry1; i <= ry2; ++i)
                            {
                                base = ((i - ry1) * w) - rx1;
                                for(j = right + 1U; j <= rx2; ++j)
                                {
                                    (void)lm_draw_dot(j, i, dat[base+j]);
                                }
                            }
                        }
                        else
                        {
                            for(i = ry1; i <= ry2; ++i)
                            {
                                for(j = right + 1U; j <= rx2; ++j)
                                {
                                    (void)lm_draw_dot(j, i, gray);
                                }
                            }
                        }
                    }

                    /*中间的矩形 */
                    (void)lm_sel_rect(left, right, ry1, ry2);
                    lm_send(LM_CMD, LM_CMD_RAMWR);
                    if(dat != NULL)
                    {
                        for(i = ry1; i <= ry2; ++i)
                        {
                            base = ((i - ry1) * w) - rx1;
                            for(j = left; j <= right; ++j)
                            {
                                (void)lm_send(LM_DAT, dat[base+j]);
                            }
                        }
                    }
                    else
                    {
                        for(i = ry1; i <= ry2; ++i)
                        {
                            for(j = left; j <= right; ++j)
                            {
                                lm_send(LM_DAT, gray);
                            }
                        }
                    }
                }
                else
                {
                    /*直接用打点法填充矩形 */
                    uint8_t i, j;
                    if(dat != NULL)
                    {
                        for(i = ry1; i <= ry2; ++i)
                        {
                            base = (i - ry1) * w;
                            for(j = 0U; j < w; ++j)
                            {
                                (void)lm_draw_dot(rx1 + j, i, (char)dat[base+j]);
                            }
                        }
                    }
                    else
                    {
                        for(i = ry1; i <= ry2; ++i)
                        {
                            for(j = rx1; j <= rx2; ++j)
                            {
                                (void)lm_draw_dot(j, i, gray);
                            }
                        }
                    }
                }
                ret = (int32_t)w * (int32_t)h;
            }
        }
    }
    return ret;
}

int32_t lm_ioctl(uint32_t op, void *arg)
{
    uint16_t contrast;
    GData gdt;
    LCD_Info *info;
    int32_t ret = OPFAULT;
    switch(op)
    {
        /*清屏 */
    case LM_CLR_SCREEN:
        gdt.x = 0U;
        gdt.y = 0U;
        gdt.width = LM_COLUMN_NUM;
        gdt.height = LM_ROW_NUM;
        gdt.gray = 0U;
        gdt.dat = NULL;
        if(lm_write((const uint8_t *)&gdt, LM_COLUMN_NUM * LM_ROW_NUM) > 0)
        {
            ret = DSUCCESS;
        }
        break;

        /*打开LED背光 */
    case LM_LED_ON:
        LM_LED_LOW;
        ret = DSUCCESS;
        break;

        /*关闭LED背光 */
    case LM_LED_OFF:
        LM_LED_HIGH;
        ret = DSUCCESS;
        break;

        /*设置对比度 */
    case LM_SET_CONTRAST:
        contrast = *(uint16_t *)arg;
        lm_setback(contrast);
        ret = DSUCCESS;
        break;

        /*睡眠 */
    case LM_SLEEP_IN:
        lm_send(LM_CMD, LM_CMD_SLPIN);
        LM_LED_HIGH;
        ret = DSUCCESS;
        break;

        /*唤醒 */
    case LM_SLEEP_OUT:
        lm_send(LM_CMD, LM_CMD_SLPOUT);
        LM_LED_LOW;
        ret = DSUCCESS;
        break;

        /*获取LCD信息 */
    case LM_GET_INFO:
        info = (LCD_Info *)arg;
        if(info != NULL)
        {
            info->column_num = LM_COLUMN_NUM;
            info->row_num    = LM_ROW_NUM;
            info->gray_num   = 32U;
        }
        ret = DSUCCESS;
        break;

    default:
        break;
    }
    return ret;
}
