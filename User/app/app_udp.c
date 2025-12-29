/*
 * @Author: 星年 && j_xingnian@163.com
 * @Date: 2025-12-29 11:26:09
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-12-29 13:08:37
 * @FilePath: \goodmind-xinlongsheng450-total\User\app\app_udp.c
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
/**
 * @file app_udp.c
 * @brief W5500 UDP通讯
 */
#include "app_udp.h"
#include "et_timer.h"
#include "w5500.h"
#include "socket.h"
#include "device.h"

static tmr_t tmr_udp_process;

// 组播配置
static uint8_t multicast_ip[4] = {225, 0, 0, 55};
static uint8_t multicast_mac[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x37};
static uint16_t local_port = 8002;
static uint16_t remote_port = 8003;

static uint8_t recv_buf[512];

static void udp_process_cb(int timer_id, void *data)
{
    uint16_t len = 0;
    uint8_t remote_ip[4];
    uint16_t port;
    
    switch (getSn_SR(0)) {
    case SOCK_UDP:
        if (getSn_IR(0) & Sn_IR_RECV) {
            setSn_IR(0, Sn_IR_RECV);
        }
        if ((len = getSn_RX_RSR(0)) > 0) {
            len = recvfrom(0, recv_buf, len, remote_ip, &port);
            // TODO: 处理接收到的PIS数据
            // pis_proc_recv(recv_buf, len);
        }
        break;
        
    case SOCK_CLOSED:
        // 重新初始化socket
        app_udp_init();
        break;
    }
}

void app_udp_init(void)
{
    Reset_W5500();
    set_default();
    set_network();
    
    // 配置组播
    setDIPR(0, multicast_ip);
    setDHAR(0, multicast_mac);
    setDPORT(0, remote_port);
    
    uint8_t mode = Sn_MR_UDP | Sn_MR_MULTI;
    IINCHIP_WRITE(Sn_MR(0), mode);
    socket(0, mode, local_port, 0);
    
    // 启动UDP处理定时器
    start_rpt_tmr(&tmr_udp_process, udp_process_cb, MS_TO_TICKS(50));
}

void app_udp_send(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return;
    if (getSn_SR(0) != SOCK_UDP) return;
    
    sendto(0, data, len, multicast_ip, remote_port);
}
