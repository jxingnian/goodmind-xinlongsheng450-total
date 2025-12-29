/*
 * @Author: 星年 && j_xingnian@163.com
 * @Date: 2025-12-29 11:26:09
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-12-29 13:02:49
 * @FilePath: \goodmind-xinlongsheng450-total\User\app\app.c
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
/**
 * @file app.c
 * @brief 总控制器应用层主文件
 */
#include "app.h"
#include "hmi_driver.h"
#include "hmi_user_uart.h"
#include "bsp_hmi.h"

static tmr_t tmr_heartbeat;

static int bsp_init(void)
{
    return 0;
}

static void tmr_heartbeat_cb(int timer_id, void *data)
{
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    
    // 读取BCD拨码开关地址(低电平有效)
    uint8_t pb5 = !HAL_GPIO_ReadPin(BCD_4_GPIO_Port, BCD_4_Pin);
    uint8_t pb4 = !HAL_GPIO_ReadPin(BCD_3_GPIO_Port, BCD_3_Pin);
    uint8_t pb3 = !HAL_GPIO_ReadPin(BCD_2_GPIO_Port, BCD_2_Pin);
    uint8_t pd2 = !HAL_GPIO_ReadPin(BCD_1_GPIO_Port, BCD_1_Pin);
    
    uint8_t bcd_val = (pb5 << 3) | (pb4 << 2) | (pb3 << 1) | pd2;
    if (bcd_val <= 9) {
        g_device_addr = bcd_val;
    }
}

static int app_init(void)
{
    log_info("\r\nFirmware: %s v%d.%d.%d\r\n", FW_NAME, FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);

    // 启动心跳定时器
    start_rpt_tmr(&tmr_heartbeat, tmr_heartbeat_cb, MS_TO_TICKS(500));

    // 串口初始化
    uart_spec_init();
    bsp_uart_start_recv_all();

    // 大彩屏初始化
    start_dacai_uart_handle();

    // UDP初始化
    app_udp_init();

    // 业务逻辑初始化
    logic_proc_init();

    return 0;
}

et_prod_init_func_t et_prod_init_tbl[] = {
    bsp_init,
    app_init,
    NULL,
};

// 设备地址(BCD拨码)
uint8_t g_device_addr = 0;
