/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-09 10:10:59
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-10-28 16:15:41
 * @FilePath: \total_controller\User\app\app.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
/**
 * @file app.c
 * @author zhaochunyun (CY.Zhao2020@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-04-20
 *
 * @copyright (c) 2024 Goodmind.
 *
 */
#include "app.h"
#include "hmi_driver.h"
#include "hmi_user_uart.h"
#include "bsp_hmi.h"

static tmr_t tmr_timeout_d5s;

static void start_timeout_timer(void); // 启动测试定时器

static int bsp_init(void)
{
    return 0;
}

static int app_init(void)
{
    log_info("\r\nFirmware name: %s \r\n", FW_NAME);
    log_info("version: %d.%d.%d \r\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_patch);

    start_timeout_timer();

    uart_spec_init();
    bsp_uart_start_recv_all();

    start_dacai_uart_handle();

    app_udp_init();
    logic_proc_init();

    return 0;
}

et_prod_init_func_t et_prod_init_tbl[] = {
    bsp_init,
    app_init,
    NULL,     /* !! NULL MUST be here as a sentinal  */
};

// 声明总控地址变量
uint8_t g_total_controller_address = 0;

/*循环定时器*/
static void tmr_timeout_d5s_loop(int timer_id, void *data)
{
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    // 获取编码器的值
    volatile static uint8_t encoder_value = 0;

    // 读取PB5、PB4、PB3、PD2的状态（低电平有效）
    volatile uint8_t pb5 = !HAL_GPIO_ReadPin(BCD_4_GPIO_Port, BCD_4_Pin);
    volatile uint8_t pb4 = !HAL_GPIO_ReadPin(BCD_3_GPIO_Port, BCD_3_Pin);
    volatile uint8_t pb3 = !HAL_GPIO_ReadPin(BCD_2_GPIO_Port, BCD_2_Pin);
    volatile uint8_t pd2 = !HAL_GPIO_ReadPin(BCD_1_GPIO_Port, BCD_1_Pin);

    // 将四个引脚的状态组合成一个4位的BCD码（8421码）
    encoder_value = (pb5 << 3) | (pb4 << 2) | (pb3 << 1) | pd2;

    // BCD码转换为十进制
    if (encoder_value <= 9) {
        g_total_controller_address = encoder_value;
    } else {
        g_total_controller_address = 0; // 如果是无效的BCD码，设置为默认值0
    }
    log_info("总控地址: %d\r\n", g_total_controller_address);
}

static void start_timeout_timer(void)
{
    start_rpt_tmr(&tmr_timeout_d5s, tmr_timeout_d5s_loop, MS_TO_TICKS(500));
}


