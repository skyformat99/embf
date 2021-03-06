#ifndef BASE64_H_
#define BASE64_H_

#include <stdint.h>

uint32_t get_encodesize(uint32_t in_len);
uint32_t get_decodesize(uint32_t in_len);
int32_t base64_encode(const uint8_t *data,uint32_t in_len,uint8_t *out_data,uint32_t out_len);
int32_t base64_decode(const uint8_t *data,uint32_t in_len,uint8_t *out_data,uint32_t out_len);

#endif

