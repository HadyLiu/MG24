/**
 * @file BatteryPolicy.cpp
 * @brief 电池领域策略实现：6.9V 低电、6.0V 临界动画、充电 60% 白呼吸
 * @layer Policy
 */
#include "BatteryPolicy.h"
#include "../driver/powerManage.h"
#include "AppConfig.h"

extern bool g_PowerProtect;

/** @brief 获取 BatteryPolicy 单例 */
BatteryPolicy &BatteryPolicy::instance()
{
    static BatteryPolicy s_policy;
    return s_policy;
}

/** @brief 是否处于低电量警告（≤6.9V） */
bool BatteryPolicy::isLowBattery() const
{
    return eg_BatStatus <= Bat_LowVolWarn;
}

/** @brief 是否处于临界保护（≤6.0V） */
bool BatteryPolicy::isCriticalBattery() const
{
    return eg_BatStatus == Bat_LowVolProt;
}

/** @brief 是否已触发低电锁灯保护 */
bool BatteryPolicy::isPowerProtected() const
{
    return PowerMonitor::instance().isPowerProtect();
}

/** @brief 是否允许用户开灯/调光 */
bool BatteryPolicy::allowUserLightControl() const
{
    return !PowerMonitor::instance().isPowerProtect();
}

/** @brief 临界态：淡出→淡入→淡出后锁灯 */
void BatteryPolicy::runCriticalShutdownAnimation()
{
    SILABS_LOG("[BatteryPolicy] critical shutdown animation start");
    WrgbColor raw = LightEngine::instance().rawColor();
    MixedEffectStep steps[3] = {
        {EffectModeHold, true, LightEngine::instance().brightness(), raw, 400, 1},
        {EffectModeHold, true, LightEngine::instance().brightness(), raw, 400, 1},
        {EffectModeHold, false, 0, raw, 400, 1},
    };

    LightEngine::instance().runMixedSequence(steps, 3, []() {
        SILABS_LOG("[BatteryPolicy] critical animation done, lock light");
        LightEngine::instance().updateNormalState(false, 0, LightEngine::instance().rawColor());
        LightEngine::instance().setLowBatteryProtection(true);
        PowerMonitor::instance().setPowerProtect(true);
    });
}

/** @brief 低电/临界态下用户尝试开灯：红闪 + 临界动画 */
void BatteryPolicy::onTryTurnOn()
{
    if (isLowBattery())
    {
        SILABS_LOG("[BatteryPolicy] low battery warn, red blink x2");
        IndicatorService::instance().startRedBlinkNormal(400, 2);
    }

    if (isCriticalBattery() || isPowerProtected())
    {
        IndicatorService::instance().startRedBlinkNormal(400, 2);
        LightEngine::instance().updateNormalState(true, LightEngine::instance().brightness(), LightEngine::instance().rawColor());
        LightEngine::instance().startFadeToCurrent(400, FadePolicyFixedMs);
        runCriticalShutdownAnimation();
    }
}

/** @brief 根据充电状态更新白呼吸/红闪指示 */
void BatteryPolicy::updateChargeIndicator(uint8_t chargeStatus)
{
    static uint8_t last = 0xFF;
    if (chargeStatus == last)
        return;
    last = chargeStatus;

    SILABS_LOG("[BatteryPolicy] charge indicator status=%u", chargeStatus);

    if (chargeStatus == Bat_InCharge)
    {
        IndicatorService::instance().startWhiteBreath(60);
        IndicatorService::instance().stopRedBlink();
        return;
    }

    IndicatorService::instance().stopWhiteBreath();

    if (chargeStatus == Bat_Nobat || chargeStatus == Bat_HighTemp || chargeStatus == Bat_ItError)
        IndicatorService::instance().startRedBlinkNormal(400, 0);
}

/** @brief 电源切换时清除保护与指示状态 */
void BatteryPolicy::onPowerSwitch()
{
    SILABS_LOG("[BatteryPolicy] power switch, clear protect");
    PowerMonitor::instance().setPowerProtect(false);
    IndicatorService::instance().stopWhiteBreath();
    IndicatorService::instance().stopRedBlink();
}

/** @brief 10ms tick：电池采样或充电逻辑 + 保护联动 */
void BatteryPolicy::tick10ms(uint32_t tickCount)
{
    GetExternPowerFlag();
    PowerSwitchAssignment();

    PowerMonitor &pm = PowerMonitor::instance();

    if (!pm.powerStatus())
    {
        pm.tickBatteryIfDue(tickCount);
        if (eg_BatStatus == Bat_LowVolProt)
            pm.setPowerProtect(true);
        g_PowerProtect = pm.isPowerProtect();
        BatOutEn();
        LightEngine::instance().setLowBatteryProtection(pm.isPowerProtect());

        // 约 2s 打印一次电池调试信息
        if ((tickCount % 200U) == 0U)
        {
            SILABS_LOG("[BatteryPolicy] bat mode status=%u vol=%lumV protect=%d", eg_BatStatus, pm.batteryVoltageMv(),
                       pm.isPowerProtect());
        }
    }
    else
    {
        pm.tickCharge(tickCount);
        BatOutDis();
        ChargeCurrentCtrlOut(LightEngine::instance().isOn());
        updateChargeIndicator(pm.batteryStatus());
    }
}
