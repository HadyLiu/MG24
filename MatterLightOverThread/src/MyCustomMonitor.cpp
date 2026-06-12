#include "cmsis_os2.h"
#include "sl_power_manager.h"
#include "lib/support/logging/CHIPLogging.h"
#include "../app/AppOrchestrator.h"
#include "app/WakeControl.h"

#define MONITOR_TASK_STACK_SIZE 2048
static osThreadId_t s_MonitorTaskHandle = nullptr;
#define INTERNAL_WAKEUP_FLAG 0x00000001U
#define KEY_WAKEUP_FLAG      0x00000002U
#define NETWORK_WAKEUP_FLAG  0x00000004U

static bool normalAndLow = true;
static bool ConditionalWake_upState = true;

//
//  在按键事件回调中调用这个函数来触发唤醒
//  在识别中唤醒
//  数据下发时唤醒
//  在按键事件回调中调用这个函数来触发唤醒
//  在识别中唤醒
//  数据下发时唤醒
void ConditionalWake_up(void)
{
    ConditionalWake_upState = true;

    // 只在低频模式下才触发唤醒，正常模式不受影响
    if (normalAndLow == false)
    {
        if (s_MonitorTaskHandle != nullptr)
        {
            osThreadFlagsSet(s_MonitorTaskHandle, INTERNAL_WAKEUP_FLAG);
        }
    }
}

// 独立的监控状态机任务
static void CustomMonitorTask_Handler(void *argument)
{
    (void)argument;
    AppOrchestrator::instance().init();
    ChipLogProgress(NotSpecified, "Custom Monitor Task started.");

    while (true)
    {
        normalAndLow = AppOrchestrator::instance().tick10ms(&ConditionalWake_upState);
        if (normalAndLow) // 如果返回 true，说明需要进入高频模式
        {
            // 正常模式
            osDelay(10);
        }
        else
        {
            // 低频模式
            // osDelay(400);
            // 低频模式：最多 400ms 执行一次；如果有唤醒事件则立即返回
            uint32_t flags = osThreadFlagsWait(INTERNAL_WAKEUP_FLAG, osFlagsWaitAny, 400);

            if ((flags & INTERNAL_WAKEUP_FLAG) != 0U)
            {
                // 被事件唤醒后，下一轮 entry_Loop 会看到 ConditionalWake_upState = true
            }
        }
    }
}

// 创建独立监控任务的接口
void CreateCustomMonitorTask(void)
{
    const osThreadAttr_t task_attr = {.name = "MyMonitor",
                                      .attr_bits = 0,
                                      .cb_mem = nullptr,
                                      .cb_size = 0,
                                      .stack_mem = nullptr,
                                      .stack_size = MONITOR_TASK_STACK_SIZE,
                                      // 🎯 优先级必须设为低（osPriorityBelowNormal 或 osPriorityBelowNormal1），
                                      // 绝对不能抢占 Matter 核心协议栈（CHIP/OpenThread）的时间片，防止手机控制卡顿
                                      .priority = osPriorityRealtime, // osPriorityBelowNormal,
                                      //.priority = osPriorityBelowNormal,
                                      .tz_module = 0,
                                      .reserved = 0};

    s_MonitorTaskHandle = osThreadNew(CustomMonitorTask_Handler, nullptr, &task_attr);

    if (s_MonitorTaskHandle != nullptr)
    {
        ChipLogProgress(NotSpecified, "Custom Monitor Task created successfully.");
    }
    else
    {
        ChipLogError(NotSpecified, "Failed to create Custom Monitor Task!");
    }
}
