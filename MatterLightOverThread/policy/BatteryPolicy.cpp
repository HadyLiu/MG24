/**
 * @file BatteryPolicy.cpp
 * @brief 电池领域策略实现：6.9V 低电、6.0V 临界动画、充电 60% 白呼吸
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 临界态执行淡出->淡入->淡出动画后锁灯并置保护，禁止再开灯。
 */
#include "BatteryPolicy.h"
#include "AppConfig.h"
#include "app/AppLog.h"
#include "app/PowerConfig.h"

/** @brief 获取 BatteryPolicy 单例 */
BatteryPolicy& BatteryPolicy::instance()
{
  static BatteryPolicy s_policy;
  return s_policy;
}

/** @brief 是否处于低电量警告（≤6.9V） */
bool BatteryPolicy::isLowBattery() const
{
  return PowerMonitor::instance().batteryStatusEnum() <= Bat_LowVolWarn;
}

/** @brief 是否处于临界保护（≤6.0V） */
bool BatteryPolicy::isCriticalBattery() const
{
  return PowerMonitor::instance().batteryStatusEnum() == Bat_LowVolProt;
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

/**
 * @brief 临界态尝试开灯动画：点亮(淡入) -> 淡出 -> 淡入 -> 淡出后锁灯
 * @note 每段 400ms 真实线性淡变；末段淡出到 0 后强制锁灯并置保护。
 */
void BatteryPolicy::runCriticalShutdownAnimation()
{
  LOG_BAT("[BatteryPolicy] critical animation (lightup->out->in->out)");
  LightEngine& le = LightEngine::instance();
  WrgbColor raw   = le.rawColor();
  uint8_t b       = le.brightness() ? le.brightness() : LED_BRIGHTNESS_MAX;

  MixedEffectStep steps[4] = {
      {EffectModeFade, true, b, raw, 400, 1},
      {EffectModeFade, false, b, raw, 400, 1},
      {EffectModeFade, true, b, raw, 400, 1},
      {EffectModeFade, false, b, raw, 400, 1},
  };

  le.runMixedSequence(steps, 4, []() {
    LOG_BAT("[BatteryPolicy] critical animation done, lock light");
    LightEngine::instance().updateNormalState(
        false, 0, LightEngine::instance().rawColor());
    LightEngine::instance().setLowBatteryProtection(true);
    PowerMonitor::instance().setPowerProtect(true);
  });
}

/** @brief 低电/临界态下用户尝试开灯：红闪 + 临界动画 */
void BatteryPolicy::onTryTurnOn()
{
  if (isLowBattery())
  {
    LOG_BAT("[BatteryPolicy] low battery warn, red blink x2");
    IndicatorService::instance().startRedBlinkNormal(400, 2);
  }

  if (isCriticalBattery() || isPowerProtected())
  {
    IndicatorService::instance().startRedBlinkNormal(400, 2);
    LightEngine::instance().updateNormalState(
        true, LightEngine::instance().brightness(),
        LightEngine::instance().rawColor());
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

  LOG_BAT("[BatteryPolicy] charge indicator status=%u", chargeStatus);

  if (chargeStatus == Bat_InCharge)
  {
    IndicatorService::instance().startWhiteBreath(60);
    IndicatorService::instance().stopRedBlink();
    return;
  }

  IndicatorService::instance().stopWhiteBreath();

  if (chargeStatus == Bat_Nobat || chargeStatus == Bat_HighTemp ||
      chargeStatus == Bat_ItError)
    IndicatorService::instance().startRedBlinkNormal(400, 0);
}

/** @brief 电源切换时清除保护与指示状态 */
void BatteryPolicy::onPowerSwitch()
{
  LOG_BAT("[BatteryPolicy] power switch, clear protect");
  PowerMonitor::instance().setPowerProtect(false);
  IndicatorService::instance().stopWhiteBreath();
  IndicatorService::instance().stopRedBlink();
}

/** @brief 10ms tick：电池采样或充电逻辑 + 保护联动 */
void BatteryPolicy::tick10ms(uint32_t tickCount)
{
  PowerMonitor& pm = PowerMonitor::instance();
  pm.refreshUpstreamPower();
  pm.powerSwitchAssignment();

  if (!pm.powerStatus())
  {
    pm.tickBatteryIfDue(tickCount);
    if (pm.batteryStatusEnum() == Bat_LowVolProt)
      pm.setPowerProtect(true);
    pm.batOutEnable();
    LightEngine::instance().setLowBatteryProtection(pm.isPowerProtect());

    if ((tickCount % 200U) == 0U)
    {
      LOG_BAT("[BatteryPolicy] bat mode status=%u vol=%lumV protect=%d",
              pm.batteryStatus(), pm.batteryVoltageMv(), pm.isPowerProtect());
    }
  }
  else
  {
    pm.tickCharge(tickCount);
    pm.batOutDisable();
    pm.chargeCurrentCtrl(LightEngine::instance().isOn());
    updateChargeIndicator(pm.batteryStatus());
  }
}
