/**
 *******************************************************************************
 * @file    et_pwm.c
 * @author  yeelight
 * @version 1.0.0
 * @date    2022-11-22
 * @brief   V1 2022-11-22
 *          create
 *          ET Pulse-width modulation , 用于脉宽调试 方式 控制gpio状态
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
#include <stdint.h>
#include <stdlib.h>

#include "config.h"

typedef struct et_pwm_notify_s {
    uint8_t  type;
    const et_pwm_node_t *node_tab;
} et_pwm_notify_t;

/*******************************************************************************
 * private define macro and struct types
 ******************************************************************************/

/*******************************************************************************
 * private function prototypes
 ******************************************************************************/

/*******************************************************************************
 * private variables
 ******************************************************************************/
static const et_pwm_node_t indic_pwr_up[] = {
    {1, 100}, {0, 500}, {0, 0}
};

static const et_pwm_node_t indic_notify[] = {
    {1, 1}, {0, 1},  {0, 0}
};

static const et_pwm_node_t indic_notify_v2[] = {
    {1, 80}, {0, 1000}, {0, 0}
};

static const et_pwm_notify_t et_pwm_indicators[] = {
    {INDICATOR_PWR_UP,    indic_pwr_up},
    {INDICATOR_NOTIFY,    indic_notify},
    {INDICATOR_NOTIFY_V2, indic_notify_v2},
};

/*******************************************************************************
 * external variables and functions
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
 * private application code, functions definitions
 *******************************************************************************
 ******************************************************************************/
static int32_t gpio_set_value(et_pwm_gpio_desc_t *desc, uint8_t value)
{
    if (!desc) {
        log_err("gpio_desc is null\r\n");
        return -1;
    }

    HAL_GPIO_WritePin(desc->gpio_port, desc->gpio_pin, (GPIO_PinState)value);

    return 0;
}

/****************************** Functions Definitions *************************/
static const et_pwm_node_t *et_pwm_get_tab(uint8_t index)
{
    int i;
    for (i = 0; i < array_size(et_pwm_indicators); i++) {
        if (index == et_pwm_indicators[i].type) {
            return et_pwm_indicators[i].node_tab;
        }
    }
    return NULL;
}

static int et_pwm_ctrl_processing(et_pwm_desc_t *et_pwm_ctrl)
{
    indicator_type_t et_pwm_state_index = et_pwm_ctrl->state.type;

    if (et_pwm_state_index >= INDICATOR_INDEX_MAX) {
        et_pwm_ctrl->state.blinking = false;
        log_err("err, et_pwm_state_index: %d\r\n", et_pwm_ctrl->state.type);

        return -1;
    }

    uint32_t interval_time = 0;

    et_pwm_ctrl->state.type = et_pwm_state_index;
    if (et_pwm_ctrl->state.blinking == false) {

        et_pwm_ctrl->node_tab = et_pwm_get_tab(et_pwm_state_index);
        if (!et_pwm_ctrl->node_tab)
            return -1;

        et_pwm_ctrl->flow_cnt = 0;

    _et_pwm_start:
        et_pwm_ctrl->flow_idx       = 0;
        et_pwm_ctrl->state.blinking = true;
        gpio_set_value(&et_pwm_ctrl->gpio, et_pwm_ctrl->node_tab[et_pwm_ctrl->flow_idx].level ^ et_pwm_ctrl->gpio.gpio_dflt_val);
        interval_time = et_pwm_ctrl->node_tab[et_pwm_ctrl->flow_idx].interval;
        et_pwm_ctrl->flow_idx++;
        return interval_time;
    }

    if (et_pwm_ctrl->node_tab[et_pwm_ctrl->flow_idx].interval == 0) {
        et_pwm_ctrl->flow_cnt++;
        if (et_pwm_ctrl->flow_cnt >= et_pwm_ctrl->repeat_times && et_pwm_ctrl->repeat_times != REPEAT_RUN_FOREVE) {
            gpio_set_value(&et_pwm_ctrl->gpio, et_pwm_ctrl->gpio.gpio_dflt_val);
            return -1;
        } else {
            goto _et_pwm_start;
        }
    } else {
        gpio_set_value(&et_pwm_ctrl->gpio, et_pwm_ctrl->node_tab[et_pwm_ctrl->flow_idx].level ^ et_pwm_ctrl->gpio.gpio_dflt_val);
        interval_time = et_pwm_ctrl->node_tab[et_pwm_ctrl->flow_idx].interval;
        et_pwm_ctrl->flow_idx++;
    }

    return interval_time;
}

static void et_pwm_ctrl_tmr_cb(int tid, void *data)
{
    et_pwm_desc_t *et_pwm = (et_pwm_desc_t *)data;

    uint32_t next_time = et_pwm_ctrl_processing(et_pwm);
    /* log_info("state type: %d, next_time: %d, repeat_times: %d\r\n",
     * et_pwm->flow_cnt, next_time, et_pwm->repeat_times); */

    if (next_time > 0) { // once link done
        start_tmr_with_data(et_pwm->et_pwm_ctrl_tmr,
                            et_pwm_ctrl_tmr_cb,
                            MS_TO_TICKS(next_time),
                            data);
    } else {
        et_pwm->state.type     = INDICATOR_INDEX_MAX;
        et_pwm->state.blinking = false;
        et_pwm->repeat_times   = 0;
        et_pwm->flow_idx       = 0;
        et_pwm->flow_cnt       = 0;
    }
}

void et_pwm_init(et_pwm_desc_t **desc, et_pwm_gpio_desc_t *gpio)
{
    if (!gpio) {
        log_err("init et_pwm gpio is null\r\n");
        return;
    }

    et_pwm_desc_t *et_pwm_ctrl = (et_pwm_desc_t *)calloc(1, sizeof(*et_pwm_ctrl));
    if (!et_pwm_ctrl) {
        log_err("faiet_pwm to calloc for et_pwm_ctrl\r\n");
        return;
    }

    et_pwm_ctrl->et_pwm_ctrl_tmr = (tmr_t *)calloc(1, sizeof(tmr_t));
    if (!et_pwm_ctrl->et_pwm_ctrl_tmr) {
        log_err("faiet_pwm to calloc for et_pwm_ctrl_tmr \r\n");
        return;
    }

    et_pwm_ctrl->gpio.gpio_dflt_val = gpio->gpio_dflt_val;
    et_pwm_ctrl->gpio.gpio_pin      = gpio->gpio_pin;
    et_pwm_ctrl->gpio.gpio_port     = gpio->gpio_port;
    et_pwm_ctrl->state.type         = INDICATOR_NULL;

    log_info("et_pwm ctrl init, gpio[%d], %p\r\n", et_pwm_ctrl->gpio.gpio_pin, et_pwm_ctrl->et_pwm_ctrl_tmr);

    *desc = et_pwm_ctrl;
}

void et_pwm_ctrl_start(et_pwm_desc_t *desc, indicator_type_t type, uint16_t repeat_times)
{
    if (!desc) {
        log_err("%s, et_pwm desc is null\r\n", __FUNCTION__);
        return;
    }

    desc->state.type     = type;
    desc->state.blinking = false;
    desc->repeat_times   = repeat_times;
    desc->flow_idx       = 0;
    desc->flow_cnt       = 0;
    /* log_info("state type: %d, repeat_times: %d\r\n", desc->state.type, desc->repeat_times); */
    start_tmr_with_data(desc->et_pwm_ctrl_tmr,
                        et_pwm_ctrl_tmr_cb,
                        0,  // no delay
                        desc);
}

void et_pwm_on(et_pwm_desc_t *et_pwm)
{
    if (!et_pwm) {
        log_err("et_pwm desc is null\r\n");
        return;
    }
    gpio_set_value(&et_pwm->gpio, !et_pwm->gpio.gpio_dflt_val);
    et_pwm->state.blinking = false;
    et_pwm->state.type     = INDICATOR_INDEX_MAX;
    et_pwm->repeat_times   = 0;
    del_timer(et_pwm->et_pwm_ctrl_tmr);

    //log_info("et_pwm on, gpio: %d \r\n", et_pwm->gpio->gpio_pin);
}

void et_pwm_off(et_pwm_desc_t *et_pwm)
{
    if (!et_pwm) {
        log_err("et_pwm desc is null\r\n");
        return;
    }
    gpio_set_value(&et_pwm->gpio, !!et_pwm->gpio.gpio_dflt_val);

    et_pwm->state.blinking = false;
    et_pwm->state.type     = INDICATOR_INDEX_MAX;
    et_pwm->repeat_times   = 0;
    del_timer(et_pwm->et_pwm_ctrl_tmr);
    // log_info("et_pwm off, gpio: %d \r\n", et_pwm->gpio->gpio_pin);
}

/********************************* end of file ********************************/

