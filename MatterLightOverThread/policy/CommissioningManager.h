/**
 * @file CommissioningManager.h
 * @brief Matter 配网策略：自动/手动进入、窗口计时、首次白呼吸、配对成功动画
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 窗口时长由 COMMISSIONING_WINDOW_MS 控制（原型 15s，量产 15min）；
 *       仅开箱首次配网展示白光呼吸，后续配网无视觉反馈。
 */
#pragma once

#include "LightEngine.h"
#include <stdint.h>

#ifndef COMMISSIONING_WINDOW_MS
#define COMMISSIONING_WINDOW_MS (15U * 1000U) ///< 原型 15s；量产改为 15*60*1000
#endif

/** @brief Matter 配网窗口与首次配网视觉反馈管理 */
class CommissioningManager
{
public:
  /** @brief 获取 CommissioningManager 单例
   *  @return 单例引用 */
  static CommissioningManager& instance();

  /**
   * @brief 配网窗口超时检测（每 10ms 调用）
   * @param elapsedMs 预留参数（当前内部按固定 10ms 累加）
   * @return 无
   */
  void tick10ms(uint32_t elapsedMs);

  /** @brief 未入网且灯开：自动进入配网
   *  @return 无 */
  void onLightTurnedOn();

  /** @brief 手动触发配网（短按/系统键）：未入网则进入或重启倒计时
   *  @return 无 */
  void onManualTrigger();

  /** @brief 配对成功：60% 快闪 2 次确认并淡入全亮，标记首次配网完成
   *  @return 无 */
  void onCommissioningComplete();

  /** @brief 停止首次配网白光呼吸
   *  @return 无 */
  void stopFirstBreath();

  /** @brief 配网窗口是否处于激活态
   *  @return true 激活中 */
  bool isWindowActive() const
  {
    return m_windowActive;
  }

private:
  CommissioningManager() = default;

  /** @brief 进入配网窗口；首次配网时主灯白光呼吸
   *  @return 无 */
  void enterCommissioningWindow();

  /** @brief 已在配网中时重启倒计时
   *  @return 无 */
  void restartWindowTimer();

  bool m_windowActive      = false; ///< 配网窗口是否激活
  uint32_t m_windowStartMs = 0;     ///< 窗口已累计时长（ms）
  bool m_firstBreathActive = false; ///< 首次配网白呼吸是否进行中
};
