#ifndef _LOGIC_PROC_H__
#define _LOGIC_PROC_H__

#include "uart_parse.h"
#include "et_os.h"
#include "et_timer.h"
#include "string.h"
#include "bsp_uart.h"
#include "app_pis_proc.h"
void logic_proc_init(void);

/* ¶¨Òå×ùÒÎ½á¹¹Ìå */
typedef struct {
    uint8_t seat_num;  /* ×ùÒÎ±àºÅ */
    uint8_t seat_status;  /* ×ùÒÎ×´Ì¬ */
} seat_t;
extern seat_t seat_ctrl_info[20];
void send_reset_call(seat_t seat_status);
void send_seat_position_set(uint8_t seat_num, uint8_t position);
void send_seat_rotation_estop(void);
void send_seat_into_guest_mode(uint8_t mode);
void send_seat_into_meeting_mode(void);
void send_seat_align_to_direction(uint8_t direction);

#endif
