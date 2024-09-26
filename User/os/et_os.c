/**
 *******************************************************************************
 * @file    et_os.c
 * @author  yeelight
 * @version 1.0.0
 * @date    2022-11-11
 * @brief   V1 2022-11-11
 *          create
 *
 *******************************************************************************
 * @attention
 *
 *
 *******************************************************************************
 */

/*******************************************************************************
 * include files
 ******************************************************************************/
#include "et_os.h"
#include "klist.h"
#include "et_timer.h"
#include "string.h"
#include "uart_parse.h"
#include "uart_spec.h"
#include "usart.h"
/*******************************************************************************
 * private define macro and struct types
 ******************************************************************************/
#define TRACE_ENTER()   log_info("[ et ] enter: %s\r\n", __FUNCTION__)

#define YGLC_MAX_REQUSTS         18
#define YGLC_POOL_ITEMS          (YGLC_MAX_REQUSTS)

typedef struct {
    list_head_t    self;
    int            occupied;
    uint8_t        buffer[sizeof(et_request_t)];
} et_buffer_t;

/*******************************************************************************
 * private function prototypes
 ******************************************************************************/
static void et_proc_request(et_request_t *req);

static void et_req_handler(void);


/*******************************************************************************
 * private variables
 ******************************************************************************/
static et_buffer_t et_buffer_pool[YGLC_POOL_ITEMS] = {0};
static KLIST_HEAD(req_head);

/*******************************************************************************
 *******************************************************************************
 * private application code, functions definitions
 *******************************************************************************
 ******************************************************************************/
static et_buffer_t *et_buffer_pool_alloc(void)
{
    for (uint32_t index = 0; index < YGLC_POOL_ITEMS; index++) {
        if (!et_buffer_pool[index].occupied) {
            et_buffer_pool[index].occupied = true;
            return &et_buffer_pool[index];
        }
    }

    return NULL;
}

static void et_buffer_pool_release(et_buffer_t *et_buffer)
{
    et_buffer->occupied = false;
}

void et_task_schedule(void)
{
    proc_timer();

    et_req_handler();
}


/**
 *  The App Entry functon
 */
static int et_prod_init(void)
{
    et_prod_init_func_t *fn;
    int rc;

    for (fn = &et_prod_init_tbl[0]; *fn != NULL; fn++) {
        rc = (*fn)();
        if (rc < 0) {
            log_err("yl product specific init failed.\r\n");
            return rc;
        }
    }

    return 0;
}


int et_task_init(void)
{
    timer_init();

    int rc = et_prod_init();
    if (rc != 0) {
        log_err("et product init err!!\r\n");
        return -1;
    }

    return 0;
}

/**
 * func: Get msg from et queue/list
 */
static void et_req_handler(void)
{
again:

    if (list_empty(&req_head)) {
        return;
    }

    /* test */
    /* et_buffer_t *req_list, *tmp; */
    /* list_for_each_entry_safe(req_list, tmp, &req_head, self, et_buffer_t) { */
    /*     // list_del(&(req_list->self)); */
    /*     et_request_t *et_req = (et_request_t*)(req_list->buffer); */
    /*     LOG_INFO("list_type: 0x%02x \r\n", et_req->type); */
    /* } */

    et_buffer_t  *et_buffer = list_first_entry(&req_head, et_buffer_t, self);
    et_request_t *et_req = (et_request_t *)(et_buffer->buffer);

    et_proc_request(et_req);

    if (et_req->cb_func != NULL) {
        et_req->cb_func((void *)et_req->priv);
    }

    if (et_buffer->self.next != NULL && et_buffer->self.prev != NULL) {
        list_del(&(et_buffer->self));
        et_buffer_pool_release(et_buffer);
    }

    goto again;
}

/**
 * func: Post msg to et queue/list
 */
int et_post_request_async(int type, const void *req, unsigned int size,
                          prot_rsp_cb_t routine, const void *priv)
{
    et_buffer_t *et_buffer = et_buffer_pool_alloc();
    if (NULL == et_buffer) {
        log_err("No et buffer, req:%d is dropped!\r\n", type);
        return -1;
    }

    et_request_t *et_req = (et_request_t *)et_buffer->buffer;
    et_req->type = type;
    et_req->cb_func = routine;
    et_req->priv = priv;

    if (req) {
        memmove(&(et_req->msg), req, size);
    }

    //OS_CPU_SR cpu_sr;

    //enter_critical();
    /* list */
    list_add_tail(&(et_buffer->self), &req_head);
    //exit_critical();

    /* LOGI("post type: %d \r\n", type); */

    return 0;
}


/**
 *  Message distribution
 */
static void et_proc_request(et_request_t *req)
{
    /* log_info("Exec et req, req: %d\r\n", req->type); */

    switch (req->type) {
    case ET_REQ_UART_RECV:
				if(req->msg.uart_recv_req.huart == &huart1){
						uart1_recv_data(req->msg.uart_recv_req.data, req->msg.uart_recv_req.data_len);
				}else
				if(req->msg.uart_recv_req.huart == &huart2){
						uart2_recv_data(req->msg.uart_recv_req.data, req->msg.uart_recv_req.data_len);
				}else
				if(req->msg.uart_recv_req.huart == &huart4){
						uart4_recv_data(req->msg.uart_recv_req.data, req->msg.uart_recv_req.data_len);
				}
        break;

    case ET_REQ_UART_SEND:
            uart_apec_send_data(req->msg.uart_send_req.uart_id, req->msg.uart_send_req.payload_data, req->msg.uart_send_req.payload_len);   
        break;

    default:
        log_err("et: not support the YGLC request id\n");
        break;
    }
}

void et_delay_ms(uint32_t ms)
{
    uint32_t tickstart = 0;
    tickstart = get_sys_time_ms();

    while ((get_sys_time_ms() - tickstart) < ms);
}

void et_delay_us(uint32_t us)
{
    uint32_t tickstart = 0;
    tickstart = get_sys_time_micro();

    while ((get_sys_time_micro() - tickstart) < us);
}

/********************************* end of file ********************************/

