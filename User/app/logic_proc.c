/**
 * @file logic_proc.c
 * @brief 业务逻辑处理
 */
#include "logic_proc.h"
#include "uart_parse.h"
#include "uart_spec.h"
#include "bsp_uart.h"
#include "config.h"

/* 全局座椅信息 */
seat_info_t g_seat_info[SEAT_COUNT] = {0};
uint8_t g_call_status = 0;  // 呼叫状态位图

static tmr_t tmr_logic_task;

/* 轮询状态机 */
static uint8_t s_poll_seat_idx = 0;    // 当前轮询的座椅索引
static uint8_t s_poll_reg_idx = 0;     // 当前轮询的寄存器索引

/* 轮询寄存器列表 */
static const uint8_t poll_regs[] = {
    REG_SEAT_STATUS,   // 座椅旋转状态
    REG_CALL_STATUS,   // 呼叫状态
    REG_LIGHT_STATUS,  // 氛围灯状态
};
#define POLL_REG_COUNT  (sizeof(poll_regs) / sizeof(poll_regs[0]))

/* 函数声明 */
static int cmd_read_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len);
static int cmd_write_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len);
static void send_to_side_cabinet(uint8_t addr, uint8_t cmd, uint8_t reg, uint8_t *data, uint8_t data_len);
static void poll_seat_status(void);

/* 命令表 */
static const method_cmd_table_t cmd_table[] = {
    {CMD_READ,  cmd_read_handler},
    {CMD_WRITE, cmd_write_handler},
};

/*============================================================================
 * 逻辑任务
 *============================================================================*/
static void logic_task(int timer_id, void *data)
{
    static uint32_t tick_count = 0;
    
    // 处理串口发送队列
    if (tmr_uart_send_rb_timeout(0, NULL) != 0) {
        return;  // 有数据在发送，等待下次
    }
    
    // 每200ms轮询一次座椅状态
    if (tick_count % 4 == 0) {
        poll_seat_status();
    }
    
    tick_count++;
}

/* 轮询座椅状态 */
static void poll_seat_status(void)
{
    uint8_t addr = s_poll_seat_idx + 1;  // 座椅地址从1开始
    uint8_t reg = poll_regs[s_poll_reg_idx];
    uint8_t data[4] = {0};
    
    // 发送读取命令
    send_to_side_cabinet(addr, CMD_READ, reg, data, 4);
    
    // 切换到下一个寄存器
    s_poll_reg_idx++;
    if (s_poll_reg_idx >= POLL_REG_COUNT) {
        s_poll_reg_idx = 0;
        // 切换到下一个座椅
        s_poll_seat_idx++;
        if (s_poll_seat_idx >= SEAT_COUNT) {
            s_poll_seat_idx = 0;
        }
    }
}

void logic_proc_init(void)
{
    // 初始化命令处理
    method_cmd_init(&huart2, cmd_table, sizeof(cmd_table)/sizeof(cmd_table[0]));
    method_cmd_init(&huart4, cmd_table, sizeof(cmd_table)/sizeof(cmd_table[0]));
    
    // 启动逻辑任务定时器 50ms
    start_rpt_tmr(&tmr_logic_task, logic_task, MS_TO_TICKS(50));
}

/*============================================================================
 * 命令响应处理
 *============================================================================*/

/* 读命令响应处理 */
static int cmd_read_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len)
{
    if (addr < 1 || addr > SEAT_COUNT) {
        return -1;
    }
    
    uint8_t seat_idx = addr - 1;
    uint8_t reg = data[0];
    
    switch (reg) {
    case REG_SEAT_STATUS:
        g_seat_info[seat_idx].status = data[4];
        g_seat_info[seat_idx].position = data[3];
        log_dbg("Seat%d status:%d pos:%d\r\n", addr, data[4], data[3]);
        break;
        
    case REG_CALL_STATUS:
        g_seat_info[seat_idx].call = data[4];
        // 更新呼叫位图
        if (data[4]) {
            g_call_status |= (1 << seat_idx);
        } else {
            g_call_status &= ~(1 << seat_idx);
        }
        log_dbg("Seat%d call:%d\r\n", addr, data[4]);
        break;
        
    case REG_LIGHT_STATUS:
        g_seat_info[seat_idx].light = data[4];
        log_dbg("Seat%d light:%d\r\n", addr, data[4]);
        break;
        
    default:
        break;
    }
    
    return 0;
}

/* 写命令响应处理 */
static int cmd_write_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len)
{
    // 写命令响应，一般只需确认
    log_dbg("Write resp addr:%d\r\n", addr);
    return 0;
}

/*============================================================================
 * 发送数据到边柜
 *============================================================================*/
static void send_to_side_cabinet(uint8_t addr, uint8_t cmd, uint8_t reg, uint8_t *data, uint8_t data_len)
{
    uart_send_data_t send_data;
    uint8_t buf[10];
    
    buf[0] = reg;
    for (int i = 0; i < 4; i++) {
        buf[1 + i] = (data && i < data_len) ? data[i] : 0;
    }
    
    uint8_t len = do_spec_data_package(send_data.uca_data, addr, cmd, buf, 5);
    send_data.uc_data_len = len;
    
    push_uart_send_data(LEFT_CTRL_UART, &send_data);
    push_uart_send_data(RIGHT_CTRL_UART, &send_data);
}

/*============================================================================
 * 座椅控制接口实现
 *============================================================================*/

/* 急停 */
void send_seat_rotation_estop(void)
{
    uint8_t data[4] = {0};
    send_to_side_cabinet(0x99, CMD_WRITE, REG_SEAT_ESTOP, data, 4);
    log_info("Seat ESTOP\r\n");
}

/* 全部座椅朝向 */
void send_seat_align_to_direction(uint8_t direction)
{
    uint8_t data[4] = {0};
    
    switch (direction) {
    case 1:
        data[0] = SEAT_ALL_1WEIDUAN;
        break;
    case 2:
        data[0] = SEAT_ALL_2WEIDUAN;
        break;
    case 3:
        data[0] = SEAT_ALL_ZHENGQIAN;
        break;
    case 4:
        data[0] = SEAT_ALL_ZHENGHOU;
        break;
    default:
        return;
    }
    
    send_to_side_cabinet(0x99, CMD_WRITE, REG_SEAT_ROTATE, data, 4);
    log_info("All seat dir:%d\r\n", direction);
}

/* 会议模式 */
void send_seat_into_meeting_mode(void)
{
    uint8_t data[4] = {0};
    data[0] = SEAT_MEETING_6REN;
    send_to_side_cabinet(0x99, CMD_WRITE, REG_SEAT_ROTATE, data, 4);
    log_info("Meeting mode\r\n");
}

/* 会客模式 */
void send_seat_into_guest_mode(uint8_t mode)
{
    uint8_t data[4] = {0};
    data[0] = mode;
    send_to_side_cabinet(0x99, CMD_WRITE, REG_SEAT_ROTATE, data, 4);
    log_info("Guest mode:0x%02X\r\n", mode);
}

/* 单个座椅位置设置 */
void send_seat_position_set(uint8_t seat_num, uint8_t position)
{
    uint8_t data[4] = {0};
    
    switch (position) {
    case 0:
        data[0] = SEAT_POS_ZHENGQIAN;
        break;
    case 1:
        data[0] = SEAT_POS_ZHENGHOU;
        break;
    case 2:
        data[0] = SEAT_POS_GUODAO;
        break;
    case 3:
        data[0] = SEAT_POS_1WEIDUAN;
        break;
    case 4:
        data[0] = SEAT_POS_2WEIDUAN;
        break;
    default:
        return;
    }
    
    send_to_side_cabinet(seat_num, CMD_WRITE, REG_SEAT_ROTATE, data, 4);
    log_info("Seat%d pos:%d\r\n", seat_num, position);
}

/* 座椅复位(回到坐姿) */
void send_seat_reset(uint8_t seat_num)
{
    uint8_t data[4] = {0};
    data[0] = seat_num;  // 0=全部
    send_to_side_cabinet(seat_num ? seat_num : 0x99, CMD_WRITE, REG_SEAT_RESET, data, 4);
    log_info("Seat reset:%d\r\n", seat_num);
}

/* 清除呼叫 */
void send_reset_call(uint8_t seat_index)
{
    uint8_t data[4] = {0};
    
    if (seat_index == 0x99) {
        // 清除全部
        data[0] = 0x3F;  // bit0-5全部置1
    } else if (seat_index >= 1 && seat_index <= SEAT_COUNT) {
        data[0] = (1 << (seat_index - 1));
    }
    
    send_to_side_cabinet(seat_index, CMD_WRITE, REG_CALL_CLEAR, data, 4);
    log_info("Clear call:%d\r\n", seat_index);
}

/* 氛围灯设置 */
void send_ambient_light_setting(uint8_t seat_num, uint8_t light_status)
{
    uint8_t data[4] = {0};
    data[0] = light_status;
    send_to_side_cabinet(seat_num, CMD_WRITE, REG_LIGHT_CTRL, data, 4);
    log_info("Light seat%d:%d\r\n", seat_num, light_status);
}
{
    uint8_t data[4] = {0};
    data[0] = light_status;
    send_to_side_cabinet(seat_num, CMD_WRITE, REG_LIGHT_CTRL, data, 4);
    log_info("Light seat%d: %d\r\n", seat_num, light_status);
}
