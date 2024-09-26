#ifndef _APP_UDP_H__
#define _APP_UDP_H__


void platform_init(void);								// initialize the dependent host peripheral
void network_init(void);								// Initialize Network information and display it
void app_udp_init(void);
void app_udp_send_data(uint8_t *data, uint16_t len);

#endif
