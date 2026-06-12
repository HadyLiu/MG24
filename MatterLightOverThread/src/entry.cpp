/**
 * @file entry.cpp
 * @brief C 语言桥接层：供 MyCustomMonitor 调用的 entry_Init / entry_Loop 入口
 * @layer App (legacy bridge)
 */
#include "AppOrchestrator.h"
#include "../integration/MatterBridge.h"
#include "../service/PowerMonitor.h"
#include "../driver/powerManage.h"
#include "AppConfig.h"

/** @brief 兼容旧代码：低电量保护全局标志，与 PowerMonitor 同步 */
bool g_PowerProtect = false;

/** @brief 监控任务启动时调用，委托 AppOrchestrator 初始化 */
void entry_Init(void)
{
    AppOrchestrator::instance().init();
}

/** @brief 10ms 循环体，委托 AppOrchestrator tick */
bool entry_Loop(bool *interruptWakeUp)
{
    g_PowerProtect = PowerMonitor::instance().isPowerProtect();
    return AppOrchestrator::instance().tick10ms(interruptWakeUp);
}

/** @brief 低功耗唤醒时清除超时并恢复 ADC */
void entry_clearTimeout(bool *normalAndLow)
{
    if (normalAndLow)
        *normalAndLow = true;
    PowerMonitor::instance().adcInit();
}

/** @brief 配对成功动画（已由 CommissioningManager 接管，保留符号兼容） */
void TriggerPairingSuccessAnimation(void)
{
    SILABS_LOG("[entry] TriggerPairingSuccessAnimation stub, handled by CommissioningManager");
}
