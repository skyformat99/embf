#ifndef LM240160_H__
#define LM240160_H__
#include "stm32f4xx.h"
#include "fcntl.h"

#define PORT_LM_DAT     (GPIOF)
#define PIN_LM_DAT      (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | \
                         GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7)

#define PORT_LM_RD      GPIOE
#define PIN_LM_RD       GPIO_Pin_6
#define PORT_LM_WR      GPIOE
#define PIN_LM_WR       GPIO_Pin_5
#define PORT_LM_CD      GPIOE
#define PIN_LM_CD       GPIO_Pin_4
#define PORT_LM_CS      GPIOE
#define PIN_LM_CS       GPIO_Pin_3
#define PORT_LM_LED     GPIOF
#define PIN_LM_LED      GPIO_Pin_10
#define PORT_LM_RST     GPIOF
#define PIN_LM_RST      GPIO_Pin_9

/* ioctl命令 */
#define LM_CLR_SCREEN   0X01U
#define LM_LED_OFF      0X02U
#define LM_LED_ON       0X03U
#define LM_SET_CONTRAST 0X04U
#define LM_SLEEP_IN     0X05U
#define LM_SLEEP_OUT    0X06U
#define LM_GET_INFO     0X07U

/* 绘图参数结构体 */
typedef struct{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint8_t  gray;
    char *dat;
}GData;

/* LCD模块信息结构体 */
typedef struct{
    uint16_t column_num;
    uint16_t row_num;
    uint8_t  gray_num;
}LCD_Info;

#define LM_COLUMN_NUM          240U
#define LM_ROW_NUM             160U

int32_t lm_open(int32_t flag, int32_t mode);
int32_t lm_write(const uint8_t buf[], uint32_t count);
int32_t lm_ioctl(uint32_t op, void* arg);

#endif
