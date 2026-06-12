/**
 * @file IndicatorService.h
 * @brief 指示灯服务：红灯闪烁与白呼吸的统一 API
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 红灯用于告警/配网/充电故障，白呼吸用于充电与首次配网；与主灯互不影响。
 */
#pragma once

#include "../hal/IndicatorHal.h"
#include <stdint.h>

/** @brief 系统指示灯（红/白）特效服务 */
class IndicatorService
{
public:
  /** @brief 获取 IndicatorService 单例
   *  @return 单例引用 */
  static IndicatorService& instance();

  /** @brief 10ms 轮询：驱动白呼吸与红灯闪烁
   *  @return 无 */
  void tick10ms();

  /** @brief 白呼吸 10ms tick
   *  @return 无 */
  void pollWhiteBreath();

  /** @brief 红灯闪烁调度与 10ms tick
   *  @return 无 */
  void dispatchRedBlink();

  /**
   * @brief 启动单速红灯闪烁
   * @param periodMs 闪烁周期（ms）
   * @param count    闪烁次数，0 表示持续闪烁
   * @return 无
   */
  void startRedBlinkNormal(uint16_t periodMs, uint16_t count);

  /**
   * @brief 启动双速混合红灯闪烁（用于恢复出厂预警）
   * @param m1Period 第一段周期（ms）
   * @param m1Count  第一段次数
   * @param m2Period 第二段周期（ms）
   * @param m2Count  第二段次数
   * @return 无
   */
  void startRedBlinkMixed(uint16_t m1Period, uint16_t m1Count,
                          uint16_t m2Period, uint16_t m2Count);

  /** @brief 停止所有红灯闪烁
   *  @return 无 */
  void stopRedBlink();

  /**
   * @brief 启动白呼吸
   * @param brightnessPercent 呼吸峰值亮度百分比（0~100）
   * @return 无
   */
  void startWhiteBreath(uint8_t brightnessPercent);

  /** @brief 停止白呼吸
   *  @return 无 */
  void stopWhiteBreath();
};
