/**
 * @file PowerMonitor.h
 * @brief 电源监控服务：ADC/电池状态/充电 tick 封装
 * @layer Service
 */
#pragma once

#include "../driver/powerManage.h"
#include <stdint.h>

/** @brief 封装 powerManage 驱动的电源与电池采样服务 */
class PowerMonitor
{
public:
    static PowerMonitor &instance();

    void adcInit();
    void adcDeInit();
    void init();
    void tickBatteryIfDue(uint32_t tick10ms, uint32_t interval10ms = 100);
    void tickCharge(uint32_t tick10ms);

    bool     powerStatus() const;
    bool     upstreamPowerStatus() const;
    uint8_t  batteryStatus() const;
    uint32_t batteryVoltageMv() const;
    bool     isPowerProtect() const;
    void     setPowerProtect(bool v);

private:
    bool m_powerProtect = false;
};
