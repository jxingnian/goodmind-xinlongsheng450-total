/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-11 14:26:09
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-11-28 14:44:05
 * @FilePath: \total_controller\User\app\app_pis_proc.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include "app_pis_proc.h"
#include "logic_proc.h"
/* 定义帧头和帧尾 */
#define FRAME_HEADER_1 0xC5  /* 第一个帧头字节 */
#define FRAME_HEADER_2 0xCC  /* 第二个帧头字节 */
#define FRAME_TAIL 0xCE      /* 帧尾字节 */


/* 定义消息类型枚举 */
typedef enum {
    REQ_RESET_CALL = 0x01,                /* 重置呼叫请求 */
    REQ_SEAT_ALIGN_TO_DIRECTION,          /* 调整全部座椅到指定车辆运行方向 */
    REQ_SEAT_INTO_MEETING_MODE,           /* 进入会议模式请求 */
    REQ_SEAT_GUEST_MODE_ADJUSTMENT,       /* 会客模式调整请求 */
    REQ_SEAT_ROTATION_ESTOP,              /* 座椅旋转急停请求 */
    REQ_SINGLE_SEAT_POSITION_SET,         /* 单个座椅位置设置请求 */
    REQ_RESET_ANOMALOUS,
    REQ_AMBIENT_LIGHT_SETTING,            /* 氛围灯设置请求 */
} e_msg_type;

/* 定义报文结构 */
typedef struct {
    uint8_t stx1;
    uint8_t stx2;
    uint8_t src[2];
    uint8_t ver_s1;
    uint8_t dst[2];
    uint8_t ver_s2;
    uint8_t fun;
    uint8_t payload_len;
    uint8_t payload[256];  // 假设最大载荷为256字节
    uint8_t check_xor;
    uint8_t tail;
} pis_message_t;

///* 计算异或校验 */
//static uint8_t calculate_xor(uint8_t *data, int len)
//{
//    uint8_t result = 0;
//    for (int i = 0; i < len; i++) {
//        result ^= data[i];
//    }
//    return result;
//}
/* 处理重置呼叫请求 */
static void handle_reset_call(uint8_t *payload, uint8_t len)
{
    if (len != 3) {
        printf("清除呼叫数据长度错误\n");
        return;
    }
    printf("处理清除呼叫：\n");
    send_reset_call(0x99);
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 8; j++) {
//            int seat_num = (2-i) * 8 + j + 1;
//            if (seat_num <= SEAT_COUNT) {
//                printf("座椅%d: %s\n", seat_num, (payload[i] & (1 << j)) ? "保持" : "复位");
//                if ((payload[2-i] & (1 << j)) == 0)
//                    send_reset_call(seat_num);
//            }
//        }
//    }
}

/* 调整全部座椅到指定车辆运行方向 */
static void handle_seat_align_to_direction(uint8_t *payload, uint8_t len)
{
    if (len != 1) {
        printf("调整座椅方向数据长度错误\n");
        return;
    }
    switch (payload[0]) {
    case 1:
        printf("调整全部座椅到1位端(司机侧)\n");
        send_seat_align_to_direction(1);
        break;
    case 2:
        printf("调整全部座椅到2位端(司机侧)\n");
        send_seat_align_to_direction(2);
        break;
    case 3:
        printf("调整全部座椅到向前\n");
        send_seat_align_to_direction(3);
        break;
    case 4:
        printf("调整全部座椅到向后\n");
        send_seat_align_to_direction(4);
        break;
    default:
        printf("未知的座椅方向: %d\n", payload[0]);
    }
}

/* 处理进入会议模式请求 */
static void handle_seat_into_meeting_mode(uint8_t *payload, uint8_t len)
{
    if (len != 0) {
        printf("进入会议模式数据长度错误\n");
        return;
    }
    printf("调整座椅进入会议模式\n");
    send_seat_into_meeting_mode();
}

/* 处理会客模式调整请求 */
static void handle_seat_guest_mode_adjustment(uint8_t *payload, uint8_t len)
{
    if (len != 1) {
        printf("会客模式调整数据长度错误\n");
        return;
    }
    send_seat_into_guest_mode(payload[0]);
}

/* 处理座椅旋转急停请求 */
static void handle_seat_rotation_estop(uint8_t *payload, uint8_t len)
{
    if (len != 0) {
        printf("座椅旋转急停数据长度错误\n");
        return;
    }
    printf("执行座椅旋转急停\n");
    send_seat_rotation_estop();
}

/* 处理单个座椅位置设置请求 */
static void handle_single_seat_position_set(uint8_t *payload, uint8_t len)
{
    if (len != 2) {
        printf("单个座椅位置设置数据长度错误\n");
        return;
    }
    printf("设置座椅 %d 到位置: ", payload[0]);
    send_seat_position_set(payload[0], payload[1]);
}

/* 处理氛围灯设置请求 */
static void handle_ambient_light_setting(uint8_t *payload, uint8_t len)
{
    // TODO 协议有效数据长度，只有一个字节
//    if (len != 2) {
//        printf("氛围灯设置数据长度错误\n");
//        return;
//    }
//    printf("设置氛围灯\n");
    send_ambient_light_setting(0x99, payload[0]);
}

/* 处理PIS数据的主函数 */
void process_pis_data(uint8_t *data, uint16_t len)
{
    /* 检查数据帧的基本有效性 */
    if (len < 12 || data[0] != FRAME_HEADER_1 || data[1] != FRAME_HEADER_2 || data[len - 1] != FRAME_TAIL) {
        printf("Invalid data frame\n");
        return;
    }

    pis_message_t msg;

    /* 解析报文结构 */
    msg.stx1 = data[0];
    msg.stx2 = data[1];
    memcpy(msg.src, &data[2], 2);
    msg.ver_s1 = data[4];
    memcpy(msg.dst, &data[5], 2);
    msg.ver_s2 = data[7];
    msg.fun = data[8];
    msg.payload_len = data[9];
    memcpy(msg.payload, &data[10], msg.payload_len);
    msg.check_xor = data[len - 2];
    msg.tail = data[len - 1];

    /* 打印报文信息 */
    printf("Source address: %d.%d\n", msg.src[0], msg.src[1]);
    printf("Destination address: %d.%d\n", msg.dst[0], msg.dst[1]);
    printf("Function code: 0x%02X\n", msg.fun);
    printf("Payload length: %d\n", msg.payload_len);
    printf("Check value: 0x%02X\n", msg.check_xor);

    /* 验证异或校验 */
//    uint8_t calculated_xor = calculate_xor(data, len - 2);
//    if (calculated_xor != msg.check_xor) {
//        printf("XOR check error, calculated value: 0x%02X, received value: 0x%02X\n", calculated_xor, msg.check_xor);
//        return;
//    }

    /* 根据功能码调用相应的处理函数 */
    switch (msg.fun) {
    case REQ_RESET_CALL://请求复位呼叫
        handle_reset_call(msg.payload, msg.payload_len);
        break;
    case REQ_SEAT_ALIGN_TO_DIRECTION://调整全部座椅到指定车辆运行方向
        handle_seat_align_to_direction(msg.payload, msg.payload_len);
        break;
    case REQ_SEAT_INTO_MEETING_MODE://请求进入会议模式
        handle_seat_into_meeting_mode(msg.payload, msg.payload_len);
        break;
    case REQ_SEAT_GUEST_MODE_ADJUSTMENT://请求会客模式调整
        handle_seat_guest_mode_adjustment(msg.payload, msg.payload_len);
        break;
    case REQ_SEAT_ROTATION_ESTOP://请求座椅旋转急停
        handle_seat_rotation_estop(msg.payload, msg.payload_len);
        break;
    case REQ_SINGLE_SEAT_POSITION_SET://请求单个座椅位置设置
        handle_single_seat_position_set(msg.payload, msg.payload_len);
        break;
    case REQ_AMBIENT_LIGHT_SETTING://请求氛围灯设置
        handle_ambient_light_setting(msg.payload, msg.payload_len);
        break;
    default:
        printf("未知的功能码: 0x%02X\n", msg.fun);
        break;
    }
}

void send_data_to_pis(uint8_t function, uint8_t *payload, uint16_t payload_len)
{
    uint8_t data[256]; // 假设最大数据长度为256字节
    uint16_t len = 0;

    // 构建报文头
    data[len++] = FRAME_HEADER_1; // 起始字节
    data[len++] = FRAME_HEADER_2;

    // 源地址（根据g_total_controller_address确定）
    if (g_total_controller_address == 1) {
        data[len++] = 0x51;
        data[len++] = 0x34;
    } else if (g_total_controller_address == 8) {
        data[len++] = 0x58;
        data[len++] = 0x34;
    }

    // 标识S1
    data[len++] = 0x01;

    // 目标地址（0.55）
    data[len++] = 0x00;
    data[len++] = 0x37;

    // 标识S2
    data[len++] = 0x00;

    // 功能码（假设为0x81，表示响应）
    data[len++] = function; // TODO 更改

    // 添加payload长度
    data[len++] = payload_len;

    // 添加payload数据
    memcpy(&data[len], payload, payload_len);
    len += payload_len;

    // 计算并添加异或校验
    uint8_t xor_check = 0;
    for (int i = 0; i < len; i++) {
        xor_check ^= data[i];
    }
    data[len++] = xor_check;

    // 添加结束字节
    data[len++] = FRAME_TAIL;

    // 发送数据
    app_udp_send_data(data, len);

    printf("向PIS发送了%d字节数据\n", len);
}




