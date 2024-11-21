#ifndef _LOGIC_PROC_H__
#define _LOGIC_PROC_H__

#include "uart_parse.h"
#include "et_os.h"
#include "et_timer.h"
#include "string.h"
#include "bsp_uart.h"
#include "app_pis_proc.h"
void logic_proc_init(void);

/* 定义座椅结构体 */
typedef struct {
    uint8_t seat_num;  /* 座椅编号 */
    uint8_t seat_status;  /* 座椅状态 */
} seat_t;
extern seat_t seat_ctrl_info[20];
void send_reset_call(uint8_t seat_index);
void send_seat_position_set(uint8_t seat_num, uint8_t position);
void send_seat_rotation_estop(void);
void send_seat_into_guest_mode(uint8_t mode);
void send_seat_into_meeting_mode(void);
void send_seat_align_to_direction(uint8_t direction);
/* 处理氛围灯设置请求 */
void send_ambient_light_setting(uint8_t seat_num, uint8_t light_status);
#endif
