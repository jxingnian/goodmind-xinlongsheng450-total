/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-09 12:19:27
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-09-12 09:55:47
 * @FilePath: \Projectc:\XingNian\XiangMu\450TongXing\CODE\TotalController\total_controller\User\app\app_udp.c
 * @Description: W5500 UDP通信实现，包括组播功能
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */

/* 包含必要的头文件 */
#include "main.h"
#include "app_udp.h"
#include "et_os.h"
#include "et_timer.h"
#include "spi.h"
#include "w5500.h"
#include "socket.h"
#include "app_pis_proc.h"
/* 定义常量 */
static tmr_t tmr_udp_process;
extern uint8 txsize[];          /* 引用外部变量，声明Socket发送缓存大小 */
extern uint8 rxsize[];          /* 引用外部变量，声明Socket接收缓存大小 */
uint8 buffer[2048];             /* 定义一个2KB的数组，用来存放Socket的通信数据 */
uint8 remote_ip[4];             /* 配置远程IP地址 */
uint16 remote_port;             /* 配置远程端口 */
uint16 len = 0;

uint8 DIP[4] = {225, 0, 0, 55};
uint8 DHAR[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x37};
uint8 LDIP[4] = {225, 0, 0, 55};
uint8 LDHAR[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x37};
//uint16 DPORT = 8002;


/* UDP处理循环函数 */
static void tmr_udp_process_loop(int timer_id, void *data)
{

    switch (getSn_SR(0)) {                                                                                  // 获取socket0的状态
    case SOCK_UDP:                                                                                          // Socket处于初始化完成(打开)状态                                                                        // Socket处于初始化完成(打开)状态
        if (getSn_IR(1) & Sn_IR_RECV) {
            setSn_IR(1, Sn_IR_RECV);                                                            // Sn_IR的RECV位置1
        }
        if ((len = getSn_RX_RSR(1)) > 0) {
            len -= 8;
            recvfrom(1, buffer, len, remote_ip, &remote_port);          // W5500接收来自远程上位机的数据，并通过SPI发送给MCU
            process_pis_data(buffer, len);  //处理接收到的数据
        }
        break;
    case SOCK_CLOSED:                                                                                       // Socket处于关闭状态
//        // 发送
//        setDIPR(0, DIP);
//        setDHAR(0, DHAR);
//        setDPORT(0, 8002);
//				uint8_t mode = 0x82;
//				mode |= (1 << 5); 
//				IINCHIP_WRITE(Sn_MR(0), mode);
//				socket(0, mode, 8002, 0);                   /*初始化socket 0的套接字*/
//				
//        // 接收
//				setDIPR(1, LDIP);
//        setDHAR(1, LDHAR);
//        setDPORT(1, 8003);
//				IINCHIP_WRITE(Sn_MR(1), mode);
//				socket(1, mode, 8003, 0);                   /*初始化socket 1的套接字*/
//				
        break;
    }
}

/* 启动UDP定时器 */
static void start_udp_timer(void)
{
    start_rpt_tmr(&tmr_udp_process, tmr_udp_process_loop, MS_TO_TICKS(100));
}

// 发送 8002
// 接收 8003
/* UDP初始化函数 */
void app_udp_init(void)
{
    Reset_W5500();
    // 启动UDP定时器
    /***** W5500的IP信息初始化 *****/
    set_default();                       // 设置默认MAC、IP、GW、SUB、DNS
    set_network();
    
          // 发送
        setDIPR(0, DIP);
        setDHAR(0, DHAR);
        setDPORT(0, 8002);
				uint8_t mode = 0x82;
				mode |= (1 << 5); 
				IINCHIP_WRITE(Sn_MR(0), mode);
				socket(0, mode, 8002, 0);                   /*初始化socket 0的套接字*/
				
        // 接收
				setDIPR(1, LDIP);
        setDHAR(1, LDHAR);
        setDPORT(1, 8003);
				IINCHIP_WRITE(Sn_MR(1), mode);
				socket(1, mode, 8003, 0);                   /*初始化socket 1的套接字*/
				
        
    start_udp_timer();
}

void app_udp_send_data(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        printf("发送数据无效\r\n");
        return;
    }

    // 确保socket处于UDP模式
    if (getSn_SR(0) != SOCK_UDP) {
        printf("Socket未处于UDP模式\r\n");
        return;
    }

    // 发送数据
    int32_t result = sendto(0, data, len, DIP, 8002);

    if (result == len) {
        printf("成功向PIS发送%d字节数据\r\n", len);
    } else {
        printf("向PIS发送数据失败，错误代码：%d\r\n", result);
    }
}
