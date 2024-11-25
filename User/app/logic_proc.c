/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-11 18:19:19
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-11-24 18:02:54
 * @FilePath: \total_controller\User\app\logic_proc.c
 * @Description: 逻辑处理模块，包含座椅控制和状态管理
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */

#include "logic_proc.h"

// 定义操作码
#define OPCODE_READ_REG  0x03  // 读寄存器操作码
#define OPCODE_WRITE_REG 0x06  // 写寄存器操作码
#define OPCODE_OTHRE     0x09  // 其他操作码

// 定义计算数组元素个数的宏
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

// 座椅控制信息数组
seat_t seat_ctrl_info[20];

// 声明静态函数
static int method_total_ctrl_read_reg_resp(uint8_t device_addr, uint16_t opcode,
                                           const uint8_t *data, uint32_t len);
static int method_total_ctrl_write_reg_resp(uint8_t device_addr, uint16_t opcode,
                                            const uint8_t *data, uint32_t len);
static int method_total_ctrl_other_resp(uint8_t device_addr, uint16_t opcode,
                                        const uint8_t *data, uint32_t len);

// 定义命令表
const method_cmd_table_t cmd_total_ctrl_table[] = {
    {OPCODE_READ_REG, method_total_ctrl_read_reg_resp},
    {OPCODE_WRITE_REG, method_total_ctrl_write_reg_resp},
    {OPCODE_OTHRE, method_total_ctrl_other_resp},
};

// 定义逻辑任务定时器
static tmr_t tmr_logic_task;

// 定义座椅状态结构体
typedef struct {
    uint8_t call_status;      // 座椅呼叫状态
    uint8_t current_position; // 当前旋转位置
    uint8_t error_status;     // 异常状态
    uint8_t hole_ctrl_cmd;    // 洞口控制指令：0x00-无指令，0xA1-打开，0xA2-关闭
} chair_status_t;

// 座椅状态数组
chair_status_t chair_status[SEAT_COUNT];

// 定义座椅状态报告联合体
typedef union {
    struct {
        uint8_t seat5 : 3;     // 座椅5状态，占3位
        uint8_t seat6 : 3;     // 座椅6状态，占3位
        uint8_t reserved1 : 2; // 保留位
        uint8_t seat3 : 3;     // 座椅3状态，占3位
        uint8_t seat4 : 3;     // 座椅4状态，占3位
        uint8_t reserved2 : 2; // 保留位
        uint8_t seat1 : 3;     // 座椅1状态，占3位
        uint8_t seat2 : 3;     // 座椅2状态，占3位
        uint8_t reserved3 : 2; // 保留位
    } seats;
    uint8_t buff[3];           // 3字节缓冲区，用于直接访问
} SeatStatusReport;

/**
 * @brief 设置座椅状态
 * @param report 座椅状态报告指针
 * @param seat_index 座椅索引（0-5）
 * @param status 状态值（0-7）
 */
void set_seat_status(SeatStatusReport *report, int seat_index, uint8_t status) {
    if (status > 7) return; // 状态值超出3位范围（0-7），直接返回
    switch (seat_index) {
        case 0: report->seats.seat1 = status; break;
        case 1: report->seats.seat2 = status; break;
        case 2: report->seats.seat3 = status; break;
        case 3: report->seats.seat4 = status; break;
        case 4: report->seats.seat5 = status; break;
        case 5: report->seats.seat6 = status; break;
    }
}

SeatStatusReport seat_position_report;
uint8_t call_status_report[3] = {0};
static void logic_task(int timer_id, void *data)
{
//    uint8_t send_buf[64];
    uint8_t data_buf[12];
    uint8_t data_len = 5;
    uint8_t addr;
    uint8_t opcode=0;
    static uint8_t addr_count = 0x01;
    static uint8_t Foot_hole_addr_count = 0x0;
    static uint8_t Foot_hole_addr = 0x81;
    static int32_t count;

    if (tmr_uart_send_rb_timeout(0, NULL)!=0) {
        return;
    }

    if (count%2==0) { 
        addr = addr_count;
        opcode = OPCODE_READ_REG;
        data_buf[0] = 0x05;
        data_buf[1] = 0x00;
        data_buf[2] = 0x00;
        data_buf[3] = 0x00;
        data_buf[4] = 0x00;
        data_len = 5;

        if (data_len > 0 && data_len < 40) {
            uart_send_data_t send_data;
            uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
            send_data.uc_data_len = len;

            push_uart_send_data(LEFT_CTRL_UART, &send_data);
        }
				
        if (addr_count++ >= SEAT_COUNT) {
            addr_count = 0x01;
        }
    }

    if (count%2==0) { 
        opcode = 0x03;
        data_buf[0] = 0x03;
        data_buf[1] = 0x00;
        data_buf[2] = 0x00;
        data_buf[3] = 0x00;
        data_buf[4] = 0xD1 + Foot_hole_addr_count++;

        if(Foot_hole_addr_count > 0x03){
            if(Foot_hole_addr == 0x81){
                Foot_hole_addr = 0x82;
                Foot_hole_addr_count = 0;
            }else{
                Foot_hole_addr = 0x81;
                Foot_hole_addr_count = 0;
            }
        }
    }

    static uint8_t fast_timer = 0;
    if (fast_timer >= 20) { // 1s
        fast_timer = 0;
        
        uint8_t seat_error_report[3] = {0};

        for (int i = 0; i < SEAT_COUNT; i++) {
            if (chair_status[i].call_status) {
                call_status_report[2 - i / 8] |= (1 << (i % 8));
            } else {
                call_status_report[2 - i / 8] &= ~(1 << (i % 8));
            }
            set_seat_status(&seat_position_report, i, chair_status[i].current_position);
            
            if (chair_status[i].error_status==1)
                seat_error_report[2 - i / 8] |= (1 << (i % 8));
            else {
                seat_error_report[2 - i / 8] &= ~(1 << (i % 8));
            }
						if(chair_status[i].hole_ctrl_cmd == 0xA1){
								switch (i)
								{
									case 0:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA1;

											Foot_hole_addr = 0x81;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA2;

											Foot_hole_addr = 0x81;
										}
										break;
									case 1:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA1;

											Foot_hole_addr = 0x82;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA2;

											Foot_hole_addr = 0x82;
										}
										break;
									case 2:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA2;

											Foot_hole_addr = 0x81;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA3;

											Foot_hole_addr = 0x81;
										}
										break;
									case 3:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA2;

											Foot_hole_addr = 0x82;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA3;

											Foot_hole_addr = 0x82;
										}
										break;
									case 4:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA3;

											Foot_hole_addr = 0x81;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA4;

											Foot_hole_addr = 0x81;
										}
										break;
									case 5:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA3;

											Foot_hole_addr = 0x82;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x01;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xA4;

											Foot_hole_addr = 0x82;
										}
										break;
									default:
										break;
									}
							}else if(chair_status[i].hole_ctrl_cmd == 0xA2){
								switch (i)
								{
									case 0:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC1;

											Foot_hole_addr = 0x81;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC2;

											Foot_hole_addr = 0x81;
										}
										break;
									case 1:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC1;

											Foot_hole_addr = 0x82;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC2;

											Foot_hole_addr = 0x82;
										}
										break;
									case 2:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC2;

											Foot_hole_addr = 0x81;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC3;

											Foot_hole_addr = 0x81;
										}
										break;
									case 3:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC2;

											Foot_hole_addr = 0x82;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC3;

											Foot_hole_addr = 0x82;
										}
										break;
									case 4:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC3;

											Foot_hole_addr = 0x81;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC4;

											Foot_hole_addr = 0x81;
										}
										break;
									case 5:
										if(chair_status[i].current_position == 3){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC3;

											Foot_hole_addr = 0x82;
										}else if(chair_status[i].current_position == 4){
											opcode = 0x03;
											data_buf[0] = 0x02;
											data_buf[1] = 0x00;
											data_buf[2] = 0x00;
											data_buf[3] = 0x00;
											data_buf[4] = 0xC4;

											Foot_hole_addr = 0x82;
										}
										break;
									default:
										break;
									}
						}
        }
        send_data_to_pis(0x01, call_status_report, sizeof(call_status_report));
        send_data_to_pis(0x02, seat_position_report.buff, 3);
        send_data_to_pis(0x03, seat_error_report, sizeof(seat_error_report));
    }
    fast_timer++;
    count++;
		if(opcode){
			uart_send_data_t send_data;
			uint8_t len = do_spec_data_package(send_data.uca_data, Foot_hole_addr, opcode, data_buf, data_len);
			send_data.uc_data_len = len;
			push_uart_send_data(RIGHT_CTRL_UART, &send_data);
		}
}
// 逻辑处理初始化函数
void logic_proc_init(void)
{
    // 初始化命令控制表，分别为UART2和UART4
    method_cmd_init(&huart2, cmd_total_ctrl_table, ARRAY_SIZE(cmd_total_ctrl_table));
    method_cmd_init(&huart4, cmd_total_ctrl_table, ARRAY_SIZE(cmd_total_ctrl_table));

    // 启动定时器，每50ms执行一次logic_task
    start_rpt_tmr(&tmr_logic_task, logic_task, MS_TO_TICKS(50));
}

// 异或校验和计算函数
uint8_t xor_crc(uint8_t *puchMsg, uint16_t usDataLen)
{
    uint8_t _dat = 0;
    uint16_t i = 0;
    // 遍历消息中的每个字节进行异或运算
    for (i = 0; i < usDataLen; i++) {
        _dat = _dat ^ puchMsg[i];
    }
    return _dat;
}

// 发送重置呼叫命令函数
void send_reset_call(uint8_t seat_index)
{
    uint8_t data_buf[12];  // 数据缓冲区
    uint8_t data_len = 0;  // 数据长度
    uint8_t addr;          // 设备地址
    uint8_t opcode;        // 操作码

    // 设置参数
    addr = seat_index;             // 座位索引作为地址
    opcode = OPCODE_WRITE_REG;     // 操作码设为写寄存器
    data_buf[0] = 0x06;            // 功能码：重置呼叫
    data_buf[1] = 0x00;            // 保留字节
    data_buf[2] = 0x00;            // 保留字节
    data_buf[3] = 0x00;            // 保留字节
    data_buf[4] = 0x00;            // 保留字节
    data_len = 5;                  // 设置数据长度

    // 检查数据长度是否在有效范围内
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        // 打包数据
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        // 发送数据到左侧控制UART
        push_uart_send_data(LEFT_CTRL_UART, &send_data);
    }
}

/* 调整全部座椅到指定车辆运行方向 */
void send_seat_align_to_direction(uint8_t direction)
{
    uint8_t data_buf[12];  // 数据缓冲区
    uint8_t data_len = 0;  // 数据长度
    uint8_t addr;          // 设备地址
    uint8_t opcode;        // 操作码

    /* 初始化参数 */
    addr = 0x99;                   // 设置广播地址
    opcode = OPCODE_WRITE_REG;     // 设置操作码为写寄存器
    
    /* 根据方向设置功能码 */
    if (direction == 1) {
        data_buf[0] = 0x01;        // 1位端方向
    } else {
        data_buf[0] = 0x02;        // 2位端方向
    }
    
    /* 设置剩余数据缓冲区 */
    data_buf[1] = 0x00;            // 保留字节
    data_buf[2] = 0x00;            // 保留字节
    data_buf[3] = 0x00;            // 保留字节
    data_buf[4] = 0x00;            // 保留字节
    data_len = 5;                  // 设置数据长度

    /* 检查数据长度是否在有效范围内 */
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        // 打包数据
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        // 发送数据到左侧控制UART
        push_uart_send_data(LEFT_CTRL_UART, &send_data);
        // 注释掉的右侧控制UART发送
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}

/* 调整座椅进入会议模式 */
void send_seat_into_meeting_mode(void)
{
    uint8_t data_buf[12];  // 数据缓冲区
    uint8_t data_len = 0;  // 数据长度
    uint8_t addr;          // 设备地址
    uint8_t opcode;        // 操作码

    /* 初始化参数 */
    addr = 0x99;                   // 设置广播地址
    opcode = OPCODE_WRITE_REG;     // 设置操作码为写寄存器
    data_buf[0] = 0x05;            // 功能码：进入会议模式
    data_buf[1] = 0x00;            // 保留字节
    data_buf[2] = 0x00;            // 保留字节
    data_buf[3] = 0x00;            // 保留字节
    data_buf[4] = 0x00;            // 保留字节
    data_len = 5;                  // 设置数据长度

    /* 检查数据长度是否在有效范围内 */
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        // 打包数据
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        // 发送数据到左侧控制UART
        push_uart_send_data(LEFT_CTRL_UART, &send_data);
        // 注释掉的右侧控制UART发送
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}

/* 设置座椅进入客人模式 */
void send_seat_into_guest_mode(uint8_t mode)
{
    uint8_t data_buf[12];  // 数据缓冲区
    uint8_t data_len = 0;  // 数据长度
    uint8_t addr;          // 设备地址
    uint8_t opcode;        // 操作码

    /* 初始化参数 */
    addr = 0x99;                   // 设置广播地址
    opcode = OPCODE_WRITE_REG;     // 设置操作码为写寄存器
    data_buf[0] = 0x09;            // 功能码：设置客人模式
    data_buf[1] = mode;            // 客人模式参数
    data_buf[2] = 0x00;            // 保留字节
    data_buf[3] = 0x00;            // 保留字节
    data_buf[4] = 0x00;            // 保留字节
    data_len = 5;                  // 设置数据长度

    /* 检查数据长度是否在有效范围内 */
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        // 打包数据
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        // 发送数据到左侧控制UART
        push_uart_send_data(LEFT_CTRL_UART, &send_data);
        // 注释掉的右侧控制UART发送
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}

/* 发送座椅旋转急停命令 */
void send_seat_rotation_estop(void)
{
    uint8_t data_buf[12];  // 数据缓冲区
    uint8_t data_len = 0;  // 数据长度
    uint8_t addr;          // 设备地址
    uint8_t opcode;        // 操作码

    /* 初始化参数 */
    addr = 0x99;                   // 设置广播地址
    opcode = OPCODE_WRITE_REG;     // 设置操作码为写寄存器
    data_buf[0] = 0x07;            // 功能码：座椅旋转急停
    data_buf[1] = 0x00;            // 保留字节
    data_buf[2] = 0x00;            // 保留字节
    data_buf[3] = 0x00;            // 保留字节
    data_buf[4] = 0x00;            // 保留字节
    data_len = 5;                  // 设置数据长度

    /* 检查数据长度是否在有效范围内 */
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        // 打包数据
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        // 发送数据到左侧控制UART
        push_uart_send_data(LEFT_CTRL_UART, &send_data);
        // 注释掉的右侧控制UART发送
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}

/* 设置单个座椅的位置 */
void send_seat_position_set(uint8_t seat_num, uint8_t position)
{
    uint8_t data_buf[12];  // 数据缓冲区
    uint8_t data_len = 0;  // 数据长度
    uint8_t addr;          // 座椅地址
    uint8_t opcode;        // 操作码

    /* 设置座椅地址和操作码 */
    addr = seat_num;
    opcode = OPCODE_WRITE_REG;  // 写寄存器操作

    /* 根据位置设置相应的命令 */
    switch (position) {
    case 0: 
        printf("设置到1位端回行方向\n");
        data_buf[0] = 0x03;
        break;
    case 1: 
        printf("设置到2位端回行方向\n");
        data_buf[0] = 0x04;
        break;
    case 2: 
        printf("设置到会议模式\n");
        data_buf[0] = 0x05;
        break;
    case 3: 
        printf("设置到1位端运行方向\n");
        data_buf[0] = 0x01;
        break;
    case 4: 
        printf("设置到2位端运行方向\n");
        data_buf[0] = 0x02;
        break;
    default: 
        printf("未知的座椅位置: %d\n", position);
        return;  // 未知位置，直接返回
    }

    /* 设置剩余的数据缓冲区 */
    data_buf[1] = 0x00;
    data_buf[2] = 0x00;
    data_buf[3] = 0x00;
    data_buf[4] = 0x00;
    data_len = 5;  // 设置数据长度

    /* 检查数据长度并发送数据 */
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        // 打包数据
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        // 发送数据到左侧控制UART
        push_uart_send_data(LEFT_CTRL_UART, &send_data);
    }
}

/* 设置呼叫状态 */
void send_ambient_light_setting(uint8_t seat_num, uint8_t light_status)
{
    uint8_t data_buf[12];  // 数据缓冲区
    uint8_t data_len = 0;  // 数据长度
    uint8_t addr;          // 设备地址
    uint8_t opcode;        // 操作码

    /* 初始化参数 */
    addr = seat_num;                // 设置设备地址为座位号
    opcode = OPCODE_WRITE_REG;      // 设置操作码为写寄存器
    data_buf[0] = 0x0A;             // 功能码：设置呼叫
    data_buf[1] = light_status;     // 环境光状态
    data_buf[2] = 0x00;             // 保留字节
    data_buf[3] = 0x00;             // 保留字节
    data_buf[4] = 0x00;             // 保留字节
    data_len = 5;                   // 设置数据长度

    /* 检查数据长度是否在有效范围内 */
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        // 打包数据
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        // 发送数据到左侧控制UART
        push_uart_send_data(LEFT_CTRL_UART, &send_data);
    }
}
/* 电机状态枚举 */
typedef enum {
    MOTOR_AISLE = 14,           /* (11) 电机移动到过道位置 */
    MOTOR_FORWARD = 15,         /* (12) 电机移动到前向位置 */
    MOTOR_REAR = 16,            /* (13) 电机移动到后向位置 */
    MOTOR_YIWEI = 17,           /* (14) 电机移动到一位端位置 */
    MOTOR_ERWEI = 18,           /* (15) 电机移动到二位端位置 */
    MOTOR_NULL = 100,           /* 无效电机状态 */
} MOTORSTATE;

/* 处理总控制器读寄存器响应 */
static int method_total_ctrl_read_reg_resp(uint8_t device_addr, uint16_t opcode,
                                           const uint8_t *data, uint32_t len)
{
    /* 更新呼叫状态 */
    chair_status[device_addr - 1].call_status = data[1];

    /* 根据电机状态更新当前位置 */
    switch ((MOTORSTATE)data[3]) {
    case MOTOR_AISLE:
        chair_status[device_addr - 1].current_position = 2;  /* 过道位置 */
        break;
    case MOTOR_FORWARD:
        chair_status[device_addr - 1].current_position = 0;  /* 前向位置 */
        break;
    case MOTOR_REAR:
        chair_status[device_addr - 1].current_position = 1;  /* 后向位置 */
        break;
    case MOTOR_YIWEI:
        chair_status[device_addr - 1].current_position = 3;  /* 一位端位置 */
        break;
    case MOTOR_ERWEI:
        chair_status[device_addr - 1].current_position = 4;  /* 二位端位置 */
        break;
    default:
        /* 未知位置，不更新 */
        break;
    }

    /* 更新错误状态和洞控制命令 */
    chair_status[device_addr - 1].error_status = data[2];
    chair_status[device_addr - 1].hole_ctrl_cmd = data[4];

    return 0;
}

/* 处理总控制器写寄存器响应 */
static int method_total_ctrl_write_reg_resp(uint8_t device_addr, uint16_t opcode,
                                            const uint8_t *data, uint32_t len)
{
    /* 暂无具体实现 */
    return 0;
}

/* 处理总控制器其他响应 */
static int method_total_ctrl_other_resp(uint8_t device_addr, uint16_t opcode,
                                        const uint8_t *data, uint32_t len)
{
    /* 暂无具体实现 */
    return 0;
}
