#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_
#include "fcntl.h"


#define LANG_ZH_CN 0x01 //简体中文
#define LANG_ZH_TW 0x02 //繁体中文
#define LANG_EN    0x03 //英语
#define LANG_FR    0x04 //法语
#define LANG_SPAN  0x05 //西班牙语
#define LANG_KON   0x06 //韩语
#define LANG_JPN   0x07 //日语
#define LANG_DE    0x08 //德语
#define LANG_IT    0x09 // 意大利语

#define IS_VALID_LANG(lang) ((lang == LANG_ZH_CN) || \
                                (lang == LANG_EN) || \
                                (lang == LANG_DE) || \
                                (lang == LANG_FR) || \
                                (lang == LANG_IT))

//语言列表

#define STR_ID0001    0 //  运行记录
#define STR_ID0002    1 //  设备记录
#define STR_ID0003    2 //  通信参数
#define STR_ID0004    3 //  参数设置
#define STR_ID0005    4 //  对比度调节
#define STR_ID0006    5 //  主界面
#define STR_ID0007    6 //  日发电量
#define STR_ID0008    7 //  总发电量
#define STR_ID0009    8 //  二氧化碳减排
#define STR_ID0010    9 //  故障记录
#define STR_ID0011    10 //  功率
#define STR_ID0012    11 //  状态
#define STR_ID0013    12 //  语言
#define STR_ID0014    13  //  时间
#define STR_ID0015    14 //  恢复出厂值
#define STR_ID0016    15 //  请输入密码
#define STR_ID0017    16//  阵列电压
#define STR_ID0018    17//  阵列电流
#define STR_ID0019    18//  日期
#define STR_ID0020    19//  密码错误
#define STR_ID0021    20//  设置完成
#define STR_ID0022    21//  确认设置
#define STR_ID0023    22//  网络
#define STR_ID0024    23//  设备地址
#define STR_ID0025    24//  功能
#define STR_ID0026    25//  IP地址
#define STR_ID0027    26//  子网
#define STR_ID0028    27//  网关
#define STR_ID0029    28//  DNS地址
#define STR_ID0030    29//  服务端口
#define STR_ID0031    30//  服务器地址
#define STR_ID0032    31//  服务器端口
#define STR_ID0033    32//  起始地址
#define STR_ID0034    33//  结束地址
#define STR_ID0035    34//  短信猫
#define STR_ID0036    35//  手机号码
#define STR_ID0037    36//  定时设置
#define STR_ID0038    37//  故障发送
#define STR_ID0039    38//  测试
#define STR_ID0040    39//  无数据
#define STR_ID0041    40//  月定时发送
#define STR_ID0042    41//  发送日期
#define STR_ID0043    42//  日定时发送
#define STR_ID0044    43//  发送开始时间
#define STR_ID0045    44//  发送结束时间
#define STR_ID0046    45//  发送频率
#define STR_ID0047    46//  短信已发送
#define STR_ID0048    47//  请输入号码
#define STR_ID0049    48//  开启
#define STR_ID0050    49//  关闭
#define STR_ID0051    50//  请稍等
#define STR_ID0052    51//  正在检测设备
#define STR_ID0053    52//  运行
#define STR_ID0054    53//  直流过压
#define STR_ID0055    54//  电网过压
#define STR_ID0056    55//  电网欠压
#define STR_ID0057    56//  变压器过温
#define STR_ID0058    57//  孤岛故障
#define STR_ID0059    58//  硬件故障
#define STR_ID0060    59//  模块故障
#define STR_ID0061    60//  接触器故障
#define STR_ID0062    61//  停机
#define STR_ID0063    62//  按键关机
#define STR_ID0064    63//  待机
#define STR_ID0065    64//  紧急停机
#define STR_ID0066    65//  启动中
#define STR_ID0067    66//  电网过频
#define STR_ID0068    67//  电网欠频
#define STR_ID0069    68//  直流母线过压
#define STR_ID0070    69//  直流母线欠压
#define STR_ID0071    70//  逆变过压
#define STR_ID0072    71//  输出过载
#define STR_ID0073    72//  蓄电池过压
#define STR_ID0074    73//  蓄电池欠压
#define STR_ID0075    74//  频率异常
#define STR_ID0076    75//  接地故障
#define STR_ID0077    76//  初始待机
#define STR_ID0078    77//  防雷器状态
#define STR_ID0079    78//  输入
#define STR_ID0080    79//  测试发送
#define STR_ID0081    80//  请查收短信
#define STR_ID0082    81//  开始
#define STR_ID0083    82//  地址设置
#define STR_ID0084    83//  日
#define STR_ID0085    84//  正常
#define STR_ID0086    85//  故障
#define STR_ID0087    86//  获取IP地址
#define STR_ID0088    87//  故障
#define STR_ID0089    88//  获取IP地址
#define STR_ID0090    89//  清除日志
#define STR_ID0091    90//  地址改变
#define STR_ID0092    91//  是否删除记录

#define STR_LAST STR_ID0092

extern const char *MenuStr_zh[];
extern const char *MenuStr_en[];


bool set_language(uint8_t lang);  /* 设置系统语言 */
uint8_t get_language(void);       /* 获取系统语言 */

#endif

