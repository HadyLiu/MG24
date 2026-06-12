/**
 * @file BatteryPolicy.h
 * @brief 电池领域策略：低电/临界保护、充电指示、临界关机动画
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 低电阈值 6.9V，临界阈值 6.0V；临界态执行淡变动画后锁灯，禁止再开灯。
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
  /** @brief 获取 BatteryPolicy 单例
   *  @return 单例引用 */
  static BatteryPolicy& instance();

  /**
   * @brief 10ms tick：电池采样或充电逻辑 + 保护联动
   * @param tickCount 全局 10ms tick 计数
   * @return 无
   */
  void tick10ms(uint32_t tickCount);

  /** @brief 电源切换时清除保护与指示状态
   *  @return 无 */
  void onPowerSwitch();

  /** @brief 低电/临界态下用户尝试开灯：红闪 + 临界动画
   *  @return 无 */
  void onTryTurnOn();

  /**
   * @brief 根据充电状态更新白呼吸/红闪指示
   * @param chargeStatus 充电状态枚举（Bat_InCharge/Bat_Nobat 等）
   * @return 无
   */
  void updateChargeIndicator(uint8_t chargeStatus);

  /** @brief 是否处于低电量警告（<=6.9V）
   *  @return true 低电 */
  bool isLowBattery() const;

  /** @brief 是否处于临界保护（<=6.0V）
   *  @return true 临界 */
  bool isCriticalBattery() const;

  /** @brief 是否已触发低电锁灯保护
   *  @return true 已锁灯 */
  bool isPowerProtected() const;

  /** @brief 是否允许用户开灯/调光
   *  @return true 允许 */
  bool allowUserLightControl() const;

private:
  BatteryPolicy() = default;

  /** @brief 临界态尝试开灯动画：点亮->淡出->淡入->淡出后锁灯
   *  @return 无 */
  void runCriticalShutdownAnimation();
};
