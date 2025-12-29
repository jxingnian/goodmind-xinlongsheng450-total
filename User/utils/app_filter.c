#include "app_filter.h"
#include "string.h"


/* 滤波器计数 */
uint8_t g_ucaIndex[FIL_NUM] = {0};

float filtTemp[FIL_NUM][FILTER_N] = {0};

/**
 * @brief 中位值平均滤波算法
 *
 * @param value         输入值
 * @param num           舍去值的数量
 * @param value_buf     缓存区
 * @param _index        滤波器序号
 * @return float        平均值
 */
float filter(float value, int num, float value_buf[], uint8_t _index)
{
    unsigned int  i, j;
    float temp;
    float value_buf_tem[FILTER_N];
    float  sum = 0;
    if (g_ucaIndex[_index] < FILTER_N) {
        value_buf[g_ucaIndex[_index]] = value;
        g_ucaIndex[_index]++;
    } else {
        for (j = FILTER_N - 1; j > 0; j--) { //递推
            value_buf[j] = value_buf[j - 1];
        }
        value_buf[0] = value;
    }
    if (g_ucaIndex[_index] < FILTER_N - 1) {
        for (int c = 0; c < FILTER_N; c++) {
            //cout << value_buf[c] << "  ";
            sum += value_buf[c];
        }
        return (float)(sum / (g_ucaIndex[_index]));
    } else {
        memcpy(value_buf_tem, value_buf, FILTER_N * sizeof(float));
        for (j = 0; j < FILTER_N - 1; j++) { //排序
            for (i = 0; i < FILTER_N - j - 1; i++) {
                if (value_buf_tem[i] > value_buf_tem[i + 1]) {
                    temp = value_buf_tem[i];
                    value_buf_tem[i] = value_buf_tem[i + 1];
                    value_buf_tem[i + 1] = temp;
                }
            }
        }
        for (int d = (num / 2); d < (FILTER_N - (num / 2)); d++) { //求值
//      cout << value_buf_tem[d] << "  ";
            sum += value_buf_tem[d];
        }
        return (float)(sum / (FILTER_N - num));
    }
}

