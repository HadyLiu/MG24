/**
 * @file IdentifyEffect.cpp
 * @brief Matter Identify 灯效策略实现
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 识别期按默认状态持续闪烁，结束 800ms EaseOut 恢复；识别期间不写 Flash。
 */
#include "IdentifyEffect.h"
#include "AppConfig.h"
#include "ColorLibrary.h"
#include "app/AppLog.h"

/** @brief 获取 IdentifyEffect 单例 */
IdentifyEffect& IdentifyEffect::instance()
{
  static IdentifyEffect s_effect;
  return s_effect;
}

/**
 * @brief Identify 开始：保存当前状态，按默认灯光状态(#7,100%)持续 400/400ms
 * 闪烁
 * @note count=0 表示持续闪烁，直到 onStop() 结束识别周期；识别期间不写 Flash。
 */
void IdentifyEffect::onStart()
{
  LOG_IDENTIFY("[Identify] start continuous blink (default state)");
  m_saved       = LightEngine::instance().captureSnapshot();
  m_active      = true;
  WrgbColor def = ColorLibrary::instance().colorAtCycleIndex(
      ColorLibrary::kDefaultCycleIndex);
  // 周期 800ms = 400ms 亮 / 400ms 灭；count=0 持续闪烁覆盖整个识别周期
  LightEngine::instance().setBlink(LED_BRIGHTNESS_MAX, def, 800, 0);
}

/** @brief Identify 结束：800ms EaseOut 恢复原状态 */
void IdentifyEffect::onStop()
{
  if (!m_active)
    return;
  LOG_IDENTIFY("[Identify] stop, restore snapshot");
  m_active = false;
  LightEngine::instance().stopEffect();
  LightEngine::instance().restoreSnapshot(m_saved, 800);
}
