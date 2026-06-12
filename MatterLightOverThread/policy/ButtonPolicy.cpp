/**
 * @file ButtonPolicy.cpp
 * @brief 按键领域策略实现：100/50/关循环、网关单按关、双击换色、长按复位
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 编排 LightEngine/MatterBridge/配网/复位；短按淡变固定 400ms。
 */
#include "ButtonPolicy.h"
#include "AppConfig.h"
#include "app/AppLog.h"

/** @brief 获取 ButtonPolicy 单例 */
ButtonPolicy& ButtonPolicy::instance()
{
  static ButtonPolicy s_policy;
  return s_policy;
}

/** @brief 分发按键事件到对应处理函数 */
void ButtonPolicy::handle(const ButtonEvent& ev)
{
  switch (ev.type)
  {
  case ButtonEventType::ShortPress:
    LOG_BTN("[ButtonPolicy] short press btn=%u", ev.buttonIndex);
    onShortPress();
    break;
  case ButtonEventType::DoublePress:
    LOG_BTN("[ButtonPolicy] double press btn=%u", ev.buttonIndex);
    onDoublePress();
    break;
  case ButtonEventType::LongPressing:
    if (BatteryPolicy::instance().allowUserLightControl())
      FactoryResetManager::instance().onLongPressTick(ev.longPressCount);
    m_savedLongPressCount = ev.longPressCount;
    break;
  case ButtonEventType::LongPressRelease:
    LOG_BTN("[ButtonPolicy] long press release count=%u",
            m_savedLongPressCount);
    if (BatteryPolicy::instance().allowUserLightControl())
      FactoryResetManager::instance().onLongPressRelease(m_savedLongPressCount);
    break;
  default:
    break;
  }
}

/** @brief 短按：100% → 50% → 关；网关已开时单按关；同步 Matter */
void ButtonPolicy::onShortPress()
{
  LightEngine& le  = LightEngine::instance();
  MatterBridge& mb = MatterBridge::instance();

  CommissioningManager::instance().onManualTrigger();

  if (le.changeOrigin() == ChangeOriginMatterApp && le.isOn())
  {
    LOG_BTN("[ButtonPolicy] remote-on override: turn off");
    le.updateNormalState(false, 0, le.rawColor());
    le.setChangeOrigin(ChangeOriginLocalKey);
    le.saveStateToFlash();
    // 恒定 400ms 淡出，与亮度差无关
    le.startFadeToCurrent(LED_FADE_KEY_TOTAL_MS, FadePolicyFixedMs);
    mb.uploadOnOff(false);
    mb.uploadBrightnessPercent(0);
    return;
  }

  if (BatteryPolicy::instance().isLowBattery())
    BatteryPolicy::instance().onTryTurnOn();

  if (!BatteryPolicy::instance().allowUserLightControl())
  {
    LOG_BTN("[ButtonPolicy] short press blocked: power protect");
    return;
  }

  uint8_t prevBright = le.brightness();

  // 标准三态循环：关 -> 100% -> 50% -> 关。
  // APP 可能下发非 100/50
  // 的任意亮度，此处按中点阈值(75%≈191)归一到标准档后再前进，
  // 确保短按行为始终可预测并与 APP 开关状态联动。
  constexpr uint8_t kMidThreshold =
      static_cast<uint8_t>((LED_BRIGHTNESS_MAX * 3) / 4); // ≈191 (75%)

  if (!le.isOn() || le.brightness() == 0)
    le.updateNormalState(true, LED_BRIGHTNESS_MAX, le.rawColor()); // 关 -> 100%
  else if (le.brightness() > kMidThreshold)
    le.updateNormalState(true, LED_BRIGHTNESS_MAX >> 1,
                         le.rawColor()); // 近100% -> 50%
  else
    le.updateNormalState(false, 0, le.rawColor()); // ≤50% 档 -> 关

  (void)prevBright;
  le.setChangeOrigin(ChangeOriginLocalKey);
  le.saveStateToFlash();
  // 恒定 400ms 淡入/淡出，无论亮度差大小
  le.startFadeToCurrent(LED_FADE_KEY_TOTAL_MS, FadePolicyFixedMs);

  mb.uploadOnOff(le.isOn());
  mb.uploadBrightnessPercent(static_cast<uint8_t>(
      (static_cast<uint16_t>(le.brightness()) + 1) * 100 >> 8));

  LOG_BTN("[ButtonPolicy] cycle result on=%d bright=%u fade=%ums", le.isOn(),
          le.brightness(), LED_FADE_KEY_TOTAL_MS);

  if (le.isOn())
    CommissioningManager::instance().onLightTurnedOn();
}

/** @brief 双击：循环颜色库并上报 Matter 色温 */
void ButtonPolicy::onDoublePress()
{
  LightEngine& le = LightEngine::instance();
  if (!le.isOn())
  {
    LOG_BTN("[ButtonPolicy] double press ignored: light off");
    return;
  }

  if (BatteryPolicy::instance().isLowBattery())
    BatteryPolicy::instance().onTryTurnOn();
  if (!BatteryPolicy::instance().allowUserLightControl())
    return;

  uint8_t next  = ColorLibrary::instance().nextCycleIndex(le.colorCycleIndex());
  uint8_t libId = ColorLibrary::instance().libraryIdAtCycleIndex(next);
  LOG_BTN("[ButtonPolicy] color cycle idx=%u libId=%u", next, libId);

  le.setColorFromCycleIndex(next, LED_FADE_COLOR_SWITCH_MS);
  le.setChangeOrigin(ChangeOriginLocalKey);
  le.saveStateToFlash();

  MatterBridge::instance().uploadOnOff(le.isOn());
  MatterBridge::instance().uploadBrightnessPercent(static_cast<uint8_t>(
      (static_cast<uint16_t>(le.brightness()) + 1) * 100 >> 8));
  MatterBridge::instance().uploadColorFromCycleIndex(next);
}
