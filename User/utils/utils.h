#ifndef _COMFUNC_H_
#define _COMFUNC_H_

#include <stdarg.h>
#include <stdbool.h>


#ifndef NULL
#define NULL 0
#endif

#define SWAP_WORD(x)  (((uint8_t)((x & 0xFF00) >> 8)) | (((uint8_t)(x)) << 8))

#define LONG_MAX 0x7fffffff
#define LONG_MIN (-LONG_MAX-1)

#define set_bit(x, bit) ((x) |= 1 << (bit))
#define clr_bit(x, bit) ((x) &= ~(1 << (bit)))
#define tst_bit(x, bit) ((x) & (1 << (bit)))
#define get_bits(val,x1,x2) (((val)>>(x1))&((1<<((x2)-(x1)+1))-1))
#define IS_POWER_OF_TWO(A) ( ((A) != 0) && ((((A) - 1) & (A)) == 0) )

#define isdigit(c) ({ int __c = (c); __c >= '0' && __c <= '9'; })

#define GET_VAL(size, paddr)                                               \
    (((size) == 1) ? *(unsigned char *)(paddr) :                   \
     (((size) == 2) ? *(unsigned short *)(paddr) : *(int *)(paddr)))


#ifndef min
#define min(a, b) ((a)<(b) ? (a):(b))
#endif
#ifndef max
#define max(a, b) ((a)>(b) ? (a):(b))
#endif

#define array_size(array) (sizeof(array)/sizeof(*array))

#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#ifndef offset_of
# define offset_of(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)
#endif


uint32_t get_le_val(const uint8_t *p, int bytes);
uint32_t get_be_val(const uint8_t *p, int bytes);
int is_all_xx(const void *s1, uint8_t val, int n);
void put_le_val(uint32_t val, uint8_t *p, int bytes);
void put_be_val(uint32_t val, uint8_t *p, int bytes);
uint16_t calc_crc16(uint16_t crc, uint8_t *data, uint32_t len);


#endif
