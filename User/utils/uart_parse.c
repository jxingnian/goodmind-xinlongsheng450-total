/**
 * @file uart_parse.c
 * @author zhaochunyun (CY.Zhao2020@outlook.com)
 * @brief
 * @version 0.0
 * @date 2024-04-30
 *
 * @copyright (c) 2024 Goodmind.
 *
 */
/*******************************************************************************
 * include files
 ******************************************************************************/
#include <string.h>
#include "uart_parse.h"
#include "config.h"
#include "et_os.h"
/*******************************************************************************
 * private define macro and struct types
 ******************************************************************************/
#define LOG_INFO(...)   log_info(__VA_ARGS__)
#define LOG_DEBUG(...)  log_dbg(__VA_ARGS__)

#define MIN(a, b)       (((a) < (b)) ? (a) : (b))

#define BYTE0(x)        ((uint8_t)( (x) & 0x000000ff))
#define BYTE1(x)        ((uint8_t)(((x) & 0x0000ff00) >>  8))
#define BYTE2(x)        ((uint8_t)(((x) & 0x00ff0000) >> 16))
#define BYTE3(x)        ((uint8_t)(((x) & 0xff000000) >> 24))


#define HEAD_DEF       {BYTE0(HEAD), BYTE1(HEAD)}

#define TAIL_DEF       {BYTE0(TAIL), BYTE1(TAIL)}

#define HEADER_PTR      (&(ctx->recv_buf[0]))
#define PACKET_HEAD     ((uint16_t)(HEADER_PTR[0] << 8) | (uint16_t)HEADER_PTR[1])  // 帧头

#define LENGTH_PTR      (HEADER_PTR + HEAD_SIZE)                                    // 长度
#define PAYLOAD_LEN     ((uint16_t)(LENGTH_PTR[0]))                                 // 除包头包尾之外的所有数据的长度
#define ADDR_PTR        (LENGTH_PTR + LENGTH_SIZE)                                  // 地址
#define OPCODE_PTR      (ADDR_PTR + ADDR_SIZE)                                      // 命令

#define CRC_PTR         (LENGTH_PTR + PAYLOAD_LEN - 1)                              // 校验
#define CRC_VAL         ((uint16_t)(CRC_PTR[0]))
#define TAIL_PTR        (CRC_PTR + CRC_SIZE)                                        // 帧尾
#define PACKET_TAIL     ((uint16_t)(TAIL_PTR[0] << 8) | (uint16_t)TAIL_PTR[1])

#define PACKET_LEN      (HEAD_SIZE + PAYLOAD_LEN + TAIL_SIZE)


/*******************************************************************************
 * private function prototypes
 ******************************************************************************/

//static void uart_recv_data(uart_context_t *ctx, const uint8_t *buf, int len);

/*******************************************************************************
 * private variables
 ******************************************************************************/
uart_context_t uart3_ctx = {0};

/*******************************************************************************
 * external variables and functions
 ******************************************************************************/
/* extern const method_cmd_table_t cmd_table[]; */

/*******************************************************************************
 *******************************************************************************
 * private application code, functions definitions
 *******************************************************************************
 ******************************************************************************/

void uart1_recv_data(const uint8_t *buf, int len)
{
//      printf("uart1_recv_data\n");
}
void uart2_recv_data(const uint8_t *buf, int len)
{
//    uart_recv_data(&uart3_ctx, buf, len);
}
void uart4_recv_data(const uint8_t *buf, int len)
{
//    uart_recv_data(&uart3_ctx, buf, len);
}
///**
// * @brief 对接收到的数据进行解析
// *
// * @param ctx 串口上下文
// * @param buf 接收到的数据指针
// * @param len 接收到的数据长度
// */
//static void uart_recv_data(uart_context_t *ctx, const uint8_t *buf, int len)
//{
//
//}


void hex_print(void *in, int len)
{
    char hex_buf[len + 5];
    int i;
    char tmp[4] = {0};
    uint8_t *data = (uint8_t *)in;
    memset(hex_buf, 0x00, sizeof(hex_buf));
    for (i = 0; i < len; i++) {
        if (i % 16 == 0 && i) {
            LOG_INFO("%s", hex_buf);
            memset(hex_buf, 0x00, sizeof(hex_buf));
        }
        sprintf(tmp, "%02x ", data[i]);
        strcat(hex_buf, tmp);
    }
    LOG_INFO("%s", hex_buf);
    LOG_INFO("\r\n");
}

/********************************* end of file ********************************/

