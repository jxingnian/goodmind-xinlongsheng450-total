/**
 * @file config.h
 * @brief 系统配置文件
 */
#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "types.h"
#include "device.h"

/* 通用定义 */
#define ON   1
#define OFF  0
#define HIGH 1
#define LOW  0

/* W5500配置 */
#define W5500_CS_PIN    GPIO_PIN_12
#define W5500_CS_PORT   GPIOB

/* 网络配置 */
#define NORMAL_STATE          0
#define NEW_APP_IN_BACK       1
#define CONFIGTOOL_FW_UP      2

#pragma pack(1)
typedef struct _CONFIG_MSG {
    uint8 op[4];
    uint8 mac[6];
    uint8 sw_ver[2];
    uint8 lip[4];
    uint8 sub[4];
    uint8 gw[4];
    uint8 dns[4];
    uint8 dhcp;
    uint8 debug;
    uint16 fw_len;
    uint8 state;
} CONFIG_MSG;
#pragma pack()

extern CONFIG_MSG ConfigMsg, RecvMsg;

/* 日志配置 */
#define USING_DEBUG
#ifdef USING_DEBUG
#define log_level  3
#define log_printf(level, ...) do { if (level <= log_level) printf(__VA_ARGS__); } while(0)
#define log_err(...)   log_printf(1, __VA_ARGS__)
#define log_warn(...)  log_printf(2, __VA_ARGS__)
#define log_info(...)  log_printf(3, __VA_ARGS__)
#define log_dbg(...)   log_printf(4, __VA_ARGS__)
#define log_hex(x, len) do { for(int i=0; i<len; i++) log_info("%02X ", ((uint8_t*)x)[i]); log_info("\r\n"); } while(0)
#else
#define log_err(...)
#define log_warn(...)
#define log_info(...)
#define log_dbg(...)
#define log_hex(x, len)
#endif

/* 固件信息 */
#define FW_NAME            "Total_Controller"
#define FW_VERSION_MAJOR   1
#define FW_VERSION_MINOR   0
#define FW_VERSION_PATCH   0

#ifdef __cplusplus
}
#endif

#endif
