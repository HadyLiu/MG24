/**
 * @file FactoryResetManager.cpp
 * @brief 恢复出厂/软复位策略实现
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 5s 预警、10s 软复位(仅清配网不重启)+完成动画；5~10s 松开取消。
 */
#include "FactoryResetManager.h"
#include "../integration/MatterBridge.h"
#include "AppConfig.h"
#include "ColorLibrary.h"
#include "app/AppLog.h"

static constexpr uint8_t kLongPressWarningCount = 25; ///< 5s（200ms 脉冲 × 25）
static constexpr uint8_t kLongPressResetCount = 50; ///< 10s

/** @brief 获取 FactoryResetManager 单例 */
FactoryResetManager& FactoryResetManager::instance()
{
  static FactoryResetManager s_mgr;
  return s_mgr;
}

/**
 * @brief 5s 预警：主灯按当前状态闪烁 + 红灯混合闪烁，持续至 10s
 * @note 仅预警，不含完成动画；末段为 1 次慢闪。若用户保持长按至 10s，
 *       由 onLongPressTick 切换到完成动画；若中途松开则由 cancel() 终止。
 *       闪烁序列时长(3.2s 快闪 + 2.4s 慢闪 = 5.6s)略长于 5s 窗口，
 *       避免到 10s 前提前结束产生空档。
 */
void FactoryResetManager::startWarningEffects()
{
  LOG_FACTORY("[FactoryReset] 5s warning start (blink current state)");
  WrgbColor raw = LightEngine::instance().rawColor();
  uint8_t b     = LightEngine::instance().brightness();
  if (b == 0)
    b = LED_BRIGHTNESS_MAX; // 关灯时给可见亮度以提示

  MixedEffectStep steps[2] = {
      {EffectModeBlink, true, b, raw, 800, 4},  // 快闪 ~3.2s
      {EffectModeBlink, true, b, raw, 2400, 1}, // 末段 1 次慢闪
  };

  LightEngine::instance().runMixedSequence(steps, 2, nullptr);
  IndicatorService::instance().startRedBlinkMixed(800, 4, 2400, 1);
}

/**
 * @brief 10s 完成动画：65% 快闪 2 次 -> 熄灭 2s -> 800ms 淡入默认 #7@100%
 */
void FactoryResetManager::startResetCompleteEffects()
{
  LOG_FACTORY("[FactoryReset] reset complete effects: 65%% blink x2 -> off 2s "
              "-> fade default");
  IndicatorService::instance().stopRedBlink();

  WrgbColor def = ColorLibrary::instance().colorAtCycleIndex(
      ColorLibrary::kDefaultCycleIndex);
  uint8_t b65 =
      static_cast<uint8_t>((LED_BRIGHTNESS_MAX * 65 + 50) / 100); // ≈166

  MixedEffectStep steps[2] = {
      {EffectModeBlink, true, b65, def, 400, 2}, // 65% 快闪 2 次
      {EffectModeHold, false, 0, def, 2000, 1},  // 熄灭 2s
  };

  LightEngine::instance().runMixedSequence(steps, 2, onResetCompleteCallback);
}

/** @brief 完成动画收尾：恢复默认状态(#7,100%) 并 800ms 淡入 */
void FactoryResetManager::onResetCompleteCallback()
{
  LOG_FACTORY("[FactoryReset] fade in to default #7 @100%%");
  LightEngine& le = LightEngine::instance();
  WrgbColor def   = ColorLibrary::instance().colorAtCycleIndex(
      ColorLibrary::kDefaultCycleIndex);
  le.updateNormalState(true, LED_BRIGHTNESS_MAX, def);
  le.setColorFromCycleIndex(ColorLibrary::kDefaultCycleIndex, 800);
  le.setChangeOrigin(ChangeOriginLocalKey);
  le.saveStateToFlash();
}

/** @brief 10s 触发：仅清 Matter/Thread 配网，不重启 */
void FactoryResetManager::executeSoftReset()
{
  LOG_FACTORY("[FactoryReset] 10s reached, trigger soft network reset");
  MatterBridge::instance().triggerSoftNetworkReset();
}

/** @brief 长按脉冲计数：5s 预警 / 10s 软复位 + 完成动画 */
void FactoryResetManager::onLongPressTick(uint16_t count)
{
  if (count == kLongPressWarningCount)
    startWarningEffects();
  if (count == kLongPressResetCount)
  {
    executeSoftReset();
    startResetCompleteEffects();
  }
}

/** @brief 5~10s 之间松开：取消复位流程 */
void FactoryResetManager::onLongPressRelease(uint16_t savedCount)
{
  if (savedCount >= kLongPressWarningCount && savedCount < kLongPressResetCount)
    cancel();
}

/** @brief 取消预警灯效与红灯闪烁 */
void FactoryResetManager::cancel()
{
  LOG_FACTORY("[FactoryReset] cancelled before 10s");
  LightEngine::instance().stopMixedEffects();
  IndicatorService::instance().stopRedBlink();
}
