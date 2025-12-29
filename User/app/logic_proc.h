/**
 * @file logic_proc.h
 * @brief 业务逻辑处理
 */
#ifndef _LOGIC_PROC_H_
#define _LOGIC_PROC_H_

#include <stdint.h>
#include "et_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 命令定义 */
#define CMD_READ   0x03  // 读
#define CMD_WRITE  0x06  // 写
#define CMD_OTHER  0x09  // 其他

/* 寄存器定义 */
#define REG_SEAT_STATUS      0x01  // 读取座椅旋转状态
#define REG_CALL_STATUS      0x02  // 读取呼叫状态
#define REG_LIGHT_STATUS     0x03  // 读取氛围灯状态
#define REG_SEAT_ROTATE      0x30  // 控制座椅旋转
#define REG_SEAT_RESET       0x31  // 座椅复位
#define REG_CALL_CLEAR       0x32  // 清除呼叫
#define REG_LIGHT_CTRL       0x33  // 控制氛围灯
#define REG_SEAT_ESTOP       0x34  // 座椅急停

/* 初始化 */
void logic_proc_init(void);

/* 座椅控制接口 (供大彩屏调用) */
void send_seat_rotation_estop(void);                      // 急停
void send_seat_align_to_direction(uint8_t direction);     // 全部座椅朝向 1=一位端 2=二位端
void send_seat_into_meeting_mode(void);                   // 会议模式
void send_seat_into_guest_mode(uint8_t mode);             // 会客模式
void send_seat_position_set(uint8_t seat_num, uint8_t position);  // 单个座椅位置
void send_reset_call(uint8_t seat_index);                 // 清除呼叫
void send_ambient_light_setting(uint8_t seat_num, uint8_t light_status);  // 氛围灯

#ifdef __cplusplus
}
#endif

#endif
