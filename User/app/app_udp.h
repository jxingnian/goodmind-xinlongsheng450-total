/**
 * @file app_udp.h
 * @brief W5500 UDP通讯
 */
#ifndef _APP_UDP_H_
#define _APP_UDP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_udp_init(void);
void app_udp_send(uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
