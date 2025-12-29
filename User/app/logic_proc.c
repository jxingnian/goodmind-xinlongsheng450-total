/**
 * @file logic_proc.c
 * @brief 业务逻辑处理
 */
#include "logic_proc.h"
#include "uart_parse.h"
#include "uart_spec.h"
#include "bsp_uart.h"
#include "config.h"

static tmr_t tmr_logic_task;

// 命令处理函数声明
static int cmd_read_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len);
static int cmd_write_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len);

// 命令表
static const method_cmd_table_t cmd_table[] = {
    {CMD_READ,  cmd_read_handler},
    {CMD_WRITE, cmd_write_handler},
};

static void logic_task(int timer_id, void *data)
{
    // 处理串口发送队列
    tmr_uart_send_rb_timeout(0, NULL);
    
    // TODO: 添加业务逻辑
}

void logic_proc_init(void)
{
    // 初始化命令处理
    method_cmd_init(&huart2, cmd_table, sizeof(cmd_table)/sizeof(cmd_table[0]));
    method_cmd_init(&huart4, cmd_table, sizeof(cmd_table)/sizeof(cmd_table[0]));
    
    // 启动逻辑任务定时器 50ms
    start_rpt_tmr(&tmr_logic_task, logic_task, MS_TO_TICKS(50));
}

// 读命令响应处理
static int cmd_read_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len)
{
    // TODO: 处理边柜返回的读取数据
    return 0;
}

// 写命令响应处理
static int cmd_write_handler(uint8_t addr, uint16_t opcode, const uint8_t *data, uint32_t len)
{
    // TODO: 处理边柜返回的写入响应
    return 0;
}

/*============================================================================
 * 座椅控制接口实现 (供大彩屏/PIS调用)
 *============================================================================*/

// 发送数据到边柜
static void send_to_side_cabinet(uint8_t addr, uint8_t cmd, uint8_t reg, uint8_t *data, uint8_t data_len)
{
    uart_send_data_t send_data;
    uint8_t buf[10];
    
    buf[0] = reg;
    if (data && data_len > 0) {
        for (int i = 0; i < data_len && i < 4; i++) {
            buf[1 + i] = data[i];
        }
    }
    
    uint8_t len = do_spec_data_package(send_data.uca_data, addr, cmd, buf, 5);
    send_data.uc_data_len = len;
    
    push_uart_send_data(LEFT_CTRL_UART, &send_data);
    push_uart_send_data(RIGHT_CTRL_UART, &send_data);
}

// 急停
void send_seat_rotation_estop(void)
{
    uint8_t data[4] = {0};
    send_to_side_cabinet(0x99, CMD_WRITE, REG_SEAT_ESTOP, data, 4);
    log_info("Seat ESTOP\r\n");
}

// 全部座椅朝向
void send_seat_align_to_direction(uint8_t direction)
{
    uint8_t data[4] = {0};
    data[0] = direction;
    send_to_side_cabinet(0x99, CMD_WRITE, REG_SEAT_ROTATE, data, 4);
    log_info("全部座椅朝向: %d\r\n", direction);
}

// 会议模式
void send_seat_into_meeting_mode(void)
{
    uint8_t data[4] = {0};
    send_to_side_cabinet(0x99, CMD_WRITE, 0x05, data, 4);
    log_info("进入会议模式\r\n");
}

// 会客模式
void send_seat_into_guest_mode(uint8_t mode)
{
    uint8_t data[4] = {0};
    data[0] = mode;
    send_to_side_cabinet(0x99, CMD_WRITE, 0x09, data, 4);
    log_info("会客模式: 0x%02X\r\n", mode);
}

// 单个座椅位置设置
void send_seat_position_set(uint8_t seat_num, uint8_t position)
{
    uint8_t data[4] = {0};
    data[0] = position;
    send_to_side_cabinet(seat_num, CMD_WRITE, REG_SEAT_ROTATE, data, 4);
    log_info("座椅%d 位置: %d\r\n", seat_num, position);
}

// 清除呼叫
void send_reset_call(uint8_t seat_index)
{
    uint8_t data[4] = {0};
    send_to_side_cabinet(seat_index, CMD_WRITE, REG_CALL_CLEAR, data, 4);
    log_info("清除呼叫: %d\r\n", seat_index);
}

// 氛围灯设置
void send_ambient_light_setting(uint8_t seat_num, uint8_t light_status)
{
    uint8_t data[4] = {0};
    data[0] = light_status;
    send_to_side_cabinet(seat_num, CMD_WRITE, REG_LIGHT_CTRL, data, 4);
    log_info("Light seat%d: %d\r\n", seat_num, light_status);
}
