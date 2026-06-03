//#include "../app/my_app.h"
//#include "cmsis_os2.h" 
//#include "AppTask.h"
//#include "AppConfig.h"
//#include "AppEvent.h"
//#include "LEDWidget.h"
////#include "sl_log.h"
//
//// ==================== ⏱️ 按键参数自定义配置区 ====================
//// 统一在此处定义，避免与头文件或原厂宏冲突
//#define BTN_SCAN_PERIOD_MS    50   // 定时器基准周期 (单位：毫秒)
//#define LONG_PRESS_TIME_MS    700  // 判定为“长按开始”所需要的时间 (单位：毫秒)
//#define DOUBLE_CLICK_GAP_MS   500  // 允许判定为双击的最大时间间隔 (单位：毫秒)
//#define LONG_PRESS_PULSE_MS   200  // 长按期间，每隔多少毫秒发送一次计数邮件 (单位：毫秒)
//// =============================================================
//
//// 根据毫秒数自动换算成状态机需要的计数次数 (Ticks)
//#define TICKS_FOR_LONG_START  (LONG_PRESS_TIME_MS / BTN_SCAN_PERIOD_MS)
//#define TICKS_FOR_PULSE       (LONG_PRESS_PULSE_MS / BTN_SCAN_PERIOD_MS)
//
//// 自定义按键状态机的内部状态
//typedef enum {
//    MY_BTN_STATE_IDLE = 0,       // 空闲状态
//    MY_BTN_STATE_WAIT_RELEASE,   // 按键按下，等待松开
//    MY_BTN_STATE_WAIT_DOUBLE     // 按键松开，等待第二下
//} MyButtonState_t;
//
//// 状态机私有全局静态变量
//static MyButtonState_t s_my_state = MY_BTN_STATE_IDLE;
//static uint8_t s_my_last_button   = 0;
//static osTimerId_t s_my_timer_id  = nullptr;
//static uint32_t s_my_long_cnt     = 0;
//
//// 声明私有定时器回调函数
//static void MyButtonTimerCallback(void *argument);
//
///**
// * @brief 🎯 自定义硬件中断回调函数
// */
//void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction)
//{
//   // 💡 引入防抖机制：两次中断之间的间隔如果小于 40ms，直接判定为机械抖动噪声
//    static uint32_t s_last_interrupt_tick = 0;
//    uint32_t current_tick = osKernelGetTickCount(); // 获取当前系统 Tick (通常 1 Tick = 1ms)
//    
//    if ((current_tick - s_last_interrupt_tick) < 40) 
//    {
//        // 间隔太短，属于物理抖动，直接无视，退出中断
//        return; 
//    }
//    s_last_interrupt_tick = current_tick; // 更新有效中断时间
//
//    // ─── 下方完全保留你原本的状态机推推乐逻辑 ───
//    uint8_t is_pressed = (btnAction == 1); 
//    s_my_last_button   = button;
//
//    SILABS_LOG("Button %d %s", button, is_pressed ? "pressed" : "released");
//    
//    AppButtonEvent btn_event;
//    btn_event.Type                  = AppButtonEvent::kEventType_Button;
//    btn_event.ButtonEvent.ButtonIdx = button;
//    btn_event.ButtonEvent.LongPressCount = 0; // 记得初始化清零，防止野数据
//    btn_event.Handler               = MyButtonActionHandler; 
//
//    if (is_pressed) 
//    {
//        if (s_my_state == MY_BTN_STATE_IDLE)
//        {
//            s_my_state    = MY_BTN_STATE_WAIT_RELEASE;
//            s_my_long_cnt = 0;
//            osTimerStart(s_my_timer_id, BTN_SCAN_PERIOD_MS); 
//        }
//        else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE)
//        {
//            osTimerStop(s_my_timer_id);
//            s_my_state = MY_BTN_STATE_IDLE;
//
//            btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_DoublePress;
//            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
//        }
//    }
//    else 
//    {
//        if (s_my_state == MY_BTN_STATE_WAIT_RELEASE)
//        {
//            osTimerStop(s_my_timer_id);
//
//            if (s_my_long_cnt < TICKS_FOR_LONG_START) 
//            {
//                s_my_state = MY_BTN_STATE_WAIT_DOUBLE;
//                osTimerStart(s_my_timer_id, DOUBLE_CLICK_GAP_MS); 
//            }
//            else
//            {
//                s_my_state = MY_BTN_STATE_IDLE;
//                // 如果需要，这里也可以补充一个 LongPressEnd 事件发出去
//            }
//        }
//    }
//}
//
///**
// * @brief ⏰ 辅助软件定时器回调函数
// */
//static void MyButtonTimerCallback(void *argument)
//{
//    AppButtonEvent btn_event;
//    btn_event.Type    = AppButtonEvent::kEventType_Button;
//    btn_event.Handler = MyButtonActionHandler; 
//
//    uint32_t packed_data = s_my_last_button;
//
//    if (s_my_state == MY_BTN_STATE_WAIT_RELEASE) // 【场景 A：按住没松开】
//    {
//        s_my_long_cnt++;
//
//        // 1. 刚好达到了长按判定的时间点
//        if (s_my_long_cnt == TICKS_FOR_LONG_START) 
//        {
//            packed_data |= (AppButtonEvent::kButtonAction_LongPressStart << 8);
//            packed_data |= (1 << 16); // LongPressCount = 1 (高16位)
//            btn_event.ButtonEvent.Action = packed_data;
//            
//            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
//        }
//        // 2. 持续长按中，后续发出持续脉冲邮件
//        else if (s_my_long_cnt > TICKS_FOR_LONG_START && 
//                ((s_my_long_cnt - TICKS_FOR_LONG_START) % TICKS_FOR_PULSE == 0)) 
//        {
//            uint16_t count = ((s_my_long_cnt - TICKS_FOR_LONG_START) / TICKS_FOR_PULSE) + 1;
//            packed_data |= (AppButtonEvent::kButtonAction_LongPressing << 8);
//            packed_data |= (count << 16); // 打包脉冲计数到高位
//            btn_event.ButtonEvent.Action = packed_data;
//            
//            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
//        }
//    }
//    else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE) // 【场景 B：超时未等到第二下】
//    {
//        osTimerStop(s_my_timer_id);
//        s_my_state = MY_BTN_STATE_IDLE;
//
//        packed_data |= (AppButtonEvent::kButtonAction_ShortPress << 8);
//        btn_event.ButtonEvent.Action = packed_data;
//        
//        AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
//    }
//}
//
///**
// * @brief 🔌 初始化自定义按键服务的软件定时器
// */
//void MyCustomButtonInit(void)
//{
//    s_my_timer_id = osTimerNew(MyButtonTimerCallback, osTimerPeriodic, nullptr, nullptr);
//    if (s_my_timer_id == nullptr)
//    {
//        //SILABS_LOG("Failed to create my custom button software timer");
//    }
//}

#include "../app/my_app.h"
#include "cmsis_os2.h" 
#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"
//#include "sl_log.h"

// ==================== ⏱️ 按键参数自定义配置区 ====================
#define BTN_SCAN_PERIOD_MS    50   // 定时器基准周期 (单位：毫秒)
#define LONG_PRESS_TIME_MS    1000 // 判定为“长按开始”所需要的时间 (单位：毫秒)
#define DOUBLE_CLICK_GAP_MS   350  // 允许判定为双击的最大时间间隔 (单位：毫秒)
#define LONG_PRESS_PULSE_MS   200  // 长按期间，每隔多少毫秒发送一次计数邮件 (单位：毫秒)
// =============================================================

#define TICKS_FOR_LONG_START  (LONG_PRESS_TIME_MS / BTN_SCAN_PERIOD_MS)
#define TICKS_FOR_PULSE       (LONG_PRESS_PULSE_MS / BTN_SCAN_PERIOD_MS)

typedef enum {
    MY_BTN_STATE_IDLE = 0,       
    MY_BTN_STATE_WAIT_RELEASE,   
    MY_BTN_STATE_WAIT_DOUBLE     
} MyButtonState_t;

static MyButtonState_t s_my_state = MY_BTN_STATE_IDLE;
static uint8_t s_my_last_button   = 0;
static osTimerId_t s_my_timer_id  = nullptr;
static uint32_t s_my_long_cnt     = 0;

static void MyButtonTimerCallback(void *argument);
static void MyButtonStateMachineHandler(AppEvent * aEvent);

// Internal edge actions: delivered by ISR, consumed in task context.
static constexpr uint8_t kInternalEdgePress   = 0xF0;
static constexpr uint8_t kInternalEdgeRelease = 0xF1;

/**
 * @brief 🎯 自定义硬件中断回调函数（加入 40ms 软件防抖滤网）
 */
void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction)
{
    // 💡 滤除物理抖动杂波
    static uint32_t s_last_interrupt_tick = 0;
    uint32_t current_tick = osKernelGetTickCount(); 
    if ((current_tick - s_last_interrupt_tick) < 40) 
    {
        return; 
    }
    s_last_interrupt_tick = current_tick;

    uint8_t is_pressed = (btnAction == 1);

    SILABS_LOG("Button %d %s", button, is_pressed ? "pressed" : "released");

    // ISR only forwards edge events; timer/state machine runs in task context.
    AppButtonEvent btn_event;
    btn_event.Type                  = AppButtonEvent::kEventType_Button;
    btn_event.ButtonEvent.ButtonIdx = button;
    btn_event.ButtonEvent.Action    = is_pressed ? kInternalEdgePress : kInternalEdgeRelease;
    btn_event.ButtonEvent.LongPressCount = 0;
    btn_event.Handler               = MyButtonStateMachineHandler;
    AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event));
}

static void MyButtonStateMachineHandler(AppEvent * aEvent)
{
    AppButtonEvent * btn_event = reinterpret_cast<AppButtonEvent *>(aEvent);
    uint8_t button             = btn_event->ButtonEvent.ButtonIdx;
    uint8_t action             = btn_event->ButtonEvent.Action;

    if (action == kInternalEdgePress)
    {
        s_my_last_button = button;

        if (s_my_state == MY_BTN_STATE_IDLE)
        {
            s_my_state    = MY_BTN_STATE_WAIT_RELEASE;
            s_my_long_cnt = 0;
            osTimerStart(s_my_timer_id, BTN_SCAN_PERIOD_MS);
        }
        else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE)
        {
            osTimerStop(s_my_timer_id);
            s_my_state = MY_BTN_STATE_IDLE;

            AppButtonEvent final_event;
            final_event.Type                  = AppButtonEvent::kEventType_Button;
            final_event.ButtonEvent.ButtonIdx = button;
            final_event.ButtonEvent.Action    = AppButtonEvent::kButtonAction_DoublePress;
            final_event.ButtonEvent.LongPressCount = 0;
            final_event.Handler               = MyButtonActionHandler;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&final_event));
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
                s_my_state = MY_BTN_STATE_IDLE;
            }
        }
    }
}

static void MyButtonTimerCallback(void *argument)
{
    AppButtonEvent btn_event;
    btn_event.Type                  = AppButtonEvent::kEventType_Button;
    btn_event.ButtonEvent.ButtonIdx = s_my_last_button;
    btn_event.Handler               = MyButtonActionHandler; 

    if (s_my_state == MY_BTN_STATE_WAIT_RELEASE) 
    {
        s_my_long_cnt++;

        if (s_my_long_cnt == TICKS_FOR_LONG_START) 
        {
            btn_event.ButtonEvent.Action         = AppButtonEvent::kButtonAction_LongPressStart;
            btn_event.ButtonEvent.LongPressCount = 1;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
        }
        else if (s_my_long_cnt > TICKS_FOR_LONG_START && 
                ((s_my_long_cnt - TICKS_FOR_LONG_START) % TICKS_FOR_PULSE == 0)) 
        {
            btn_event.ButtonEvent.Action         = AppButtonEvent::kButtonAction_LongPressing;
            btn_event.ButtonEvent.LongPressCount = ((s_my_long_cnt - TICKS_FOR_LONG_START) / TICKS_FOR_PULSE) + 1;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
        }
    }
    else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE) 
    {
        osTimerStop(s_my_timer_id);
        s_my_state = MY_BTN_STATE_IDLE;

        btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_ShortPress;
        AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
    }
}

void MyCustomButtonInit(void)
{
    s_my_timer_id = osTimerNew(MyButtonTimerCallback, osTimerPeriodic, nullptr, nullptr);
    if (s_my_timer_id == nullptr)
    {
       // SILABS_LOG("Failed to create my custom button software timer");
    }
}
