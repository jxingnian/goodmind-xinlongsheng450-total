/**
 * @file uart_parse.h
 * @author zhaochunyun (CY.Zhao2020@outlook.com)
 * @brief
 * @version 0.0
 * @date 2024-04-30
 * @attention
 *
 * 报文格式：
 * +--------+-----------+------+------+------+------------+------+------+-----------+
 * | 描述域 |   包头    | 长度 | 地址 | 命令 | 寄存器地址 | 数据 | 校验 |   包尾    |
 * +--------+-----------+------+------+------+------------+------+------+-----------+
 * |  长度  |     2     |  1   |  1   |  1   |    1/0     |  n   |  1   |     2     |
 * +--------+-----------+------+------+------+------------+------+------+-----------+
 * |  内容  | 0x7B 0x7B | len  | addr | cmd  | reg addr   | data | xor  | 0x7D 0x7D |
 * +--------+-----------+------+------+------+------------+------+------+-----------+
 *
 * @copyright (c) 2024 Goodmind.
 *
 */

/*******************************************************************************
 ********************* define to prevent recursive inclusion *******************
 ******************************************************************************/
#ifndef __UART_PARSE__
#define __UART_PARSE__

/*******************************************************************************
 ********************************* include files *******************************
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 ************************ exported macros and struct types *********************
 ******************************************************************************/
#define HEAD            0x7B7B
#define HEAD_SIZE       2

#define TAIL            0x7D7D
#define TAIL_SIZE       2

#define LENGTH_SIZE     1
#define ADDR_SIZE       1
#define OPCODE_SIZE     1
#define CRC_SIZE        1

#define MIN_PACKET_SIZE (HEAD_SIZE + OPCODE_SIZE + ADDR_SIZE + LENGTH_SIZE + CRC_SIZE + TAIL_SIZE)

#define MAX_PACKET_SIZE 80
#define MSG_MAX_PAYLOAD 64

typedef struct {
    unsigned recv_len;
    uint8_t recv_buf[MAX_PACKET_SIZE];
} uart_context_t;

typedef int (*cmd_process_t)(uint16_t opcode, const uint8_t *data, uint32_t len);

typedef struct {
    uint16_t opcode;
    cmd_process_t cmd_process;
} method_cmd_table_t;

/*******************************************************************************
 ******************************* exported functions ****************************
 ******************************************************************************/
void method_cmd_init(const method_cmd_table_t *pcmd_table, uint32_t table_len);

void uart1_recv_data(const uint8_t *buf, int len);
void uart2_recv_data(const uint8_t *buf, int len);
void uart4_recv_data(const uint8_t *buf, int len);
uint8_t do_spec_data_package(uint8_t *dst_buf, int opcode, const uint8_t *payload_data, uint8_t payload_data_len);

void hex_print(void *in, int len);

/*******************************************************************************
 ***************************  exported global variables ************************
 ******************************************************************************/
// extern int g_xxx;

#ifdef __cplusplus
}
#endif

#endif /* __UART_PARSE__ */

/********************************* end of file ********************************/


