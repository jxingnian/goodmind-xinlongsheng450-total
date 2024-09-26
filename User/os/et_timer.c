/**
 *******************************************************************************
 * @file    et_timer.c
 * @author  yeelight
 * @version 1.0.0
 * @date    2022-11-01
 * @brief   V1 2022-11-01
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
#include "et_timer.h"
#include "et_os.h"

/*******************************************************************************
 * private define macro and struct types
 ******************************************************************************/
/* t1 is earlier than t2 */
#define time_before(t1, t2)      ((int)((t1) - (t2)) <= 0)
#define timer_before(tmr1, tmr2) (time_before(tmr1->expire, tmr2->expire))

typedef struct tmr_cb {
    struct list_head  tmr_list;
    volatile timer_ms_t  sys_msec;
} tmr_cb_t;

static tmr_cb_t thdl;

/*******************************************************************************
 *******************************************************************************
 * private application code, functions definitions
 *******************************************************************************
 ******************************************************************************/
/**
 * Update the current time.
 * Because all timer's expiring time is relative to current time, so we must
 * update current time after each time-consuming operations.
 */
static void time_update(void)
{
    thdl.sys_msec = get_sys_time_micro();

    return;
}

void timer_init(void)
{
    INIT_LIST_HEAD(&(thdl.tmr_list));

    time_update();
}

void del_timer(tmr_t *timer)
{
    if (timer->self.next != NULL && timer->self.prev != NULL) {
        list_del(&(timer->self));
    }
}

/**
 * Place the timer into timer queue.
 */
void add_timer(tmr_t *timer)
{
    tmr_t *tmr;

    del_timer(timer); // try del it before add timer

    timer_ms_t now_ms = thdl.sys_msec;

    if (time_before(now_ms, timer->expire + MS_PER_TICK) && time_before(timer->expire, now_ms)) {
        /* assume timer was restarted */
        timer->expire = timer->expire + timer->val * MS_PER_TICK;
    } else {
        timer->expire = now_ms + timer->val * MS_PER_TICK;
    }

    INIT_LIST_HEAD(&(timer->self));

    list_for_each_entry(tmr, &(thdl.tmr_list), self) {
        if (timer_before(timer, tmr)) {
            break;
        }
    }
    //OS_CPU_SR cpu_sr;

    //enter_critical();
    list_add_tail(&(timer->self), &(tmr->self));
    //exit_critical();

}

/**
 * Reset timer based on last expiration time
 * Place the timer into timer queue.
 */
void timer_re_add(tmr_t *timer)
{
    tmr_t *tmr;

    timer->expire += timer->val * MS_PER_TICK;
    INIT_LIST_HEAD(&(timer->self));

    list_for_each_entry(tmr, &(thdl.tmr_list), self) {
        if (timer_before(timer, tmr)) {
            break;
        }
    }

    list_add_tail(&(timer->self), &(tmr->self));
}

/*
 * timer is running, return 1, other return 0
 */
int timer_is_running(tmr_t *timer)
{
    if (timer->self.next != NULL && timer->self.prev != NULL) {
        return 1;
    }
    return 0;
}

/*
 * return the remain time. unit: ms
 */
timer_ms_t timer_expire(tmr_t *timer)
{
    tmr_cb_t *tcb = (tmr_cb_t *)&thdl;

    if (timer_is_running(timer)) {
        if (time_before(timer->expire, tcb->sys_msec)) {
            return 0;
        } else {
            return (timer->expire - tcb->sys_msec);
        }
    }

    return 0;
}

/**
 * Do callbacks for all the expired timer, restart the timer
 * if it's repeatitive.
 */
void proc_timer(void)
{
    tmr_cb_t *tcb = (tmr_cb_t *)&thdl;
    tmr_t *tmr;

    time_update();

    for (;;) {
        if (list_empty(&(tcb->tmr_list))) {
            break;
        }

        tmr = list_first_entry(&(tcb->tmr_list), tmr_t, self);

        if (time_before(tmr->expire, tcb->sys_msec)) {
            del_timer(tmr);
            if (tmr->repeat) {
                add_timer(tmr);
            }
            tmr->fn(tmr->timer_id, tmr->data);
        } else {
            break;
        }
    }
}

/**
 * Find out how much time can we sleep before we need to
 * wake up to handle the timer.
 */
int get_next_timeout(timer_ms_t *tick)
{
    tmr_cb_t *tcb = (tmr_cb_t *)&thdl;
    tmr_t *tmr;

    if (list_empty(&(tcb->tmr_list))) {
        return -1;
    }

    tmr = list_first_entry(&(tcb->tmr_list), tmr_t, self);

    if (time_before(tmr->expire, tcb->sys_msec)) {
        *tick = 0;
    } else {
        *tick = tmr->expire - tcb->sys_msec;
    }

    return 0;
}

int time2expire(void)
{
    tmr_cb_t *tcb = (tmr_cb_t *)&thdl;
    if (list_empty(&(tcb->tmr_list))) {
        return  24 * 3600 * 1000ull; // 18h is insanely long
    }
    tmr_t *tmr = list_first_entry(&(tcb->tmr_list), tmr_t, self);

    time_update();

    if (!tmr || !timer_is_running(tmr))
        return  24 * 3600 * 1000ull; // 18h is insanely long

    return tmr->expire - tcb->sys_msec;
}

/********************************* end of file ********************************/
