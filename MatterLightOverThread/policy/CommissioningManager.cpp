/**
 * @file CommissioningManager.cpp
 * @brief Matter 配网策略实现
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 仅开箱首次配网展示白光呼吸；超时关闭窗口；配对成功 60% 闪 2
 * 次后淡入全亮。
 */
#include "CommissioningManager.h"
#include "../integration/MatterBridge.h"
#include "AppConfig.h"
#include "IndicatorService.h"
#include "app/AppLog.h"

/** @brief 获取 CommissioningManager 单例 */
CommissioningManager& CommissioningManager::instance()
{
  static CommissioningManager s_mgr;
  return s_mgr;
}

/** @brief 进入配网窗口；首次配网时主灯白光呼吸 */
void CommissioningManager::enterCommissioningWindow()
{
  LOG_COMMISSION("[Commissioning] enter window, timeout=%ums",
                 COMMISSIONING_WINDOW_MS);
  m_windowActive  = true;
  m_windowStartMs = 0;
  MatterBridge::instance().openCommissioningWindow();

  if (!LightEngine::instance().isFirstCommissionDone())
  {
    LOG_COMMISSION("[Commissioning] first-time white breath effect");
    WrgbColor white = {1023, 0, 0, 0};
    LightEngine::instance().setBreath(100, white, 0);
    m_firstBreathActive = true;
  }
}

/** @brief 已在配网中时，短按/系统键重启倒计时 */
void CommissioningManager::restartWindowTimer()
{
  LOG_COMMISSION("[Commissioning] restart window timer");
  m_windowStartMs = 0;
  MatterBridge::instance().openCommissioningWindow();
}

/** @brief 手动触发配网（短按/系统键） */
void CommissioningManager::onManualTrigger()
{
  if (MatterBridge::instance().isUnprovisioned())
  {
    if (m_windowActive)
      restartWindowTimer();
    else
      enterCommissioningWindow();
  }
}

/** @brief 未入网且灯开：自动进入配网 */
void CommissioningManager::onLightTurnedOn()
{
  if (MatterBridge::instance().isUnprovisioned() &&
      LightEngine::instance().isOn())
    enterCommissioningWindow();
}

/** @brief 停止首次配网白光呼吸 */
void CommissioningManager::stopFirstBreath()
{
  if (m_firstBreathActive)
  {
    LOG_COMMISSION("[Commissioning] stop first breath");
    LightEngine::instance().stopEffect();
    m_firstBreathActive = false;
  }
}

/** @brief 配对成功后：60% 快闪结束 -> 400ms 淡入恢复全亮(100%) */
static void onPairingBlinkDone()
{
  LOG_COMMISSION("[Commissioning] pairing blink done, fade in to full");
  LightEngine& le = LightEngine::instance();
  le.updateNormalState(true, LED_BRIGHTNESS_MAX, le.rawColor());
  le.startFadeToCurrent(LED_FADE_KEY_TOTAL_MS, FadePolicyFixedMs);
  le.saveStateToFlash();
}

/** @brief 配对成功：60% 快闪 2 次确认，再淡入到全亮，并标记首次配网完成 */
void CommissioningManager::onCommissioningComplete()
{
  LOG_COMMISSION("[Commissioning] complete, success blink x2 -> fade full");
  stopFirstBreath();
  m_windowActive = false;
  LightEngine::instance().setFirstCommissionDone();

  WrgbColor raw = LightEngine::instance().rawColor();
  // 60% 亮度 = 255 * 60% ≈ 153
  MixedEffectStep step = {EffectModeBlink, true, 153, raw, 400, 2};
  LightEngine::instance().runMixedSequence(&step, 1, onPairingBlinkDone);
}

/** @brief 配网窗口超时检测（每 10ms 调用） */
void CommissioningManager::tick10ms(uint32_t elapsedMs)
{
  (void)elapsedMs;
  if (!m_windowActive)
    return;

  m_windowStartMs += 10;
  if (m_windowStartMs >= COMMISSIONING_WINDOW_MS)
  {
    LOG_COMMISSION("[Commissioning] window timeout, close window");
    m_windowActive = false;
    stopFirstBreath();
    MatterBridge::instance().closeCommissioningWindow();
  }
}
