/****************************************************************************** 
* Copyright (C), 1997-2012, SUNGROW POWER SUPPLY CO., LTD. 
* File name      :main.c
* Author         :llemmx
* Date           :2012-08-07
* Description    :Base64编解码文件。
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2012-08-07 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const uint8_t encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                  'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                  '4', '5', '6', '7', '8', '9', '+', '/'};
/*
|+ |, |- |. |/ |0 |1 |2 |3 |4 |5 |6 |7 |8 |9 |: |; |< |= |> |? |@ |A |B |C |D |
 62 64 64 64 63 52 53 54 55 56 57 58 59 60 61 64 64 64 64 64 64 64 00 01 02 03
|E |F |G |H |I |J |K |L |M |N |O |P |Q |R |S |T |U |V |W |X |Y |Z |[ |\ |] |^ |
 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 64 64 64 64
|_ |` |a |b |c |d |e |f |g |h |i |j |k |l |m |n |o |p |q |r |s |t |u |v |w |x |
 64 64 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49
|y |z |
 50 51
*/
const uint8_t decoding_table[] = {
    62,64,64,64,63,52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,00,01,02,03,
    04,05,06,07, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,
    64,64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,
    50,51
};
const int32_t mod_table[] = {0, 2, 1};

/******************************************************************************
* Function       :get_encodesize
* Author         :llemmx
* Date           :2012-08-07
* Description    :事件捕获函数，主要负责设备一些查询消息的捕获，以及分配，消息有优先级
                  以及屏蔽位的概念。本函数并不直接处理数据，而是将消息捕获后转发出去。
* Calls          :无 
* Table Accessed :无
* Table Updated  :无
* Input          :msg - 需要调度的消息队列指针
* Output         :msg - 需要调度的消息队列指针
* Return         :返回1表示属于本模块的地址范围，0表示不属于本模块地址范围
*******************************************************************************
* History:        2011-05-03 
*------------------------------------------------------------------------------
* 2011-05-03 : 1.0.0 : llemmx
* Modification   : 创建
*------------------------------------------------------------------------------
******************************************************************************/
uint32_t get_encodesize(uint32_t in_len)
{
    uint32_t ret;
    ret=in_len/3;
    if ((in_len%3)!=0){
        ret++;
    }
    ret*=4;
    return ret;
}

uint32_t get_decodesize(uint32_t in_len)
{
    uint32_t ret;
    
    ret=in_len/4*3;
    return ret;
}

int base64_encode(const uint8_t *data,uint32_t in_len,uint8_t *out_data,uint32_t out_len)
{
    int index=0;
    if (data==NULL || out_data==NULL){
        return -1;
    }
    if (out_len<get_encodesize(in_len)){
        return -1;
    }
    
    for (int i = 0; i < in_len;) {

        uint32_t octet_a = i < in_len ? data[i++] : 0;
        uint32_t octet_b = i < in_len ? data[i++] : 0;
        uint32_t octet_c = i < in_len ? data[i++] : 0;
        uint32_t triple = (octet_a<<16) | (octet_b<<8) | octet_c;

        out_data[index++] = encoding_table[(triple >> 18) & 0x3F];
        out_data[index++] = encoding_table[(triple >> 12) & 0x3F];
        out_data[index++] = encoding_table[(triple >>  6) & 0x3F];
        out_data[index++] = encoding_table[(triple >>  0) & 0x3F];
    }
    
    for (int i = 0; i < mod_table[in_len % 3]; i++)
        out_data[index - i - 1] = '=';

    return index;
}

int base64_decode(const uint8_t *data,uint32_t in_len,uint8_t *out_data,uint32_t out_len)
{
    if (in_len % 4 != 0){
        return -1;
    }

    if (out_len<get_decodesize(in_len)){
        return -1;
    }

    int output_length = get_decodesize(in_len);
    if ('=' == data[in_len - 1]){
        output_length--;
    }
    if ('=' == data[in_len - 2]){
        output_length--;
    }

    int index=0;
    
    for (int i = 0; i < in_len;){
        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]-'+'];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]-'+'];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]-'+'];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]-'+'];

        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | (sextet_d << 0);

        if (index < output_length){
            out_data[index++] = (triple >> 16) & 0xFF;
        }
        if (index < output_length){
            out_data[index++] = (triple >>  8) & 0xFF;
        }
        if (index < output_length){
            out_data[index++] = (triple >>  0) & 0xFF;
        }
    }

    return index;
}

