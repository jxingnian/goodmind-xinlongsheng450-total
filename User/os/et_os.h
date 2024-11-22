/**
 *******************************************************************************
 * @file    et_os.h
 * @author  yeelight
 * @version
 * @date    2022-11-11
 * @brief
 *
 *
 *******************************************************************************
 * @attention
 *
 *
 *******************************************************************************
 */

/*******************************************************************************
 ********************* define to prevent recursive inclusion *******************
 ******************************************************************************/
#ifndef _ET_OS_H_
#define _ET_OS_H_

/*******************************************************************************
 ********************************* include files *******************************
 ******************************************************************************/
#include "klist.h"
#include "stdint.h"
#include "config.h"

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

/*******************************************************************************
 ************************ exported macros and struct types *********************
 ******************************************************************************/
#define OS_CPU_SR   uint32_t

#define enter_critical()        \
    do { cpu_sr = __get_PRIMASK(); __disable_irq();} while (0)

#define exit_critical()         \
    do { __set_PRIMASK(cpu_sr);} while (0)

#define ET_POST_REQUEST(type, req, size, routine, priv) \
    et_post_request_async(type, req, size, routine, priv)


typedef enum {
    ET_REQ_UART_RECV,
    ET_REQ_UART_SEND,
    ET_REQ_TYPE_MAX,
} et_req_type_t;

typedef struct {
    UART_HandleTypeDef *huart;
    uint16_t    data_len;
    uint8_t     data[64];
} et_uart_recv_req_t;

typedef struct {
    uint8_t     uart_id;
    uint16_t    payload_len;
    uint8_t     payload_data[24];
} et_uart_send_req_t;

#define LEFT_CTRL_UART  2  // ×ó²àÖ÷¿Ø´®¿Ú
#define RIGHT_CTRL_UART 4  // ÓÒ²à½Å¶´´®¿Ú

/*
 * msg standard format
 */
typedef void (*prot_rsp_cb_t)(const void *);

typedef struct {
    int             type;
    prot_rsp_cb_t   cb_func;
    const void     *priv; /* priv will be passed back in cb_func */
    union {
        int         data[3];      // peripheral module
        et_uart_recv_req_t uart_recv_req;
        et_uart_send_req_t uart_send_req;
    } msg;
} et_request_t;

typedef int (*et_prod_init_func_t)(void);

/*******************************************************************************
 ******************************* exported functions ****************************
 ******************************************************************************/
int et_post_request_async(int type, const void *req, unsigned int size,
                          prot_rsp_cb_t routine, const void *priv);
int et_task_init(void);
void et_task_schedule(void);
void et_delay_ms(uint32_t ms);
void et_delay_us(uint32_t us);

/*******************************************************************************
 ***************************  exported global variables ************************
 ******************************************************************************/
extern et_prod_init_func_t et_prod_init_tbl[];

#endif

/********************************* end of file ********************************/

