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
#include "uart_parse.h"
/*******************************************************************************
 * private define macro and struct types
 ******************************************************************************/#define LOG_INFO(...)   log_info(__VA_ARGS__)
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

typedef struct {
    unsigned recv_len;
    uint8_t recv_buf[MAX_PACKET_SIZE];
    const method_cmd_table_t *cmd_table;
    uint32_t cmd_table_len;
} uart_context_t;


/*******************************************************************************
 * private function prototypes
 ******************************************************************************/

/* 异或校验 */
static uint8_t calculate_xor(uint8_t *data, uint16_t length);
/* 指令解析函数 */
static void uart_recv_data(uart_context_t *ctx, const uint8_t *buf, int len);
/* 解析完成对数据指令进行处理 */
static void do_spec_data_process(const method_cmd_table_t *cmd_table,
                                 uint32_t cmd_table_len, uint8_t *buf, int len);


/*******************************************************************************
 * private variables
 ******************************************************************************/

static uart_context_t huart_left_ctrl_ctx = {0};
static uart_context_t huart_right_ctrl_ctx = {0};

/*******************************************************************************
 * external variables and functions
 ******************************************************************************/
/* extern const method_cmd_table_t cmd_table[]; */

/*******************************************************************************
 *******************************************************************************
 * private application code, functions definitions
 *******************************************************************************
 ******************************************************************************/

void uart2_recv_data(const uint8_t *buf, int len)
{
    uart_recv_data(&huart_left_ctrl_ctx, buf, len);
}
void uart4_recv_data(const uint8_t *buf, int len)
{
    uart_recv_data(&huart_right_ctrl_ctx, buf, len);
}

void method_cmd_init(UART_HandleTypeDef *uart,
                     const method_cmd_table_t *pcmd_table, uint32_t table_len)
{
    if (uart == &huart2) {
        huart_left_ctrl_ctx.cmd_table = pcmd_table;
        huart_left_ctrl_ctx.cmd_table_len = table_len;
    } else if (uart == &huart4) {
        huart_right_ctrl_ctx.cmd_table = pcmd_table;
        huart_right_ctrl_ctx.cmd_table_len = table_len;
    }
}

uint8_t do_spec_data_package(uint8_t *dst_buf, uint8_t addr, int opcode, const uint8_t *payload_data, uint8_t payload_data_len)
{
    uint8_t len = 0;

    /* clear send data */
    uint8_t *pdata = dst_buf;

    /* head */
    *(pdata + len) = BYTE1(HEAD);
    *(pdata + len + 1) = BYTE0(HEAD);
    len += 2;

    /* package len*/
    *(pdata + len) = payload_data_len + 4; // 除包头包尾的长度
    len += 1;

    /* addr */
    *(pdata + len) = addr;
    len += 1;

    /* data type 命令 */
    *(pdata + len) = BYTE0(opcode);
    len += 1;

    /* payload 寄存器地址 + 数据 */
    memcpy(pdata + len, payload_data, payload_data_len);
    len += payload_data_len;

    /* 异或校验 */
    uint8_t  crc = calculate_xor(pdata + 2, len - 2);
    *(pdata + len) = crc;
    len += 1;

    /*end*/
    *(pdata + len) = BYTE1(TAIL);
    *(pdata + len + 1) = BYTE0(TAIL);
    len += 2;

    return len;
}

/**
 * @brief 对接收到的数据进行解析
 *
 * @param ctx 串口上下文
 * @param buf 接收到的数据指针
 * @param len 接收到的数据长度
 */
static void uart_recv_data(uart_context_t *ctx, const uint8_t *buf, int len)
{
    uint8_t head[HEAD_SIZE] = HEAD_DEF;

    int off_head = 0;
    int copy_len = 0;

copy_data:

    if (len == 0) {
        LOG_DEBUG("no available data, return!\n");
        return;
    }

    /* 拷贝最小能放下的数据到缓存区 */
    copy_len = MIN(sizeof(ctx->recv_buf) - ctx->recv_len, len);
    if (copy_len) {
        memcpy(ctx->recv_buf + ctx->recv_len, buf, copy_len);
        buf += copy_len;
        len -= copy_len;
        ctx->recv_len += copy_len;

        LOG_DEBUG("--->copy_len: %d, recv_len: %d\n", copy_len, ctx->recv_len);
    }

find_head:

    if (ctx->recv_len < MIN_PACKET_SIZE) {
        LOG_DEBUG("local buffer size[%d] too few, get more\n", ctx->recv_len);
        // hex_print(HEADER_PTR, ctx->recv_len);

        goto copy_data;
    }

    off_head = 0;

    /* 找帧头 */
    while (off_head <= ctx->recv_len - HEAD_SIZE &&
            memcmp(ctx->recv_buf + off_head, head, sizeof(head))) {
        off_head ++;
        LOG_DEBUG("off_head: %d, recv_len: %d, min_size: %d\n", off_head,
                  ctx->recv_len, MIN_PACKET_SIZE);
    }

    /* 舍去偏移 */
    if (off_head) {
        LOG_DEBUG("dropping all bytes before head match\n");
        memmove(ctx->recv_buf, ctx->recv_buf + off_head,
                ctx->recv_len - off_head);
        ctx->recv_len -= off_head;
    }

    if (ctx->recv_len < MIN_PACKET_SIZE) {
        LOG_DEBUG("available data too few, goto copy..\r\n");
        goto copy_data;
    }

    LOG_DEBUG("PACKET_LEN: %d, MSG_MAX_PAYLOAD: %d\n",
              PACKET_LEN, MSG_MAX_PAYLOAD);
    /* length */
    if (PACKET_HEAD != HEAD || PAYLOAD_LEN > MSG_MAX_PAYLOAD) {
        LOG_DEBUG("invalid header[%x] or length[%d], drop head\n",
                  PACKET_HEAD, PAYLOAD_LEN);

        memmove(ctx->recv_buf, ctx->recv_buf + HEAD_SIZE,
                ctx->recv_len - HEAD_SIZE);

        ctx->recv_len -= HEAD_SIZE;

        goto find_head;
    }

    /* PACKET_LEN */
    if (ctx->recv_len < PACKET_LEN) {

        if (ctx->recv_len == sizeof(ctx->recv_buf)) {

            LOG_DEBUG("message size exceeds receive buffer size, corrupted?\n");

            memmove(ctx->recv_buf, ctx->recv_buf + HEAD_SIZE,
                    ctx->recv_len - HEAD_SIZE);

            ctx->recv_len -= HEAD_SIZE;

            goto find_head;

        } else {
            LOG_DEBUG("recv_len < PACKET_LEN, goto copy..\r\n");
            goto copy_data;
        }
    }

    /* tail */
    if (PACKET_TAIL != TAIL) {
        LOG_DEBUG("invalid tail[%x], drop head, find head again!\n",
                  PACKET_TAIL);

        memmove(ctx->recv_buf, ctx->recv_buf + HEAD_SIZE,
                ctx->recv_len - HEAD_SIZE);

        ctx->recv_len -= HEAD_SIZE;

        goto find_head;
    }

    /* 异或值 */
    uint8_t calc_crc = calculate_xor(LENGTH_PTR, PAYLOAD_LEN - 1);
    if (CRC_VAL != calc_crc) {
        LOG_DEBUG("invalid crc[%02x - %02x], drop head, find head again\n",
                  calc_crc, CRC_VAL);

        memmove(ctx->recv_buf, ctx->recv_buf + HEAD_SIZE,
                ctx->recv_len - HEAD_SIZE);

        ctx->recv_len -= HEAD_SIZE;

        goto find_head;
    }

    uint32_t msg_len = PACKET_LEN;
    /* LOG_DEBUG("msg_len: %d, recv_len: %d\n", msg_len, ctx->recv_len); */

    do_spec_data_process(ctx->cmd_table, ctx->cmd_table_len,
                         HEADER_PTR, msg_len);
    /* cache remaining data */
    if (ctx->recv_len - msg_len) {
        memmove(ctx->recv_buf, ctx->recv_buf + msg_len, ctx->recv_len - msg_len);
    }

    ctx->recv_len -= msg_len;
    LOG_DEBUG("retry goto find head, msg len: %d, recv_len: %d\n",
              msg_len, ctx->recv_len);

    goto find_head;
}
static void do_spec_data_process(const method_cmd_table_t *cmd_table,
                                 uint32_t cmd_table_len, uint8_t *buf, int len)
{
#define _LENGTH_PTR     (&(buf[0]) + HEAD_SIZE)
#define _PAYLOAD_LEN    ((uint16_t)_LENGTH_PTR[0])
#define _ADDR_PTR       (_LENGTH_PTR + LENGTH_SIZE)
#define _ADDR_VAL       (_ADDR_PTR[0])
#define _OPCODE_PTR     (_ADDR_PTR + ADDR_SIZE)
#define _OPCODE_VAL     ((uint16_t)(_OPCODE_PTR[0]))
#define _DATA_PTR       (_OPCODE_PTR + OPCODE_SIZE)

    /* LOG_INFO("------> [stm] do_spec_data_process: "); */
    /* hex_print(buf, len); */
    /* LOG_INFO("opcode: 0x%04x, len: %d \r\n ", _OPCODE_VAL, _PAYLOAD_LEN); */

    if (NULL == cmd_table) {
        return;
    }

    for (uint16_t i = 0; i < cmd_table_len; ++i) {
        if (cmd_table[i].opcode == _OPCODE_VAL) {
            if (cmd_table[i].cmd_process) {
                cmd_table[i].cmd_process(_ADDR_VAL, _OPCODE_VAL, _DATA_PTR,
                                         _PAYLOAD_LEN - 4);
            }
        }
    }

    return;
}
static uint8_t calculate_xor(uint8_t *data, uint16_t length)
{
    uint8_t result = 0;
    for (uint16_t i = 0; i < length; i++) {
        result ^= data[i];
    }
    return result;
}
/********************************* end of file ********************************/

