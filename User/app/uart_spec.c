/*
 * @Author: 星年 && j_xingnian@163.com
 * @Date: 2025-12-29 11:26:09
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-12-29 13:08:01
 * @FilePath: \goodmind-xinlongsheng450-total\User\app\uart_spec.c
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
/**
 * @file uart_spec.c
 * @brief 串口发送队列管理
 */
#include "uart_spec.h"
#include "config.h"
#include "ring_buffer.h"
#include "bsp_uart.h"
#include <string.h>

#define UART_SEND_RB_SIZE  3

static ring_buffer_t *rb_uart2_send = NULL;
static ring_buffer_t *rb_uart4_send = NULL;

void uart_spec_init(void)
{
    rb_init(&rb_uart2_send, sizeof(uart_send_data_t) * UART_SEND_RB_SIZE);
    rb_init(&rb_uart4_send, sizeof(uart_send_data_t) * UART_SEND_RB_SIZE);
}

int32_t push_uart_send_data(int uart_id, uart_send_data_t *data)
{
    ring_buffer_t *rb = NULL;
    
    if (uart_id == LEFT_CTRL_UART) {
        rb = rb_uart2_send;
    } else if (uart_id == RIGHT_CTRL_UART) {
        rb = rb_uart4_send;
    } else {
        return -1;
    }
    
    return rb_push_back(rb, (char *)data, sizeof(uart_send_data_t), 1);
}

static int32_t pop_uart_send_data(int uart_id, uart_send_data_t *data)
{
    ring_buffer_t *rb = NULL;
    
    if (uart_id == LEFT_CTRL_UART) {
        rb = rb_uart2_send;
    } else if (uart_id == RIGHT_CTRL_UART) {
        rb = rb_uart4_send;
    } else {
        return -1;
    }
    
    return rb_pop_front(rb, (char *)data, sizeof(uart_send_data_t), 1) / sizeof(uart_send_data_t);
}

static int32_t rb_uart_send_count(int uart_id)
{
    ring_buffer_t *rb = NULL;
    
    if (uart_id == LEFT_CTRL_UART) {
        rb = rb_uart2_send;
    } else if (uart_id == RIGHT_CTRL_UART) {
        rb = rb_uart4_send;
    } else {
        return -1;
    }
    
    return rb_count(rb) / sizeof(uart_send_data_t);
}

int tmr_uart_send_rb_timeout(int timer_id, void *data)
{
    int ret = 0;
    uart_send_data_t send_data;
    
    if (rb_uart_send_count(LEFT_CTRL_UART) > 0 && bsp_uart_get_uart_idle(1)) {
        if (pop_uart_send_data(LEFT_CTRL_UART, &send_data)) {
            bsp_uart2_dma_send(send_data.uca_data, send_data.uc_data_len);
            ret = 1;
        }
    }
    
    if (rb_uart_send_count(RIGHT_CTRL_UART) > 0 && bsp_uart_get_uart_idle(2)) {
        if (pop_uart_send_data(RIGHT_CTRL_UART, &send_data)) {
            bsp_uart4_dma_send(send_data.uca_data, send_data.uc_data_len);
            ret = 1;
        }
    }
    
    return ret;
}


void uart_apec_send_data(int uart_id, const uint8_t *payload_data, uint8_t payload_len)
{
    if (uart_id == LEFT_CTRL_UART || uart_id == RIGHT_CTRL_UART) {
        uart_send_data_t send_data;
        send_data.uc_data_len = payload_len;
        memcpy(send_data.uca_data, payload_data, payload_len);
        push_uart_send_data(uart_id, &send_data);
    }
}
