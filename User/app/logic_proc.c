/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-11 18:19:19
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-11-22 16:05:49
 * @FilePath: \total_controller\User\app\logic_proc.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include "logic_proc.h"
#define OPCODE_READ_REG             0x03
#define OPCODE_WRITE_REG            0x06
#define OPCODE_OTHRE                0x09
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

seat_t seat_ctrl_info[20];
static int method_total_ctrl_read_reg_resp(uint8_t device_addr, uint16_t opcode,
                                           const uint8_t *data, uint32_t len);
static int method_total_ctrl_write_reg_resp(uint8_t device_addr, uint16_t opcode,
                                            const uint8_t *data, uint32_t len);
static int method_total_ctrl_other_resp(uint8_t device_addr, uint16_t opcode,
                                        const uint8_t *data, uint32_t len);

const method_cmd_table_t cmd_total_ctrl_table[] = {
    {OPCODE_READ_REG, method_total_ctrl_read_reg_resp},
    {OPCODE_WRITE_REG, method_total_ctrl_write_reg_resp},
    {OPCODE_OTHRE, method_total_ctrl_other_resp},
};

static tmr_t tmr_logic_task;

typedef struct {
    uint8_t call_status;         // 座椅呼叫状态
    uint8_t current_position;    // 当前旋转位置
    uint8_t error_status;        // 异常状态
} chair_status_t;
chair_status_t chair_status[SEAT_COUNT];

typedef union {
    struct {
        uint8_t seat5 : 3;  // 座椅3状态，占3位
        uint8_t seat6 : 3;  // 座椅4状态，占3位
        uint8_t reserved1 : 2;
        uint8_t seat3 : 3;  // 座椅5状态，占3位
        uint8_t seat4 : 3;  // 座椅6状态，占3位
        uint8_t reserved2 : 2;
        uint8_t seat1 : 3;  // 座椅7状态，占3位
        uint8_t seat2 : 3;  // 座椅8状态，占3位
        uint8_t reserved3 : 2;
    } seats;

    uint8_t buff[3]; // 直接访问整个 3 字节数据
} SeatStatusReport;

// 设置座椅状态的辅助函数
void set_seat_status(SeatStatusReport *report, int seat_index, uint8_t status) {
    if (status > 7) return; // 状态超过3位范围（0-7），直接返回
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
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;
    static uint8_t addr_count = 0x01;
    static uint8_t Foot_hole_addr_count = 0x0;
    static uint8_t Foot_hole_addr = 0x81;
    static int32_t count;

    /* 判断发送队列是否有数据发送 */
    if (tmr_uart_send_rb_timeout(0, NULL)!=0) {
        // 等待发送完成
        return;
    }

    /* 发送查询主控数据 */
    if (count%2==0) { // 100ms发送一次
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

    /* 发送查询脚洞数据 */
    if (count%2==0) { // 100ms发送一次
        opcode = 0x03;
        data_buf[0] = 0x05;
        data_buf[1] = 0x00;
        data_buf[2] = 0x00;
        data_buf[3] = 0x00;
        data_buf[4] = 0xD1 + Foot_hole_addr_count++;
        data_len = 5;

        if (data_len > 0 && data_len < 40) {
            uart_send_data_t send_data;
            uint8_t len = do_spec_data_package(send_data.uca_data, Foot_hole_addr, opcode, data_buf, data_len);
            send_data.uc_data_len = len;
            push_uart_send_data(RIGHT_CTRL_UART, &send_data);
        }

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
        
      
        
//        uint8_t seat_position_report[3] = {0};
        uint8_t seat_error_report[3] = {0};

        for (int i = 0; i < SEAT_COUNT; i++) {
            if (chair_status[i].call_status) {
                call_status_report[2 - i / 8] |= (1 << (i % 8));
            } else {
                call_status_report[2 - i / 8] &= ~(1 << (i % 8));
            }
            set_seat_status(&seat_position_report, i, chair_status[i].current_position);
//            seat_position_report[i / 8] |= (chair_status[i].current_position & 0x07) << ((i % 8) * 3);
            
            if (chair_status[i].error_status==1)
                seat_error_report[2 - i / 8] |= (1 << (i % 8));
            else {
                seat_error_report[2 - i / 8] &= ~(1 << (i % 8));
            }
        }
        send_data_to_pis(0x01, call_status_report, sizeof(call_status_report));
        send_data_to_pis(0x02, seat_position_report.buff, 3);
        send_data_to_pis(0x03, seat_error_report, sizeof(seat_error_report));
    }
    fast_timer++;
    count++;
}
void logic_proc_init(void)
{
    method_cmd_init(&huart2, cmd_total_ctrl_table, ARRAY_SIZE(cmd_total_ctrl_table));
    method_cmd_init(&huart4, cmd_total_ctrl_table, ARRAY_SIZE(cmd_total_ctrl_table));

    // 启动定时器,每50ms执行一次logic_task
    start_rpt_tmr(&tmr_logic_task, logic_task, MS_TO_TICKS(50));
}

uint8_t xor_crc(uint8_t *puchMsg, uint16_t usDataLen)
{
    uint8_t _dat = 0;
    uint16_t i = 0;
    for (i = 0; i < usDataLen; i++) {
        _dat = _dat ^ puchMsg[i];
    }
    return _dat;
}

/* 清除呼叫 */
void send_reset_call(uint8_t seat_index)
{
    uint8_t data_buf[12];
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;

    /* code */
    addr = seat_index;
    opcode = OPCODE_WRITE_REG;
    data_buf[0] = 0x06;
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
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }

}

/* 调整全部座椅到指定车辆运行方向 */
void send_seat_align_to_direction(uint8_t direction)
{
    uint8_t data_buf[12];
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;

    /* code */
    addr = 0x99;
    opcode = OPCODE_WRITE_REG;
    if (direction == 1) {
        data_buf[0] = 0x01;
    } else {
        data_buf[0] = 0x02;
    }
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
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}
/* 进入会议模式 */
void send_seat_into_meeting_mode(void)
{
    uint8_t data_buf[12];
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;

    /* code */
    addr = 0x99;
    opcode = OPCODE_WRITE_REG;
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
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}

/* 会客模式 */
void send_seat_into_guest_mode(uint8_t mode)
{
    uint8_t data_buf[12];
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;

    /* code */
    addr = 0x99;
    opcode = OPCODE_WRITE_REG;
    data_buf[0] = 0x09;
    data_buf[1] = mode;
    data_buf[2] = 0x00;
    data_buf[3] = 0x00;
    data_buf[4] = 0x00;
    data_len = 5;
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        push_uart_send_data(LEFT_CTRL_UART, &send_data);
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}

/* 处理座椅旋转急停请求 */
void send_seat_rotation_estop(void)
{
    uint8_t data_buf[12];
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;

    /* code */
    addr = 0x99;
    opcode = OPCODE_WRITE_REG;
    data_buf[0] = 0x07;
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
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}
/* 处理单个座椅位置设置请求 */
void send_seat_position_set(uint8_t seat_num, uint8_t position)
{
    uint8_t data_buf[12];
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;

    /* code */
    addr = seat_num;
    opcode = OPCODE_WRITE_REG;
    switch (position) {
    case 0: printf("1位端会客朝向\n");
        data_buf[0] = 0x03;
        break;
    case 1: printf("2位端会客朝向\n");
        data_buf[0] = 0x04;
        break;
    case 2: printf("过道会客朝向\n");
        data_buf[0] = 0x05;
        break;
    case 3: printf("1位端正常朝向\n");
        data_buf[0] = 0x01;
        break;
    case 4: printf("2位端正常朝向\n");
        data_buf[0] = 0x02;
        break;
    default: printf("未知的座椅位置: %d\n", position);
        return;
    }
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
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }

}

/* 处理氛围灯设置请求 */
void send_ambient_light_setting(uint8_t seat_num, uint8_t light_status)
{
    uint8_t data_buf[12];
    uint8_t data_len = 0;
    uint8_t addr;
    uint8_t opcode;

    /* code */
    addr = seat_num;
    opcode = OPCODE_WRITE_REG;
    data_buf[0] = 0x0A;
    data_buf[1] = light_status;
    data_buf[2] = 0x00;
    data_buf[3] = 0x00;
    data_buf[4] = 0x00;
    data_len = 5;
    if (data_len > 0 && data_len < 40) {
        uart_send_data_t send_data;
        uint8_t len = do_spec_data_package(send_data.uca_data, addr, opcode, data_buf, data_len);
        send_data.uc_data_len = len;

        push_uart_send_data(LEFT_CTRL_UART, &send_data);
        // push_uart_send_data(RIGHT_CTRL_UART, &send_data);
    }
}

/* 电机状态 */
typedef enum {
    MOTOR_AISLE = 14,           /* (11) 电机运动面向过道 */
    MOTOR_FORWARD = 15,         /* (12) 电机运动面向前方 */
    MOTOR_REAR = 16,            /* (13) 电机运动面向后方 */
    MOTOR_YIWEI = 17,           /* (14) 电机运动面向一位端 */
    MOTOR_ERWEI = 18,           /* (15) 电机运动面向二位端 */
    MOTOR_NULL = 100,
} MOTORSTATE;

static int method_total_ctrl_read_reg_resp(uint8_t device_addr, uint16_t opcode,
                                           const uint8_t *data, uint32_t len)
{
    chair_status[device_addr - 1].call_status = data[1];
    switch ((MOTORSTATE)data[3]) {
    case MOTOR_AISLE:
        chair_status[device_addr - 1].current_position = 2;
        break;
        
    case MOTOR_FORWARD:
        chair_status[device_addr - 1].current_position = 0;
        break;
    case MOTOR_REAR:
        chair_status[device_addr - 1].current_position = 1;
        break;
    case MOTOR_YIWEI:
        chair_status[device_addr - 1].current_position = 3;
        break;
    case MOTOR_ERWEI:
        chair_status[device_addr - 1].current_position = 4;
        break;
    default:
        break;
    }
    chair_status[device_addr - 1].error_status = data[2];
    return 0;
}
static int method_total_ctrl_write_reg_resp(uint8_t device_addr, uint16_t opcode,
                                            const uint8_t *data, uint32_t len)
{

    return 0;
}
static int method_total_ctrl_other_resp(uint8_t device_addr, uint16_t opcode,
                                        const uint8_t *data, uint32_t len)
{
    return 0;
}

