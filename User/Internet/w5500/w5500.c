// 包含必要的头文件
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "spi2.h"
#include "w5500.h"
#include "socket.h"

// 如果定义了 __DEF_IINCHIP_PPP__，则包含 md5.h
#ifdef __DEF_IINCHIP_PPP__
   #include "md5.h"
#endif

// 定义静态数组用于存储每个套接字的状态和缓冲区大小
static uint8 I_STATUS[MAX_SOCK_NUM];
static uint16 SSIZE[MAX_SOCK_NUM]; /**< 每个通道的最大发送缓冲区大小 */
static uint16 RSIZE[MAX_SOCK_NUM]; /**< 每个通道的最大接收缓冲区大小 */

// 获取指定套接字的中断状态
uint8 getISR(uint8 s)
{
  return I_STATUS[s];
}

// 设置指定套接字的中断状态
void putISR(uint8 s, uint8 val)
{
   I_STATUS[s] = val;
}

// 获取指定套接字的最大接收缓冲区大小
uint16 getIINCHIP_RxMAX(uint8 s)
{
   return RSIZE[s];
}

// 获取指定套接字的最大发送缓冲区大小
uint16 getIINCHIP_TxMAX(uint8 s)
{
   return SSIZE[s];
}

// 禁用芯片选择（CS）信号
void IINCHIP_CSoff(void)
{
  WIZ_CS(LOW);
}

// 启用芯片选择（CS）信号
void IINCHIP_CSon(void)
{
   WIZ_CS(HIGH);
}

// 通过SPI发送数据并返回接收到的数据
uint8  IINCHIP_SpiSendData(uint8 dat)
{
   return(SPI2_SendByte(dat));
}

// 向W5500写入一个字节的数据
void IINCHIP_WRITE( uint32 addrbsb,  uint8 data)
{
   IINCHIP_ISR_DISABLE();                        // 禁用中断服务程序
   IINCHIP_CSoff();                              // CS=0，开始SPI传输
   IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);// 发送地址字节1
   IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);// 发送地址字节2
   IINCHIP_SpiSendData( (addrbsb & 0x000000F8) + 4);// 发送数据写入命令和写入数据长度1
   IINCHIP_SpiSendData(data);                    // 写入1字节数据
   IINCHIP_CSon();                               // CS=1，结束SPI传输
   IINCHIP_ISR_ENABLE();                         // 启用中断服务程序
}

// 从W5500读取一个字节的数据
uint8 IINCHIP_READ(uint32 addrbsb)
{
   uint8 data = 0;
   IINCHIP_ISR_DISABLE();                        // 禁用中断服务程序
   IINCHIP_CSoff();                              // CS=0，开始SPI传输
   IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);// 发送地址字节1
   IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);// 发送地址字节2
   IINCHIP_SpiSendData( (addrbsb & 0x000000F8))    ;// 发送数据读取命令和读取数据长度1
   data = IINCHIP_SpiSendData(0x00);             // 读取1字节数据
   IINCHIP_CSon();                               // CS=1，结束SPI传输
   IINCHIP_ISR_ENABLE();                         // 启用中断服务程序
   
   return data;    
}

// 向W5500写入多个字节的数据
uint16 wiz_write_buf(uint32 addrbsb,uint8* buf,uint16 len)
{
   uint16 idx = 0;
   if(len == 0) printf("Unexpected2 length 0\r\n");

   IINCHIP_ISR_DISABLE();
   IINCHIP_CSoff();                              // CS=0，开始SPI传输
   IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);// 发送地址字节1
   IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);// 发送地址字节2
   IINCHIP_SpiSendData( (addrbsb & 0x000000F8) + 4);    // 发送数据写入命令和写入数据长度1
   for(idx = 0; idx < len; idx++)                // 循环写入数据
   {
     IINCHIP_SpiSendData(buf[idx]);
   }
   IINCHIP_CSon();                               // CS=1，结束SPI传输
   IINCHIP_ISR_ENABLE();                         // 启用中断服务程序    

   return len;  
}

// 从W5500读取多个字节的数据
uint16 wiz_read_buf(uint32 addrbsb, uint8* buf,uint16 len)
{
  uint16 idx = 0;
  if(len == 0)
  {
    printf("Unexpected2 length 0\r\n");
  }

  IINCHIP_ISR_DISABLE();
  //SPI MODE I/F
  IINCHIP_CSoff();                                  // CS=0，开始SPI传输
  IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);// 发送地址字节1
  IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);// 发送地址字节2
  IINCHIP_SpiSendData( (addrbsb & 0x000000F8));    // 发送数据读取命令和读取数据长度1
  for(idx = 0; idx < len; idx++)                    // 循环读取数据
  {
    buf[idx] = IINCHIP_SpiSendData(0x00);
  }
  IINCHIP_CSon();                                   // CS=1，结束SPI传输
  IINCHIP_ISR_ENABLE();                             // 启用中断服务程序
  
  return len;
}

/**
 * @brief 重置并初始化W5500芯片
 * 
 * 此函数用于重置W5500芯片，并初始化它以在直接或间接模式下工作。
 * 它通过设置模式寄存器(MR)来实现重置操作。
 */
void iinchip_init(void)
{
  setMR( MR_RST );  // 设置模式寄存器为复位状态
#ifdef __DEF_IINCHIP_DBG__
  printf("MR value is %02x \r\n",IINCHIP_READ_COMMON(MR));  // 调试输出：打印模式寄存器的值
#endif
}

/**
 * @brief 初始化系统并设置每个通道的发送和接收缓冲区大小
 * 
 * @param tx_size 指向每个通道发送缓冲区大小的数组指针
 * @param rx_size 指向每个通道接收缓冲区大小的数组指针
 * 
 * @note W5500的总内存大小为16KB，可以在8个通道间动态分配
 *       每个通道的内存大小可以是1KB, 2KB, 4KB, 8KB或16KB
 *       所有通道的总内存分配不能超过16KB
 */
void sysinit(uint8 * tx_size, uint8 * rx_size)
{
    int16 i;
    int16 ssum, rsum;  // 用于累计发送和接收内存总和
    
#ifdef __DEF_IINCHIP_DBG__
    printf("sysinit()\r\n");
#endif
    
    ssum = 0;
    rsum = 0;

    // 遍历所有通道，设置每个通道的发送和接收内存大小
    for (i = 0; i < MAX_SOCK_NUM; i++)
    {
        // 设置每个通道的发送和接收内存大小
        IINCHIP_WRITE((Sn_TXMEM_SIZE(i)), tx_size[i]);
        IINCHIP_WRITE((Sn_RXMEM_SIZE(i)), rx_size[i]);
        
#ifdef __DEF_IINCHIP_DBG__
        printf("tx_size[%d]: %d, Sn_TXMEM_SIZE = %d\r\n", i, tx_size[i], IINCHIP_READ(Sn_TXMEM_SIZE(i)));
        printf("rx_size[%d]: %d, Sn_RXMEM_SIZE = %d\r\n", i, rx_size[i], IINCHIP_READ(Sn_RXMEM_SIZE(i)));
#endif

        // 初始化SSIZE和RSIZE数组
        SSIZE[i] = (int16)(0);
        RSIZE[i] = (int16)(0);

        // 设置发送缓冲区大小
        if (ssum <= 16384)
        {
            switch(tx_size[i])
            {
                case 1:  SSIZE[i] = (int16)(1024);  break;
                case 2:  SSIZE[i] = (int16)(2048);  break;
                case 4:  SSIZE[i] = (int16)(4096);  break;
                case 8:  SSIZE[i] = (int16)(8192);  break;
                case 16: SSIZE[i] = (int16)(16384); break;
                default: SSIZE[i] = (int16)(2048);  break;
            }
        }

        // 设置接收缓冲区大小
        if (rsum <= 16384)
        {
            switch(rx_size[i])
            {
                case 1:  RSIZE[i] = (int16)(1024);  break;
                case 2:  RSIZE[i] = (int16)(2048);  break;
                case 4:  RSIZE[i] = (int16)(4096);  break;
                case 8:  RSIZE[i] = (int16)(8192);  break;
                case 16: RSIZE[i] = (int16)(16384); break;
                default: RSIZE[i] = (int16)(2048);  break;
            }
        }

        // 累加发送和接收内存总和
        ssum += SSIZE[i];
        rsum += RSIZE[i];
    }
}

/**
 * @brief 设置网关IP地址
 * 
 * @param addr 指向4字节数组的指针，包含网关IP地址
 */
void setGAR(uint8 * addr)
{
    wiz_write_buf(GAR0, addr, 4);
}

/**
 * @brief 获取网关IP地址
 * 
 * @param addr 指向4字节数组的指针，用于存储获取的网关IP地址
 */
void getGWIP(uint8 * addr)
{
    wiz_read_buf(GAR0, addr, 4);
}

/**
 * @brief 设置子网掩码地址
 * 
 * @param addr 指向4字节数组的指针，包含子网掩码地址
 */
void setSUBR(uint8 * addr)
{   
    wiz_write_buf(SUBR0, addr, 4);
}

/**
 * @brief 设置MAC地址
 * 
 * @param addr 指向6字节数组的指针，包含MAC地址
 */
void setSHAR(uint8 * addr)
{
  wiz_write_buf(SHAR0, addr, 6);  
}

/**
 * @brief 设置源IP地址
 * 
 * @param addr 指向4字节数组的指针，包含源IP地址
 */
void setSIPR(uint8 * addr)
{
    wiz_write_buf(SIPR0, addr, 4);  
}

/**
 * @brief 设置目标IP地址
 * 
 * @param n 套接字号
 * @param addr 指向4字节数组的指针，包含目标IP地址
 */
void setDIPR(SOCKET n, uint8 * addr)
{
    wiz_write_buf(Sn_DIPR0(n), addr, 4);  
}

/**
 * @brief 设置目标MAC地址
 * 
 * @param n 套接字号
 * @param addr 指向6字节数组的指针，包含目标MAC地址
 */
void setDHAR(SOCKET n, uint8 * addr)
{
    wiz_write_buf(Sn_DHAR0(n), addr, 6);  
}

/**
 * @brief 设置目标端口
 * 
 * @param n 套接字号
 * @param port 目标端口号
 */
void setDPORT(SOCKET n, uint16 port)
{
    IINCHIP_WRITE(Sn_DPORT1(n), (uint8)(port & 0x00FF)); 
    IINCHIP_WRITE(Sn_DPORT0(n), (uint8)((port & 0xFF00)>>8));
} 

/**
 * @brief 获取网关地址
 * 
 * @param addr 指向4字节数组的指针，用于存储获取的网关地址
 */
void getGAR(uint8 * addr)
{
    wiz_read_buf(GAR0, addr, 4);
}

/**
 * @brief 获取子网掩码
 * 
 * @param addr 指向4字节数组的指针，用于存储获取的子网掩码
 */
void getSUBR(uint8 * addr)
{
    // 从W5500的SUBR0寄存器开始读取4字节数据到addr指向的数组中
    wiz_read_buf(SUBR0, addr, 4);
}

/**
 * @brief 获取MAC地址
 * 
 * @param addr 指向6字节数组的指针，用于存储获取的MAC地址
 */
void getSHAR(uint8 * addr)
{
    // 从W5500的SHAR0寄存器开始读取6字节数据到addr指向的数组中
    wiz_read_buf(SHAR0, addr, 6);
}

/**
 * @brief 获取IP地址
 * 
 * @param addr 指向4字节数组的指针，用于存储获取的IP地址
 */
void getSIPR(uint8 * addr)
{
    // 从W5500的SIPR0寄存器开始读取4字节数据到addr指向的数组中
    wiz_read_buf(SIPR0, addr, 4);
}

/**
 * @brief 设置模式寄存器
 * 
 * @param val 要设置的模式值
 */
void setMR(uint8 val)
{
    // 向W5500的MR寄存器写入指定的值
    IINCHIP_WRITE(MR,val);
}

/**
 * @brief 获取中断寄存器的值
 * 
 * @return uint8 中断寄存器的当前值
 */
uint8 getIR( void )
{
    // 读取并返回W5500的IR寄存器的值
    return IINCHIP_READ(IR);
}

/**
 * @brief 设置重传超时时间
 * 
 * @param timeout 重传超时时间值
 */
void setRTR(uint16 timeout)
{
    // 将16位的timeout值分别写入RTR0和RTR1寄存器
    IINCHIP_WRITE(RTR0,(uint8)((timeout & 0xff00) >> 8));
    IINCHIP_WRITE(RTR1,(uint8)(timeout & 0x00ff));
}

/**
 * @brief 设置重传次数
 * 
 * @param retry 重传次数
 */
void setRCR(uint8 retry)
{
    // 向W5500的WIZ_RCR寄存器写入重传次数
    IINCHIP_WRITE(WIZ_RCR,retry);
}

/**
 * @brief 清除中断标志
 * 
 * @param mask 要清除的中断标志掩码
 */
void clearIR(uint8 mask)
{
    // 清除IR寄存器中指定的中断标志位
    IINCHIP_WRITE(IR, ~mask | getIR() );
}

/**
 * @brief 设置TCP最大段大小（MSS）
 * 
 * @param s 套接字号
 * @param Sn_MSSR 最大段大小值
 */
void setSn_MSS(SOCKET s, uint16 Sn_MSSR)
{
    // 将16位的Sn_MSSR值分别写入Sn_MSSR0和Sn_MSSR1寄存器
    IINCHIP_WRITE( Sn_MSSR0(s), (uint8)((Sn_MSSR & 0xff00) >> 8));
    IINCHIP_WRITE( Sn_MSSR1(s), (uint8)(Sn_MSSR & 0x00ff));
}

/**
 * @brief 设置生存时间（TTL）
 * 
 * @param s 套接字号
 * @param ttl 生存时间值
 */
void setSn_TTL(SOCKET s, uint8 ttl)
{    
    // 向指定套接字的Sn_TTL寄存器写入TTL值
    IINCHIP_WRITE( Sn_TTL(s) , ttl);
}

/**
 * @brief 获取套接字中断状态
 * 
 * @param s 套接字号
 * @return uint8 套接字中断状态
 */
uint8 getSn_IR(SOCKET s)
{
    // 读取并返回指定套接字的Sn_IR寄存器的值
    return IINCHIP_READ(Sn_IR(s));
}

/**
 * @brief 获取套接字状态
 * 
 * @param s 套接字号
 * @return uint8 套接字状态
 */
uint8 getSn_SR(SOCKET s)
{
    // 读取并返回指定套接字的Sn_SR寄存器的值
    return IINCHIP_READ(Sn_SR(s));
}

/**
 * @brief 获取套接字发送缓冲区的空闲大小
 * 
 * @param s 套接字号
 * @return uint16 发送缓冲区的空闲大小（字节）
 */
uint16 getSn_TX_FSR(SOCKET s)
{
    uint16 val=0,val1=0;
    do
    {
        // 读取Sn_TX_FSR寄存器的值（高8位和低8位）
        val1 = IINCHIP_READ(Sn_TX_FSR0(s));
        val1 = (val1 << 8) + IINCHIP_READ(Sn_TX_FSR1(s));
        if (val1 != 0)
        {
            // 再次读取以确保数据稳定
            val = IINCHIP_READ(Sn_TX_FSR0(s));
            val = (val << 8) + IINCHIP_READ(Sn_TX_FSR1(s));
        }
    } while (val != val1); // 如果两次读取的值不一致，则继续循环
    return val; // 返回发送缓冲区的空闲大小
}

/**
 * @brief 获取套接字接收缓冲区中接收到的数据大小
 * 
 * @param s 套接字号
 * @return uint16 接收缓冲区中的数据大小（字节）
 */
uint16 getSn_RX_RSR(SOCKET s)
{
    uint16 val = 0, val1 = 0;
    do
    {
        // 读取Sn_RX_RSR寄存器的值（高8位和低8位）
        val1 = IINCHIP_READ(Sn_RX_RSR0(s));
        val1 = (val1 << 8) + IINCHIP_READ(Sn_RX_RSR1(s));
        
        if(val1 != 0)
        {
            // 再次读取以确保数据稳定
            val = IINCHIP_READ(Sn_RX_RSR0(s));
            val = (val << 8) + IINCHIP_READ(Sn_RX_RSR1(s));
        }
    } while (val != val1); // 如果两次读取的值不一致，则继续循环
    
    return val; // 返回接收缓冲区中的数据大小
}

/**
 * @brief 处理发送数据
 * 
 * @param s 套接字号
 * @param data 要发送的数据指针
 * @param len 要发送的数据长度
 */
void send_data_processing(SOCKET s, uint8 *data, uint16 len)
{
    uint16 ptr =0;
    uint32 addrbsb =0;
    if(len == 0)
    {
        printf("CH: %d Unexpected1 length 0\r\n", s);
        return;
    }

    // 读取发送写指针
    ptr = IINCHIP_READ( Sn_TX_WR0(s) );
    ptr = ((ptr & 0x00ff) << 8) + IINCHIP_READ(Sn_TX_WR1(s));

    // 计算基地址
    addrbsb = (uint32)(ptr<<8) + (s<<5) + 0x10;
    // 写入数据到发送缓冲区
    wiz_write_buf(addrbsb, data, len);
  
    // 更新发送写指针
    ptr += len;
    IINCHIP_WRITE( Sn_TX_WR0(s) ,(uint8)((ptr & 0xff00) >> 8));
    IINCHIP_WRITE( Sn_TX_WR1(s),(uint8)(ptr & 0x00ff));
}

/**
 * @brief 处理接收数据
 * 
 * @param s 套接字号
 * @param data 接收数据的缓冲区指针
 * @param len 要接收的数据长度
 */
void recv_data_processing(SOCKET s, uint8 *data, uint16 len)
{
    uint16 ptr = 0;
    uint32 addrbsb = 0;
  
    if(len == 0)
    {
        printf("CH: %d Unexpected2 length 0\r\n", s);
        return;
    }

    // 读取接收读指针
    ptr = IINCHIP_READ( Sn_RX_RD0(s) );
    ptr = ((ptr & 0x00ff) << 8) + IINCHIP_READ( Sn_RX_RD1(s) );

    // 计算基地址
    addrbsb = (uint32)(ptr<<8) + (s<<5) + 0x18;
    // 从接收缓冲区读取数据
    wiz_read_buf(addrbsb, data, len);
    
    // 更新接收读指针
    ptr += len;
    IINCHIP_WRITE( Sn_RX_RD0(s), (uint8)((ptr & 0xff00) >> 8));
    IINCHIP_WRITE( Sn_RX_RD1(s), (uint8)(ptr & 0x00ff));
}

/**
 * @brief 设置套接字中断标志
 * 
 * @param s 套接字号
 * @param val 要设置的中断标志值
 */
void setSn_IR(uint8 s, uint8 val)
{
    // 向指定套接字的Sn_IR寄存器写入中断标志值
    IINCHIP_WRITE(Sn_IR(s), val);
}

