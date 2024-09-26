/**************************************************************************************************
* Filename:             config.h
* Revised:
* Revision:
* Description:
**************************************************************************************************/
#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef __cplusplus

extern "C"
{

#endif

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "types.h"
#include "device.h"

#define SOCK_DHCP             0
#define SOCK_HTTP             1

#define ON                      1
#define OFF                     0
#define HIGH                        1
#define LOW                         0

#define W5500_CS_PIN GPIO_PIN_12    /* W5500片选引脚 */
#define W5500_CS_PORT GPIOB         /* W5500片选端口 */

#define CONFIG_MSG_LEN        sizeof(CONFIG_MSG) - 4 // the 4 bytes OP will not save to EEPROM

#define MAX_BUF_SIZE                    1460
#define KEEP_ALIVE_TIME         30  // 30sec
#define SOCK_BUF_ADDR         0x20000000
#define AppBackAddress        0x08020000 //from 128K
#define ConfigAddr              0x0800FC00

#define NORMAL_STATE          0
#define NEW_APP_IN_BACK       1 //there is new app in back address
#define CONFIGTOOL_FW_UP      2 //configtool update f/w in app


#pragma pack(1)  // 使结构体按1字节对齐，确保结构体紧凑存储

typedef struct _CONFIG_MSG {
    uint8 op[4];     // 操作码，用于标识不同的操作类型，如 FIND、SETT、FACT 等
    uint8 mac[6];    // MAC地址，6字节
    uint8 sw_ver[2]; // 软件版本号，2字节
    uint8 lip[4];    // 本地IP地址，4字节
    uint8 sub[4];    // 子网掩码，4字节
    uint8 gw[4];     // 网关地址，4字节
    uint8 dns[4];    // DNS服务器地址，4字节
    uint8 dhcp;      // DHCP开关，1表示启用，0表示禁用
    uint8 debug;     // 调试模式开关，1表示启用，0表示禁用
    uint16 fw_len;   // 固件长度
    uint8 state;     // 设备状态，可能用于表示正常、更新中等不同状态
} CONFIG_MSG;
#pragma pack()  // 恢复默认的内存对齐方式
extern CONFIG_MSG  ConfigMsg, RecvMsg;
/*
 * 调试信息
 */
#define USING_DEBUG
#ifdef USING_DEBUG
#define log_level  0
#define log_printf(level,...)\
    {\
        if (level <= log_level)\
        {\
            printf(__VA_ARGS__);\
        }\
    }
#define log_err(...)  log_printf(1, __VA_ARGS__)
#define log_warn(...) log_printf(2, __VA_ARGS__)
#define log_info(...) log_printf(3, __VA_ARGS__)
#define log_dbg(...)  log_printf(4, __VA_ARGS__)
#define log_hex(x,len)    do { int i = 0;uint8_t *p=x;for(i=0;i<len;i++) log_info("%02X",p[i]); log_info("\r\n");} while (0)
# define assert(p) do { \
        if (!(p)) { \
            log_err("BUG at assert(%s)\n", #p); \
        }       \
    } while (0)
#else
#define log_err(...)
#define log_warn(...)
#define log_info(...)
#define log_dbg(...)
#define log_hex(x,len)
#define assert(p) ((void)0)
#endif

#define FW_NAME            "Total_control"                    // 固件名
#define FW_VERSION_MAJOR   0                                    // 主版本号
#define FW_VERSION_MINOR   0                                    // 次版本号
#define FW_VERSION_patch   0                                    // 修订版本号

#ifdef __cplusplus
}
#endif

#endif
