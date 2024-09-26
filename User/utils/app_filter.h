#ifndef __APP_FILTER_H
#define __APP_FILTER_H

#include "main.h"


/* filter */
/* 连续采样N个数据 */
#define FILTER_N 10
/* 滤波器数量 */
#define FIL_NUM 1

extern float filtTemp[FIL_NUM][FILTER_N];


float filter(float value, int num, float value_buf[], uint8_t _index);


#endif
