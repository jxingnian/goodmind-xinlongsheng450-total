/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-11 14:26:43
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-09-12 13:04:00
 * @FilePath: \Projectc:\XingNian\XiangMu\450TongXing\CODE\TotalController\total_controller\User\app\app_pis_proc.h
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#ifndef _APP_PIS_PROC_H__
#define _APP_PIS_PROC_H__

#include "main.h"
#include "stdio.h"
#include <string.h>
#include "app.h"


#define SEAT_COUNT 6
void process_pis_data(uint8_t *data, uint16_t len);
void send_data_to_pis(uint8_t *payload, uint16_t payload_len);


#endif
