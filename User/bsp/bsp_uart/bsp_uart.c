/**
 * @file bsp_uart.c
 * @author zhaochunyun (CY.Zhao2020@outlook.com)
 * @brief
 * @version 0.2
 * @date 2024-04-20
 * @note 在用dma发送的时候，一定要判断上一帧数据是否发完
 * @copyright (c) 2024 Goodmind.
 *
 */
#include "main.h"
#include "bsp_uart.h"
#include <stdio.h>
#include <string.h>
#include "et_os.h"

#define UART_COUNT  4

#define SEND_BUFF_SIZE  64
#define RCV_BUFF_SIZE   64

uint8_t g_ucaSendBuff[UART_COUNT][SEND_BUFF_SIZE];
uint8_t g_ucaRcvBuff[UART_COUNT][RCV_BUFF_SIZE];
static uint8_t s_flag_uart_idle[UART_COUNT] = {1, 1, 1};

void bsp_uart_start_recv_all(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, &g_ucaRcvBuff[0][0], RCV_BUFF_SIZE);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, &g_ucaRcvBuff[1][0], RCV_BUFF_SIZE);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart4, &g_ucaRcvBuff[2][0], RCV_BUFF_SIZE);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1) {
        et_uart_recv_req_t uart_recv_data = {
            .huart = huart,
            .data_len = Size,
        };
        memcpy(uart_recv_data.data, &g_ucaRcvBuff[0][0], Size);
        ET_POST_REQUEST(ET_REQ_UART_RECV, &uart_recv_data, sizeof(et_uart_recv_req_t), NULL, NULL);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, &g_ucaRcvBuff[0][0], RCV_BUFF_SIZE);
    } else if (huart == &huart2) {
        et_uart_recv_req_t uart_recv_data = {
            .huart = huart,
            .data_len = Size,
        };
        memcpy(uart_recv_data.data, &g_ucaRcvBuff[1][0], Size);
        ET_POST_REQUEST(ET_REQ_UART_RECV, &uart_recv_data, sizeof(et_uart_recv_req_t), NULL, NULL);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, &g_ucaRcvBuff[1][0], RCV_BUFF_SIZE);
    } else if (huart == &huart4) {
        et_uart_recv_req_t uart_recv_data = {
            .huart = huart,
            .data_len = Size,
        };
        memcpy(uart_recv_data.data, &g_ucaRcvBuff[2][0], Size);
        ET_POST_REQUEST(ET_REQ_UART_RECV, &uart_recv_data, sizeof(et_uart_recv_req_t), NULL, NULL);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart4, &g_ucaRcvBuff[2][0], RCV_BUFF_SIZE);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, &g_ucaRcvBuff[0][0], RCV_BUFF_SIZE);
    } else if (huart == &huart2) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, &g_ucaRcvBuff[1][0], RCV_BUFF_SIZE);
    } else if (huart == &huart4) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart4, &g_ucaRcvBuff[2][0], RCV_BUFF_SIZE);
    }
}

/**
 * @brief 串口发送完成函数
 *
 * @param huart 串口句柄
 * @note 功能： 1、告知dma发送完成，可以进行下一最数据的发送
 *             2、发送完成后可以，把485设置为接收模式
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        s_flag_uart_idle[0] = 1;
    } else if (huart == &huart2) {
        s_flag_uart_idle[1] = 1;
    } else if (huart == &huart4) {
        s_flag_uart_idle[2] = 1;
    }
}

void bsp_uart1_dma_send(uint8_t *data, uint8_t data_len)
{
    memcpy(&g_ucaSendBuff[0][0], data, data_len);
    s_flag_uart_idle[0] = 0;
    HAL_UART_Transmit_DMA(&huart1, &g_ucaSendBuff[0][0], data_len);
}

void bsp_uart2_dma_send(uint8_t *data, uint8_t data_len)
{
    memcpy(&g_ucaSendBuff[1][0], data, data_len);
    s_flag_uart_idle[1] = 0;
    HAL_UART_Transmit_DMA(&huart2, &g_ucaSendBuff[1][0], data_len);
}

void bsp_uart4_dma_send(uint8_t *data, uint8_t data_len)
{
    memcpy(&g_ucaSendBuff[2][0], data, data_len);
    s_flag_uart_idle[2] = 0;
    HAL_UART_Transmit_DMA(&huart4, &g_ucaSendBuff[2][0], data_len);
}

int  bsp_uart_get_uart_idle(uint8_t uart_id)
{
    return s_flag_uart_idle[uart_id];
}

///重定向c库函数printf到串口DEBUG_USART，重定向后可使用printf函数
int fputc(int ch, FILE *f)
{
    /* 发送一个字节数据到串口DEBUG_USART */
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);

    return (ch);
}

///重定向c库函数scanf到串口DEBUG_USART，重写向后可使用scanf、getchar等函数
int fgetc(FILE *f)
{

    int ch;
    HAL_UART_Receive(&huart1, (uint8_t *)&ch, 1, 1000);
    return (ch);
}
