/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-11 18:19:19
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-09-12 13:07:45
 * @FilePath: \Projectc:\XingNian\XiangMu\450TongXing\CODE\TotalController\total_controller\User\app\logic_proc.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "logic_proc.h"
#include "et_os.h"
#include "et_timer.h"
#include "string.h"

// 定义指令格式
#define CMD_NO_COMMAND    0x00
#define CMD_QUERY_ADDRESS 0x01
#define CMD_FACE_YIWEI    0x02
#define CMD_FACE_ERWEI    0x03
#define CMD_FACE_FORWARD  0x04
#define CMD_FACE_BACKWARD 0x05
#define CMD_FACE_AISLE    0x06
#define CMD_CLEAR_CALL    0x07
#define CMD_EMERGENCY_STOP 0x08
#define CMD_QUERY_ALL     0x0A

#define MAX_CHAIRS 20

// 定义座椅状态结构体
typedef struct {
    uint8_t addr;
    uint8_t position;
    uint8_t call_status;
    uint8_t error_status;
} ChairStatus;
// 定义存储所有座椅状态的数组
static ChairStatus chair_statuses[MAX_CHAIRS];
// 定义当前查询的座椅索引
static uint8_t current_chair_index = 0;
// 定义上次向PIS发送数据的时间
static uint32_t current_time = 0;

static void send_command(uint8_t cmd, uint8_t addr);

static tmr_t tmr_logic_task;




static void logic_task(int timer_id, void *data)
{
    static uint8_t current_cmd = CMD_QUERY_ALL;

    // 发送一键查询命令给当前座椅
    send_command(current_cmd, current_chair_index + 1);

    // 更新到下一个座椅
    current_chair_index = (current_chair_index + 1) % MAX_CHAIRS;

    // 每秒向PIS发送数据
    if (current_time++ >= 40) {
        send_data_to_pis(NULL, 3);
        send_data_to_pis(NULL, 3);
        send_data_to_pis(NULL, 3);
        current_time=0;
    }
}

void logic_proc_init(void)
{
    // 启动定时器,每50ms执行一次logic_task
    start_rpt_tmr(&tmr_logic_task, logic_task, MS_TO_TICKS(25));
}

uint8_t xor_crc(uint8_t *puchMsg, uint16_t usDataLen)
{
	uint8_t _dat = 0;
	uint16_t i = 0;
	for(i = 0; i < usDataLen;i++)
	{
		_dat = _dat ^ puchMsg[i];
	}
	return _dat;
}

// 发送命令的函数
static void send_command(uint8_t cmd, uint8_t addr)
{
    et_uart_send_req_t uart_send_data;

    uint8_t tx_buf[7];
    tx_buf[0] = 0x7B;
    tx_buf[1] = 0x7B;
    tx_buf[2] = cmd;
    tx_buf[3] = addr;
    tx_buf[4] = xor_crc(&tx_buf[2], 2); 
    tx_buf[5] = 0x7D;
    tx_buf[6] = 0x7D;

    // 根据地址确定uart_id
    if (addr % 2 == 1) {
        uart_send_data.uart_id = LEFT_CTRL_UART;  // 假设LEFT_UART已定义为左侧UART的ID
    } else {
        uart_send_data.uart_id = RIGHT_CTRL_UART; // 假设RIGHT_UART已定义为右侧UART的ID
    }
		
    uart_send_data.payload_len = 7;
    memcpy(uart_send_data.payload_data, tx_buf, 7);
    ET_POST_REQUEST(ET_REQ_UART_SEND, &uart_send_data, sizeof(et_uart_send_req_t), NULL, NULL);
}


