/*
 * @Author: XingNian j_xingnian@163.com
 * @Date: 2024-09-09 12:49:39
 * @LastEditors: XingNian j_xingnian@163.com
 * @LastEditTime: 2024-09-10 10:53:03
 * @FilePath: \Projectc:\XingNian\XiangMu\450TongXing\CODE\TotalController\total_controller\User\bsp\bsp_spi\bsp_spi.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include "bsp_spi.h"
#include "main.h"

extern SPI_HandleTypeDef hspi2;

/**
  * @brief  写1字节数据到SPI总线
  * @param  TxData 写到总线的数据
  * @retval None
  */
void SPI_WriteByte(uint8_t TxData)
{
#if 0
    while ((SPI2->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET); //等待发送区空
    SPI2->DR = TxData;                                                        //发送一个byte
    while ((SPI2->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET); //等待接收完一个byte
    SPI2->DR;
#else
    while ((SPI1->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET); //等待发送区空
    SPI1->DR = TxData;                                          //发送一个byte
    while ((SPI1->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET); //等待接收完一个byte
    SPI1->DR;
#endif
}
/**
  * @brief  从SPI总线读取1字节数据
  * @retval 读到的数据
  */
uint8_t SPI_ReadByte(void)
{
#if 0
    while ((SPI2->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET); //等待发送区空
    SPI2->DR = 0xFF;                                            //发送一个空数据产生输入数据的时钟
    while ((SPI2->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET); //等待接收完一个byte
    return SPI2->DR;
#else
    while ((SPI1->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET); //等待发送区空
    SPI1->DR = 0xFF;                                            //发送一个空数据产生输入数据的时钟
    while ((SPI1->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET); //等待接收完一个byte
    return SPI1->DR;
#endif
}
/**
  * @brief  进入临界区
  * @retval None
  */
void SPI_CrisEnter(void)
{
    __set_PRIMASK(1);
}
/**
  * @brief  退出临界区
  * @retval None
  */
void SPI_CrisExit(void)
{
    __set_PRIMASK(0);
}

/**
  * @brief  片选信号输出低电平
  * @retval None
  */
void SPI_CS_Select(void)
{
#if 0
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
#else
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
#endif
}
/**
  * @brief  片选信号输出高电平
  * @retval None
  */
void SPI_CS_Deselect(void)
{
#if 0
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
#else
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
#endif
}
/*********************************END OF FILE**********************************/

