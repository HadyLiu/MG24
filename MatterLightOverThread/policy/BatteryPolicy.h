/**
 * @file BatteryPolicy.h
 * @brief 电池领域策略：低电/临界保护、充电指示、临界关机动画
 * @layer Policy
 */
#pragma once

#include "IndicatorService.h"
#include "LightEngine.h"
#include "PowerMonitor.h"
#include <stdint.h>

/** @brief 电池与充电业务策略，编排 PowerMonitor 与指示灯/灯光 */
class BatteryPolicy
{
public:
    static BatteryPolicy &instance();

    void tick10ms(uint32_t tickCount);
    void onPowerSwitch();
    void onTryTurnOn();
    void updateChargeIndicator(uint8_t chargeStatus);

    bool isLowBattery() const;
    bool isCriticalBattery() const;
    bool isPowerProtected() const;
    bool allowUserLightControl() const;

private:
    BatteryPolicy() = default;
    void runCriticalShutdownAnimation();
};
