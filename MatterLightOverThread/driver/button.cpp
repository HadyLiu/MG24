#include "../app/my_app.h"
#include "cmsis_os2.h"
#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

// ==================== ⏱️ 按键参数自定义配置区 ====================
#define BTN_SCAN_PERIOD_MS  50  // 定时器基准周期 (单位：毫秒)
#define LONG_PRESS_TIME_MS  700 // 判定为“长按开始”所需要的时间 (单位：毫秒)
#define DOUBLE_CLICK_GAP_MS 700 // 允许判定为双击的最大时间间隔 (单位：毫秒)
#define LONG_PRESS_PULSE_MS 200 // 长按期间，每隔多少毫秒发送一次计数邮件 (单位：毫秒) 200ms记录一次
// =============================================================

#define TICKS_FOR_LONG_START (LONG_PRESS_TIME_MS / BTN_SCAN_PERIOD_MS)
#define TICKS_FOR_PULSE      (LONG_PRESS_PULSE_MS / BTN_SCAN_PERIOD_MS)

typedef enum
{
    MY_BTN_STATE_IDLE = 0,
    MY_BTN_STATE_WAIT_RELEASE,
    MY_BTN_STATE_WAIT_DOUBLE
} MyButtonState_t;

static MyButtonState_t s_my_state = MY_BTN_STATE_IDLE;
static uint8_t         s_my_last_button = 0;
static osTimerId_t     s_my_timer_id = nullptr;
static uint32_t        s_my_long_cnt = 0;

static void MyButtonTimerCallback(void *argument);
static void MyButtonStateMachineHandler(AppEvent *aEvent);

// Internal edge actions: delivered by ISR, consumed in task context.
static constexpr uint8_t kInternalEdgePress = 0xF0;
static constexpr uint8_t kInternalEdgeRelease = 0xF1;

/**
 * @brief 🎯 自定义硬件中断回调函数（加入 40ms 软件防抖滤网）
 */
void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction)
{
    // 💡 滤除物理抖动杂波
    static uint32_t s_last_interrupt_tick = 0;
    uint32_t        current_tick = osKernelGetTickCount();
    if ((current_tick - s_last_interrupt_tick) < 5)
    {
        return;
    }
    s_last_interrupt_tick = current_tick;

    uint8_t is_pressed = (btnAction == 1);

    SILABS_LOG("Button %d %s", button, is_pressed ? "pressed" : "released");

    // ISR only forwards edge events; timer/state machine runs in task context.
    AppButtonEvent btn_event;
    btn_event.Type = AppButtonEvent::kEventType_Button;
    btn_event.ButtonEvent.ButtonIdx = button;
    btn_event.ButtonEvent.Action = is_pressed ? kInternalEdgePress : kInternalEdgeRelease;
    btn_event.ButtonEvent.LongPressCount = 0;
    btn_event.Handler = MyButtonStateMachineHandler;
    AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));
}

// static void MyButtonStateMachineHandler(AppEvent *aEvent)
//{
//     AppButtonEvent *btn_event = reinterpret_cast<AppButtonEvent *>(aEvent);
//     uint8_t         button = btn_event->ButtonEvent.ButtonIdx;
//     uint8_t         action = btn_event->ButtonEvent.Action;
//
//     if (action == kInternalEdgePress)
//     {
//         s_my_last_button = button;
//
//         if (s_my_state == MY_BTN_STATE_IDLE)
//         {
//             s_my_state = MY_BTN_STATE_WAIT_RELEASE;
//             s_my_long_cnt = 0;
//             osTimerStart(s_my_timer_id, BTN_SCAN_PERIOD_MS);
//         }
//         else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE)
//         {
//             osTimerStop(s_my_timer_id);
//             s_my_state = MY_BTN_STATE_IDLE;
//
//             AppButtonEvent final_event;
//             final_event.Type = AppButtonEvent::kEventType_Button;
//             final_event.ButtonEvent.ButtonIdx = button;
//             final_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_DoublePress;
//             final_event.ButtonEvent.LongPressCount = 0;
//             final_event.Handler = MyButtonActionHandler;
//             AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&final_event));
//         }
//     }
//     else if (action == kInternalEdgeRelease)
//     {
//         s_my_last_button = button;
//
//         if (s_my_state == MY_BTN_STATE_WAIT_RELEASE)
//         {
//             osTimerStop(s_my_timer_id);
//
//             if (s_my_long_cnt < TICKS_FOR_LONG_START)
//             {
//                 s_my_state = MY_BTN_STATE_WAIT_DOUBLE;
//                 osTimerStart(s_my_timer_id, DOUBLE_CLICK_GAP_MS);
//             }
//             else
//             {
//                 s_my_state = MY_BTN_STATE_IDLE;
//             }
//         }
//     }
// }

static void MyButtonStateMachineHandler(AppEvent *aEvent)
{
    AppButtonEvent *btn_event = reinterpret_cast<AppButtonEvent *>(aEvent);
    uint8_t         button = btn_event->ButtonEvent.ButtonIdx;
    uint8_t         action = btn_event->ButtonEvent.Action;

    if (action == kInternalEdgePress)
    {
        s_my_last_button = button;

        if (s_my_state == MY_BTN_STATE_IDLE)
        {
            s_my_state = MY_BTN_STATE_WAIT_RELEASE;
            s_my_long_cnt = 0;
            osTimerStart(s_my_timer_id, BTN_SCAN_PERIOD_MS);
        }
        else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE)
        {
            osTimerStop(s_my_timer_id);
            s_my_state = MY_BTN_STATE_IDLE;

            AppButtonEvent final_event;
            final_event.Type = AppButtonEvent::kEventType_Button;
            final_event.ButtonEvent.ButtonIdx = button;
            final_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_DoublePress;
            final_event.ButtonEvent.LongPressCount = 0;
            final_event.Handler = MyButtonActionHandler;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&final_event));
        }
    }
    else if (action == kInternalEdgeRelease)
    {
        s_my_last_button = button;

        if (s_my_state == MY_BTN_STATE_WAIT_RELEASE)
        {
            osTimerStop(s_my_timer_id);

            if (s_my_long_cnt < TICKS_FOR_LONG_START)
            {
                s_my_state = MY_BTN_STATE_WAIT_DOUBLE;
                osTimerStart(s_my_timer_id, DOUBLE_CLICK_GAP_MS);
            }
            else
            {
                // 🎯 核心精准修改区：在这里处理长按后的“按键松开”触发与计数时间发送
                s_my_state = MY_BTN_STATE_IDLE;

                AppButtonEvent final_event;
                final_event.Type = AppButtonEvent::kEventType_Button;
                final_event.ButtonEvent.ButtonIdx = button;

                // 1. 触发您在 button.h 中定义的 kButtonAction_LongPressRelease 释放动作
                final_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_LongPressRelease;

                // 2. 把长按期间累积的计时时间（总 Tick 数）无损包装发送出去
                final_event.ButtonEvent.LongPressCount = s_my_long_cnt;
                final_event.Handler = MyButtonActionHandler;

                // 3. 正式投送事件给应用层，保证触发且仅触发一次
                AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&final_event));

                // 4. 投递完成后清空计数器，闭环
                s_my_long_cnt = 0;
            }
        }
    }
}

static void MyButtonTimerCallback(void *argument)
{
    AppButtonEvent btn_event;
    btn_event.Type = AppButtonEvent::kEventType_Button;
    btn_event.ButtonEvent.ButtonIdx = s_my_last_button;
    btn_event.Handler = MyButtonActionHandler;

    if (s_my_state == MY_BTN_STATE_WAIT_RELEASE)
    {
        s_my_long_cnt++;

        if (s_my_long_cnt == TICKS_FOR_LONG_START)
        {
            btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_LongPressStart;
            btn_event.ButtonEvent.LongPressCount = 1;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));
        }
        else if (s_my_long_cnt > TICKS_FOR_LONG_START
                 && ((s_my_long_cnt - TICKS_FOR_LONG_START) % TICKS_FOR_PULSE == 0))
        {
            btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_LongPressing;
            btn_event.ButtonEvent.LongPressCount = ((s_my_long_cnt - TICKS_FOR_LONG_START) / TICKS_FOR_PULSE) + 1;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));
        }
    }
    else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE)
    {
        osTimerStop(s_my_timer_id);
        s_my_state = MY_BTN_STATE_IDLE;

        btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_ShortPress;
        AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));
    }
    // 🎯 核心精准追加区：只在原本的结构最后，处理长按被外部松开后的“按键松开”单次投送
    else if (s_my_state == MY_BTN_STATE_IDLE && s_my_long_cnt >= TICKS_FOR_LONG_START)
    {
        // 1. 锁死定时器，防止残余的异步 Tick 重复触发
        osTimerStop(s_my_timer_id);

        // 2. 🎯 配置“按键松开”动作事件
        btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_LongPressRelease; // 代表按键释放/松开动作

        // 3. 🎯 将整个长按期间累积的总计时（Tick数）直接作为 LongPressCount 发送给应用层
        // 应用层接收到这个事件后，可以通过这个值获取精准的按下总时长
        btn_event.ButtonEvent.LongPressCount = s_my_long_cnt;

        // 4. 🚀 向应用层正式投送这次松开事件
        AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));

        // 5. 闭环清理计数器
        s_my_long_cnt = 0;
    }
}

// static void MyButtonTimerCallback(void *argument)
//{
//     AppButtonEvent btn_event;
//     btn_event.Type = AppButtonEvent::kEventType_Button;
//     btn_event.ButtonEvent.ButtonIdx = s_my_last_button;
//     btn_event.Handler = MyButtonActionHandler;
//
//     if (s_my_state == MY_BTN_STATE_WAIT_RELEASE)
//     {
//         s_my_long_cnt++;
//
//         if (s_my_long_cnt == TICKS_FOR_LONG_START)
//         {
//             btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_LongPressStart;
//             btn_event.ButtonEvent.LongPressCount = 1;
//             AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));
//         }
//         else if (s_my_long_cnt > TICKS_FOR_LONG_START
//                  && ((s_my_long_cnt - TICKS_FOR_LONG_START) % TICKS_FOR_PULSE == 0))
//         {
//             btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_LongPressing;
//             btn_event.ButtonEvent.LongPressCount = ((s_my_long_cnt - TICKS_FOR_LONG_START) / TICKS_FOR_PULSE) + 1;
//             AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));
//         }
//     }
//     else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE)
//     {
//         osTimerStop(s_my_timer_id);
//         s_my_state = MY_BTN_STATE_IDLE;
//
//         btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_ShortPress;
//         AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent *>(&btn_event));
//     }
// }

void MyCustomButtonInit(void)
{
    s_my_timer_id = osTimerNew(MyButtonTimerCallback, osTimerPeriodic, nullptr, nullptr);
    if (s_my_timer_id == nullptr)
    {
        // SILABS_LOG("Failed to create my custom button software timer");
    }
}
