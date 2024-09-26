#include <stdio.h>
#include <stdint.h>
#include "utils.h"

#define LONG_MAX 0x7fffffff
#define LONG_MIN (-LONG_MAX-1)

#define RAND_IN(delay, min, max) (((((max) - (min)) * (delay)) >> (16)) + (min))

uint32_t get_le_val(const uint8_t *p, int bytes)
{
    uint32_t ret = 0;

    while (bytes-- > 0) {
        ret <<= 8;
        ret |= *(p + bytes);
    }
    return ret;
}
uint32_t get_be_val(const uint8_t *p, int bytes)
{
    uint32_t ret = 0;
    while (bytes-- > 0) {
        ret <<= 8;
        ret |= *p++;
    }

    return ret;
}
int is_all_xx(const void *s1, uint8_t val, int n)
{
    while (n && *(uint8_t *) s1 == val) {
        s1 = (uint8_t *) s1 + 1;
        n--;
    }
    return !n;
}

void put_le_val(uint32_t val, uint8_t *p, int bytes)
{
    while (bytes-- > 0) {
        *p++ = val & 0xFF;
        val >>= 8;
    }
}

void put_be_val(uint32_t val, uint8_t *p, int bytes)
{
    while (bytes-- > 0) {
        *(p + bytes) = val & 0xFF;
        val >>= 8;
    }
}

static uint16_t crc16_update(uint16_t crc, uint8_t a)
{
    int i;

    crc ^= (uint16_t)a;
    for (i = 0; i < 8; ++i) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }

    return crc;
}

uint16_t calc_crc16(uint16_t crc, uint8_t *data, uint32_t len)
{
    //uint16_t crc = 0xffff;
    uint32_t i = 0;

    //crc = 0xFFFF;
    for (i = 0; i < len; i++)
        crc = crc16_update(crc, data[i]);

    return crc;
}
