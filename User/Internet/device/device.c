/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-11 09:55:50
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-09-11 13:17:56
 * @FilePath: \Projectc:\XingNian\XiangMu\450TongXing\CODE\TotalController\total_controller\User\Internet\device\device.c
 * @Description: 该文件包含W5500设备的初始化和配置函数
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */

// 包含必要的头文件
#include "device.h"
#include "config.h"
#include "gpio.h"
#include "w5500.h"
#include "string.h"

// 定义配置消息结构体变量
CONFIG_MSG  ConfigMsg, RecvMsg;

// 定义每个socket的发送和接收缓冲区大小（单位：KB）
uint8 txsize[MAX_SOCK_NUM] = {2, 2, 2, 2, 2, 2, 2, 2};
uint8 rxsize[MAX_SOCK_NUM] = {2, 2, 2, 2, 2, 2, 2, 2};

// 声明外部变量MAC地址
extern uint8 MAC[6];

// 定义公共缓冲区，用于DHCP、DNS和HTTP
uint8 pub_buf[1460];

/**
 * @brief 重置W5500芯片
 *
 * 该函数通过控制W5500的复位引脚来重置芯片
 */
void Reset_W5500(void)
{
    // 将W5500的复位引脚拉低
    HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_RESET);
    HAL_Delay(2);  // 延时2ms

    // 将W5500的复位引脚拉高
    HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_SET);
    HAL_Delay(1600);  // 延时1600ms，等待W5500完成初始化
}

/**
 * @brief 配置网络参数并初始化W5500
 *
 * 该函数设置W5500的MAC地址、IP地址、子网掩码和网关，
 * 然后初始化所有socket并打印网络配置信息
 */
void set_network(void)
{
    uint8 ip[4];

    // 设置MAC地址、子网掩码、网关和IP地址
    setSHAR(ConfigMsg.mac);
    setSUBR(ConfigMsg.sub);
    setGAR(ConfigMsg.gw);
    setSIPR(ConfigMsg.lip);

    // 初始化8个socket
    sysinit(txsize, rxsize);

    // 设置重传超时时间和最大重传次数
    setRTR(2000);  // 设置重传超时时间为2000（单位：100us）
    setRCR(3);     // 设置最大重传次数为3次

    // 获取并打印IP地址
    getSIPR(ip);
    printf("IP : %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);

    // 获取并打印子网掩码
    getSUBR(ip);
    printf("SN : %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);

    // 获取并打印网关地址
    getGAR(ip);
    printf("GW : %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);
}

/**
 * @brief 设置默认网络配置
 *
 * 该函数设置默认的MAC地址、IP地址、子网掩码、网关和DNS服务器，
 * 并初始化其他配置参数
 */
void set_default(void)
{
    // 定义默认的网络参数
    uint8 mac[6] = {0x00, 0x08, 0xdc, 0x11, 0x11, 0x11}; // 默认MAC地址
    uint8 lip[4] = {192, 168, 81, 52}; // 默认本地IP地址
    uint8 sub[4] = {255, 255, 255, 0}; // 默认子网掩码
    uint8 gw[4]  = {192, 168, 1, 1}; // 默认网关
    uint8 dns[4] = {8, 8, 8, 8};     // 默认DNS服务器（Google Public DNS）

    // 将默认参数复制到配置结构体中
    memcpy(ConfigMsg.lip, lip, 4);
    memcpy(ConfigMsg.sub, sub, 4);
    memcpy(ConfigMsg.gw,  gw,  4);
    memcpy(ConfigMsg.mac, mac, 6);
    memcpy(ConfigMsg.dns, dns, 4);

    // 设置其他配置参数
    ConfigMsg.dhcp = 0;       // 禁用DHCP
    ConfigMsg.debug = 1;      // 启用调试模式
    ConfigMsg.fw_len = 0;     // 固件长度初始化为0

    ConfigMsg.state = NORMAL_STATE;  // 设置设备状态为正常
    ConfigMsg.sw_ver[0] = FW_VER_HIGH;  // 设置软件版本号高位
    ConfigMsg.sw_ver[1] = FW_VER_LOW;   // 设置软件版本号低位
}
