/**
 * @file bsp_uart.h
 * @author zhaochunyun (CY.Zhao2020@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-04-20
 *
 * @copyright (c) 2024 Goodmind.
 *
 */
#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "usart.h"

void bsp_uart_start_recv_all(void);
void bsp_uart1_dma_send(uint8_t *data, uint8_t data_len);
void bsp_uart2_dma_send(uint8_t *data, uint8_t data_len);
void bsp_uart4_dma_send(uint8_t *data, uint8_t data_len);
int  bsp_uart_get_uart_idle(uint8_t uart_id);

#endif
