/**
 * @file PowerMonitor.cpp
 * @brief 电源监控服务实现
 * @layer Service
 */
#include "PowerMonitor.h"
#include "AppConfig.h"

extern unsigned int g_ADBatLowVal;

/** @brief 获取 PowerMonitor 单例 */
PowerMonitor &PowerMonitor::instance()
{
    static PowerMonitor s_mon;
    return s_mon;
}

/** @brief 初始化 ADC 通道 */
void PowerMonitor::adcInit()
{
    powerManage_adc_Init();
}

/** @brief 反初始化 ADC，进入低功耗前调用 */
void PowerMonitor::adcDeInit()
{
    powerManage_adc_DeInit();
}

/** @brief 电源管理模块初始化 */
void PowerMonitor::init()
{
    SILABS_LOG("[PowerMonitor] init");
    PowerManageInit();
}

bool PowerMonitor::powerStatus() const { return eg_PowerStatus; }
bool PowerMonitor::upstreamPowerStatus() const { return eg_UpPowerStatus; }
uint8_t PowerMonitor::batteryStatus() const { return static_cast<uint8_t>(eg_BatStatus); }
uint32_t PowerMonitor::batteryVoltageMv() const { return g_ADBatLowVal * 3; }
bool PowerMonitor::isPowerProtect() const { return m_powerProtect; }

/** @brief 设置低电保护标志 */
void PowerMonitor::setPowerProtect(bool v)
{
    if (m_powerProtect != v)
        SILABS_LOG("[PowerMonitor] power protect=%d", v);
    m_powerProtect = v;
}

/** @brief 电池模式下周期性采样（默认 1s） */
void PowerMonitor::tickBatteryIfDue(uint32_t tick10ms, uint32_t interval10ms)
{
    static uint32_t last = 0;
    if (tick10ms - last >= interval10ms)
    {
        last = tick10ms;
        GetDisChargeStatus();
        if (eg_BatStatus == Bat_LowVolProt)
            m_powerProtect = true;
    }
}

/** @brief 外部供电模式下充电逻辑 tick */
void PowerMonitor::tickCharge(uint32_t tick10ms)
{
    (void)tick10ms;
    ChargeTimeUpdata();
    ChargeLogic(ChargeDetect());
}
