/**
 * @file IdentifyEffect.h
 * @brief Matter Identify 灯效策略：识别期持续 400/400ms 闪烁，结束 800ms
 * 恢复原态
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 识别期按默认灯光状态(#7,100%)持续闪烁覆盖整个识别周期；识别期间不写
 * Flash。
 */
#pragma once

#include "LightEngine.h"

/** @brief 设备识别（Identify）期间的灯光特效与恢复 */
class IdentifyEffect
{
public:
  /** @brief 获取 IdentifyEffect 单例
   *  @return 单例引用 */
  static IdentifyEffect& instance();

  /** @brief Identify 开始：保存快照并按默认状态持续闪烁
   *  @return 无 */
  void onStart();

  /** @brief Identify 结束：停止闪烁并 800ms EaseOut 恢复原状态
   *  @return 无 */
  void onStop();

private:
  IdentifyEffect() = default;
  LightSnapshot m_saved{}; ///< 识别前的灯光状态快照
  bool m_active = false;   ///< 识别是否进行中
};
