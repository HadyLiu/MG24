/**
 * @file FactoryResetManager.h
 * @brief 恢复出厂/软复位策略：5s 预警灯效、10s 清配网、取消与完成动画
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 长按 5s 起预警闪烁，满 10s 执行软复位(仅清配网，不重启)并播放完成动画；
 *       5~10s 间松开则取消。脉冲按 200ms 计数。
 */
#pragma once

#include "IndicatorService.h"
#include "LightEngine.h"
#include <stdint.h>

/** @brief 长按 10s 软复位与预警灯效管理 */
class FactoryResetManager
{
public:
  /** @brief 获取 FactoryResetManager 单例
   *  @return 单例引用 */
  static FactoryResetManager& instance();

  /**
   * @brief 长按脉冲计数回调：5s 预警 / 10s 软复位 + 完成动画
   * @param count 长按脉冲计数（每 200ms 递增）
   * @return 无
   */
  void onLongPressTick(uint16_t count);

  /**
   * @brief 长按释放回调：5~10s 之间松开则取消复位
   * @param savedCount 释放时保存的脉冲计数
   * @return 无
   */
  void onLongPressRelease(uint16_t savedCount);

  /** @brief 取消预警灯效与红灯闪烁
   *  @return 无 */
  void cancel();

private:
  FactoryResetManager() = default;

  /** @brief 5s 预警：主灯按当前状态闪 + 红灯混合闪，持续至 10s
   *  @return 无 */
  void startWarningEffects();

  /** @brief 10s 触发：仅清 Matter/Thread 配网，不重启
   *  @return 无 */
  void executeSoftReset();

  /** @brief 10s 完成动画：65% 快闪 2 次 -> 灭 2s -> 淡入默认
   *  @return 无 */
  void startResetCompleteEffects();

  /** @brief 完成动画收尾回调：恢复默认状态并淡入
   *  @return 无 */
  static void onResetCompleteCallback();
};
