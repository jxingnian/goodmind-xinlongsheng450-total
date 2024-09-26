/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-11 12:30:52
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-09-11 15:19:54
 * @FilePath: \Projectc:\XingNian\XiangMu\450TongXing\CODE\TotalController\total_controller\User\Internet\spi\spi2.c
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "stm32f1xx_hal.h"
#include "config.h"
#include "socket.h"
#include "w5500.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gpio.h"
#include "spi.h"

// Connected to Data Flash
void WIZ_CS(uint8_t val)
{
	if (val == LOW) 
	{
   		HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_RESET);
	}
	else if (val == HIGH)
	{
   		HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_SET);
	}
}

uint8_t SPI2_SendByte(uint8_t byte)
{
    uint8_t receivedByte;
    
//    // 等待发送缓冲区为空
//    while(__HAL_SPI_GET_FLAG(&hspi2, SPI_FLAG_TXE) == RESET);
//    
    // 发送并接收数据
    if (HAL_SPI_TransmitReceive(&hspi2, &byte, &receivedByte, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        // 如果传输失败，返回错误值
        return 0xFF;
    }
    return receivedByte;
}



