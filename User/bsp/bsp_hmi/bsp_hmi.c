#include "main.h"
#include "et_timer.h"
#include "bsp_hmi.h"
#include "string.h"
#include "hmi_driver.h"
#include "usart.h"
#include "app.h"

static void dacai_process_message(PCTRL_MSG msg, uint16 size);
static void dacai_notify_button(uint16 screen_id, uint16 control_id, uint8 state);
static void dacai_notify_text(uint16 screen_id, uint16 control_id, uint8 *str);
static void NotifyProgress(uint16 screen_id, uint16 control_id, uint32 value);
static void NotifyScreen(uint16 screen_id);

//const char *seat_dir[] = {
//    "一位端",
//    "二位端"
//};

static tmr_t tmr_dacai_handle;
static uint16_t s_ul_hmi_cur_screen_id = 0;

//大彩数据处理
static void tmr_dacai_handle_callback(int timer_id, void *data)
{
    static uint32_t time_count = 0;
//    uint8_t  dacai_send_buff[50];
    uint8_t  dacai_cmd_buffer[64];

    qsize size = queue_find_cmd(dacai_cmd_buffer, CMD_MAX_SIZE); // 从缓冲区中获取一条指令
    if (size > 0 && dacai_cmd_buffer[1] != 0x07) {
        dacai_process_message((PCTRL_MSG) dacai_cmd_buffer, size); // 指令处理
    } else if (size > 0 && dacai_cmd_buffer[1] == 0x07) { // 插入屏幕
        ;
    }

    /* 200ms更新 */
    if (time_count % 20 == 0) {
        ;
    }

    /* 1s 更新 */
    if (time_count % 100 == 0) {
        ;
    }

    time_count++;
}

//启动大彩数据处理定时器
void start_dacai_uart_handle(void)
{
    start_rpt_tmr(&tmr_dacai_handle, tmr_dacai_handle_callback, MS_TO_TICKS(10));
}

//屏幕按钮回调
static void dacai_notify_button(uint16 screen_id, uint16 control_id, uint8 state)
{
    if (screen_id == 0) { //画面0
        if (state == 0)   // 抬起
            return;

    } else if (screen_id == 1) { //画面1

    }

}

//屏幕文本回调
static void dacai_notify_text(uint16 screen_id, uint16 control_id, uint8 *str)
{
    float value = 0;

    sscanf((char *) str, "%f", &value);

    if (screen_id == 0) { //画面0
        switch (control_id) {
        case 6:
            break;
        default:
            break;
        }
    }
}

/*!
*  \brief  进度条控件通知
*  \details  调用GetControlValue时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param value 值
*/
static void NotifyProgress(uint16 screen_id, uint16 control_id, uint32 value)
{
    (void)screen_id;
    (void)control_id;
    (void)value;
}

/*!
*  \brief  画面切换通知
*  \details  当前画面改变时(或调用GetScreen)，执行此函数
*  \param screen_id 当前画面ID
*/
static void NotifyScreen(uint16 screen_id)
{
    s_ul_hmi_cur_screen_id = screen_id;
}

unsigned short Convert(unsigned short s)
{
    char right, left;
    right = s & 0XFF; // 低八位
    left = s >> 8;    // 高八位  右移8位
    s = right * 256 + left;
    return s;
}

//大彩数据解析
static void dacai_process_message(PCTRL_MSG msg, uint16 size)
{
    uint8 cmd_type = msg->cmd_type;               // 指令类型
    uint8 ctrl_msg = msg->ctrl_msg;               // 消息的类型
    uint8 control_type = msg->control_type;       // 控件类型
    uint16 screen_id = Convert(msg->screen_id);   // 画面ID
    uint16 control_id = Convert(msg->control_id); // 控件ID
    uint32 value = PTR2U32(msg->param);

    switch (cmd_type) {
    case NOTIFY_CONTROL: {
        if (ctrl_msg == MSG_GET_CURRENT_SCREEN) {
            NotifyScreen(screen_id);
        } else if (ctrl_msg == MSG_GET_DATA) {
            switch (control_type) {
            case kCtrlButton: // 按钮控件
                dacai_notify_button(screen_id, control_id, msg->param[1]);
                break;
            case kCtrlText: // 文本控件
                dacai_notify_text(screen_id, control_id, msg->param);
                break;
            case kCtrlProgress: // 进度条
                NotifyProgress(screen_id, control_id, value);
                break;
            default:
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}
