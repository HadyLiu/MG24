#include "../app/my_app.h"
//#include "sl_matter_logging.h"
#include "cmsis_os2.h" // 引入 SiSDK 2025 标准系统定时器


#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

#include "LEDWidget.h"
#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
#include "RGBLEDWidget.h"
#endif //(defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)

#include <app/persistence/AttributePersistenceProviderInstance.h>
#include <app/persistence/DefaultAttributePersistenceProvider.h>
#include <app/persistence/DeferredAttributePersistenceProvider.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/on-off-server/on-off-server.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include <assert.h>

#include <platform/silabs/platformAbstraction/SilabsPlatform.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#include <lib/support/CodeUtils.h>

#include <lib/support/Span.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/silabs/tracing/SilabsTracingMacros.h>



// ==================== ⏱️ 按键参数自定义配置区 ====================
#define BTN_SCAN_PERIOD_MS    50   // 定时器基准周期 (单位：毫秒)
#define LONG_PRESS_TIME_MS    1000 // 判定为“长按开始”所需要的时间 (单位：毫秒)
#define DOUBLE_CLICK_GAP_MS   350  // 允许判定为双击的最大时间间隔 (单位：毫秒)
#define LONG_PRESS_PULSE_MS   200  // 长按期间，每隔多少毫秒发送一次计数邮件 (单位：毫秒)
// =============================================================

// 根据毫秒数自动换算成状态机需要的计数次数 (Ticks)
#define TICKS_FOR_LONG_START  (LONG_PRESS_TIME_MS / BTN_SCAN_PERIOD_MS)
#define TICKS_FOR_PULSE       (LONG_PRESS_PULSE_MS / BTN_SCAN_PERIOD_MS)

// 自定义按键状态机的内部状态
typedef enum {
    MY_BTN_STATE_IDLE = 0,       // 空闲状态
    MY_BTN_STATE_WAIT_RELEASE,   // 按键按下，等待松开（用来判定和计算长按）
    MY_BTN_STATE_WAIT_DOUBLE     // 按键松开，等待第二下（用来判定双击或超时单击）
} MyButtonState_t;

// 状态机私有全局静态变量
static MyButtonState_t s_my_state = MY_BTN_STATE_IDLE;
static uint8_t s_my_last_button   = 0;
static osTimerId_t s_my_timer_id  = nullptr;
static uint32_t s_my_long_cnt     = 0;

// 声明私有定时器回调函数
static void MyButtonTimerCallback(void *argument);

/**
 * @brief 🎯 自定义硬件中断回调函数
 * @note  当硬件引脚电平变化时由芯片底座的 GPIO IRQ 中断直接触发
 */
void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction)
{
    // 将原厂定义的 btnAction 转换为布尔值：1 为按下，0 为松开
    uint8_t is_pressed = (btnAction == 1); 
    s_my_last_button   = button;

    // 提前准备好一封我们自定义格式的空白信件
    AppButtonEvent btn_event;
    btn_event.Type                  = AppButtonEvent::kEventType_Button;
    btn_event.ButtonEvent.ButtonIdx = button;
    btn_event.Handler               = MyButtonActionHandler; // 强行指定收信人

    if (is_pressed) // ───> 🟢 芯片硬件中断：检测到物理按键被【按下】
    {
        if (s_my_state == MY_BTN_STATE_IDLE)
        {
            s_my_state    = MY_BTN_STATE_WAIT_RELEASE;
            s_my_long_cnt = 0;
            // 启动定时器，以基准周期（如50ms）不断唤醒，辅助检测长按
            osTimerStart(s_my_timer_id, BTN_SCAN_PERIOD_MS); 
        }
        else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE)
        {
            // 🎯 在双击等待窗口期内按键再次按下 ──> 100% 确诊双击！
            osTimerStop(s_my_timer_id);
            s_my_state = MY_BTN_STATE_IDLE;

            btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_DoublePress;
            
            // ⚡【核心类型强转】转换为原厂指针安全发送
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); 
        }
    }
    else // ───> 🔴 芯片硬件中断：检测到物理按键被【松开】
    {
        if (s_my_state == MY_BTN_STATE_WAIT_RELEASE)
        {
            osTimerStop(s_my_timer_id);

            // 如果松开时，定时器跑过的次数还不到长按阈值，说明这只是个短按
            if (s_my_long_cnt < TICKS_FOR_LONG_START) 
            {
                s_my_state = MY_BTN_STATE_WAIT_DOUBLE;
                // 开启软件倒计时，在指定的双击允许间隔内等待主人按第二下
                osTimerStart(s_my_timer_id, DOUBLE_CLICK_GAP_MS); 
            }
            else
            {
                // 如果是从长按状态中松开，则直接回到空闲
                s_my_state = MY_BTN_STATE_IDLE;
            }
        }
    }
}

/**
 * @brief ⏰ 辅助软件定时器回调函数
 * @note  负责处理长按过程中的时钟递增，以及双击等待超时导致的“单击确诊”
 */
static void MyButtonTimerCallback(void *argument)
{
    AppButtonEvent btn_event;
    btn_event.Type                  = AppButtonEvent::kEventType_Button;
    btn_event.ButtonEvent.ButtonIdx = s_my_last_button;
    btn_event.Handler               = MyButtonActionHandler; 

    if (s_my_state == MY_BTN_STATE_WAIT_RELEASE) // 【场景 A：按键一直被按着没松开】
    {
        s_my_long_cnt++;

        // 1. 刚好达到了长按判定的时间点
        if (s_my_long_cnt == TICKS_FOR_LONG_START) 
        {
            btn_event.ButtonEvent.Action         = AppButtonEvent::kButtonAction_LongPressStart;
            btn_event.ButtonEvent.LongPressCount = 1;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); // ⚡强转寄信
        }
        // 2. 已经处于长按中，后续每隔固定的步进时间发送一次计数脉冲邮件
        else if (s_my_long_cnt > TICKS_FOR_LONG_START && 
                ((s_my_long_cnt - TICKS_FOR_LONG_START) % TICKS_FOR_PULSE == 0)) 
        {
            btn_event.ButtonEvent.Action         = AppButtonEvent::kButtonAction_LongPressing;
            btn_event.ButtonEvent.LongPressCount = ((s_my_long_cnt - TICKS_FOR_LONG_START) / TICKS_FOR_PULSE) + 1;
            AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); // ⚡强转寄信
        }
    }
    else if (s_my_state == MY_BTN_STATE_WAIT_DOUBLE) // 【场景 B：松开后等了很久，第二下没来】
    {
        // ⏳ 说明在设定的时间内主人没有按下第二下 ──> 确诊为单击
        osTimerStop(s_my_timer_id);
        s_my_state = MY_BTN_STATE_IDLE;

        btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_ShortPress;
        AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event)); // ⚡强转寄信
    }
}


/**
 * @brief 🔌 初始化自定义按键服务的软件定时器
 */
void MyCustomButtonInit(void)
{
    // 创建一个周期性执行的 CMSIS-RTOS2 标准软件定时器，与系统内核时钟彻底绑定
    s_my_timer_id = osTimerNew(MyButtonTimerCallback, osTimerPeriodic, nullptr, nullptr);
    if (s_my_timer_id == nullptr)
    {
        SILABS_LOG("Failed to create my custom button software timer");
    }
}
