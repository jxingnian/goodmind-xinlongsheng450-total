/**
 *******************************************************************************
 * @file    uart_spec.c
 * @author  xxx
 * @version 1.0.0
 * @date    2022-11-01
 * @brief   V1 2022-11-01
 *          create
 *
 *******************************************************************************
 * @attention
 *
 *
 *******************************************************************************
 */

/*******************************************************************************
 * include files
 ******************************************************************************/
#include "uart_parse.h"
#include "uart_spec.h"
#include "config.h"
#include "ring_buffer.h"
#include "app.h"
#include "et_os.h"
#include "bsp_uart.h"
#include "string.h"

/*******************************************************************************
 * private define macro and struct types
 ******************************************************************************/
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

#define TRACE_ENTER()   // log_info("[uart spec] enter: %s\r\n", __FUNCTION__)

#define OPCODE_READ_REG             0x03
#define OPCODE_WRITE_REG            0x06
#define OPCODE_OTHRE                0x09

#define UART_SEND_RB_SIZE  3

/*******************************************************************************
 * private function prototypes
 ******************************************************************************/
static int32_t pop_uart_send_data(int uart_id, uart_send_data_t *data);
static int32_t rb_uart_send_unread_pos_count(int uart_id);

/*******************************************************************************
 * external variables and functions
 ******************************************************************************/
//extern int method_read_reg(uint16_t opcode, const uint8_t *data, uint32_t len);
//extern int method_write_reg(uint16_t opcode, const uint8_t *data, uint32_t len);
//extern int method_other(uint16_t opcode, const uint8_t *data, uint32_t len);

/*******************************************************************************
 * private variables
 ******************************************************************************/

ring_buffer_t *rb_uart2_send = NULL;
ring_buffer_t *rb_uart4_send = NULL;

// static tmr_t tmr_uart_send_rb;
/*******************************************************************************
 *******************************************************************************
 * private application code, functions definitions
 *******************************************************************************
 ******************************************************************************/

/**
 * @brief 发送数据
 *
 * @param opcode 1/2/4
 * @param payload_data 数据
 * @param payload_len 数据的长度
 */
void uart_apec_send_data(int uart_id, const uint8_t *payload_data, uint8_t payload_len)
{
    if (uart_id == LEFT_CTRL_UART || uart_id == RIGHT_CTRL_UART) {
        // 发送打包信息到队列, 发送队列定时器再执行发送
        uart_send_data_t send_data;
        send_data.uc_data_len = payload_len;
        memcpy(send_data.uca_data, payload_data, payload_len);
        push_uart_send_data(uart_id, &send_data);
    }

}

void uart_spec_init(void)
{
    // 初始化三个发送队列
    rb_init(&rb_uart2_send, sizeof(uart_send_data_t) * UART_SEND_RB_SIZE);
    rb_init(&rb_uart4_send, sizeof(uart_send_data_t) * UART_SEND_RB_SIZE);
    // 启动发送队列定时器
    // start_rpt_tmr(&tmr_uart_send_rb, tmr_uart_send_rb_timeout, MS_TO_TICKS(10));
}

/*循环定时器*/
int tmr_uart_send_rb_timeout(int timer_id, void *data)
{
    int ret = 0;
    uart_send_data_t send_data;
    if (rb_uart_send_unread_pos_count(LEFT_CTRL_UART) > 0 && bsp_uart_get_uart_idle(1)) {
        if (pop_uart_send_data(LEFT_CTRL_UART, &send_data)) {
            bsp_uart2_dma_send(send_data.uca_data, send_data.uc_data_len);
            ret = 1;
        }
    }
    if (rb_uart_send_unread_pos_count(RIGHT_CTRL_UART) > 0 && bsp_uart_get_uart_idle(2)) {
        if (pop_uart_send_data(RIGHT_CTRL_UART, &send_data)) {
            bsp_uart4_dma_send(send_data.uca_data, send_data.uc_data_len);
            ret = 1;
        }
    }
    return ret;
}

int32_t push_uart_send_data(int uart_id, uart_send_data_t *data)
{
    ring_buffer_t *rb;
    switch (uart_id) {
    case LEFT_CTRL_UART:
        rb = rb_uart2_send;
        break;
    case RIGHT_CTRL_UART:
        rb = rb_uart4_send;
        break;
    default:
        return -1;
    }
    return rb_push_back(rb, (char *)data, sizeof(uart_send_data_t), 1);
}

static int32_t pop_uart_send_data(int uart_id, uart_send_data_t *data)
{
    ring_buffer_t *rb;
    switch (uart_id) {
    case LEFT_CTRL_UART:
        rb = rb_uart2_send;
        break;
    case RIGHT_CTRL_UART:
        rb = rb_uart4_send;
        break;
    default:
        return -1;
    }
    return rb_pop_front(rb, (char *)data, sizeof(uart_send_data_t), 1) / sizeof(uart_send_data_t);
}

static int32_t rb_uart_send_unread_pos_count(int uart_id)
{
    ring_buffer_t *rb;
    switch (uart_id) {
    case LEFT_CTRL_UART:
        rb = rb_uart2_send;
        break;
    case RIGHT_CTRL_UART:
        rb = rb_uart4_send;
        break;
    default:
        return -1;
    }
    uint32_t size = rb_count(rb);
    return size / sizeof(uart_send_data_t);
}

/********************************* end of file ********************************/

