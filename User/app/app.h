/**
 * @file app.h
 * @brief 总控制器应用层头文件
 */
#ifndef _APP_H_
#define _APP_H_

#include "config.h"
#include "et_os.h"
#include "et_timer.h"
#include "bsp_uart.h"
#include "uart_spec.h"
#include "app_udp.h"
#include "logic_proc.h"
#include "main.h"
#include <string.h>

// 设备地址(BCD拨码)
extern uint8_t g_device_addr;

#endif
