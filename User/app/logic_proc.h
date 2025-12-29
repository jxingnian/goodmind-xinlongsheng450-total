/*
 * @Author: 星年 && j_xingnian@163.com
 * @Date: 2025-12-29 11:26:09
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-12-29 13:28:43
 * @FilePath: \goodmind-xinlongsheng450-total\User\app\logic_proc.h
 * @Description: 业务逻辑处理
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef _LOGIC_PROC_H_
#define _LOGIC_PROC_H_

#include <stdint.h>
#include "et_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 座椅数量 */
#define SEAT_COUNT  6

/* 命令定义 */
#define CMD_READ   0x03  // 读
#define CMD_WRITE  0x06  // 写
#define CMD_OTHER  0x09  // 其他

/* 寄存器定义 - 读 */
#define REG_SEAT_STATUS      0x01  // 读取座椅旋转状态
#define REG_CALL_STATUS      0x02  // 读取呼叫状态
#define REG_LIGHT_STATUS     0x03  // 读取氛围灯状态

/* 寄存器定义 - 写 */
#define REG_SEAT_ROTATE      0x30  // 控制座椅旋转
#define REG_SEAT_RESET       0x31  // 座椅复位(回到坐姿)
#define REG_CALL_CLEAR       0x32  // 清除呼叫
#define REG_LIGHT_CTRL       0x33  // 控制氛围灯
#define REG_SEAT_ESTOP       0x34  // 座椅急停

/* 座椅状态 */
typedef enum {
    SEAT_STATUS_REST  = 0x00,  // 复位/空闲
    SEAT_STATUS_RUN   = 0x01,  // 运行中
    SEAT_STATUS_ERROR = 0x02,  // 错误
} seat_status_t;

/* 座椅旋转位置 */
typedef enum {
    /* 单个座椅旋转方向 */
    SEAT_POS_ZHENGQIAN   = 0x50,  // 面向前
    SEAT_POS_ZHENGHOU    = 0x51,  // 面向后
    SEAT_POS_GUODAO      = 0x52,  // 面向过道
    SEAT_POS_1WEIDUAN    = 0x53,  // 面向一位端
    SEAT_POS_2WEIDUAN    = 0x54,  // 面向二位端

    /* 全部座椅旋转 */
    SEAT_ALL_1WEIDUAN    = 0x40,  // 一位端
    SEAT_ALL_2WEIDUAN    = 0x41,  // 二位端
    SEAT_ALL_ZHENGQIAN   = 0x42,  // 正前
    SEAT_ALL_ZHENGHOU    = 0x43,  // 正后

    /* 会议模式 */
    SEAT_MEETING_6REN    = 0x80,  // 六人会议

    /* 两人会客 */
    SEAT_GUEST_1A_2A     = 0x00,
    SEAT_GUEST_2A_3A     = 0x01,
    SEAT_GUEST_1F_2F     = 0x10,
    SEAT_GUEST_2F_3F     = 0x11,
    SEAT_GUEST_1A_1F     = 0x20,
    SEAT_GUEST_2A_2F     = 0x21,
    SEAT_GUEST_3A_3F     = 0x22,

    /* 四人会客 */
    SEAT_GUEST_1A_2A_1F_2F = 0x30,
    SEAT_GUEST_2A_3A_2F_3F = 0x31,
} seat_position_t;

/* 座椅信息结构体 */
typedef struct {
    uint8_t status;       // 座椅状态 seat_status_t
    uint8_t position;     // 当前位置
    uint8_t call;         // 呼叫状态 0=无 1=有
    uint8_t light;        // 氛围灯状态 0=关 1=开
} seat_info_t;

/* 全局座椅信息 */
extern seat_info_t g_seat_info[SEAT_COUNT];
extern uint8_t g_call_status;  // 呼叫状态位图

/* 初始化 */
void logic_proc_init(void);

/* 座椅控制接口 */
void send_seat_rotation_estop(void);                      // 急停
void send_seat_align_to_direction(uint8_t direction);     // 全部座椅朝向
void send_seat_into_meeting_mode(void);                   // 会议模式
void send_seat_into_guest_mode(uint8_t mode);             // 会客模式
void send_seat_position_set(uint8_t seat_num, uint8_t position);  // 单个座椅位置
void send_seat_reset(uint8_t seat_num);                   // 座椅复位(回到坐姿)
void send_reset_call(uint8_t seat_index);                 // 清除呼叫
void send_ambient_light_setting(uint8_t seat_num, uint8_t light_status);  // 氛围灯

#ifdef __cplusplus
}
#endif

#endif
