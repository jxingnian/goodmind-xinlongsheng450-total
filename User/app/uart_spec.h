/**
 * @file uart_spec.h
 * @brief 串口发送队列管理
 */
#ifndef __UART_SPEC_H__
#define __UART_SPEC_H__

#include <stdint.h>
#include "et_os.h"

#ifdef __cplusplus
extern "C" {
#endif

// LEFT_CTRL_UART 和 RIGHT_CTRL_UART 定义在 et_os.h 中

typedef struct {
    uint8_t uc_data_len;
    uint8_t uca_data[64];
} uart_send_data_t;

void uart_spec_init(void);
int32_t push_uart_send_data(int uart_id, uart_send_data_t *data);
int tmr_uart_send_rb_timeout(int timer_id, void *data);
void uart_apec_send_data(int uart_id, const uint8_t *payload_data, uint8_t payload_len);

#ifdef __cplusplus
}
#endif

#endif
