/**
 * @file IndicatorHal.h
 * @brief 红/白指示灯 HAL：红灯 GPIO 闪烁与白灯 PWM 呼吸
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 由 led_red_indic / led_white_indic 迁入；IndicatorService 委托调用。
 */
#pragma once

#include <stdint.h>

/** @brief 红/白指示灯 HAL */
class IndicatorHal
{
public:
  /**
   * @brief 启动单速红灯闪烁
   * @param periodMs 闪烁周期（ms）
   * @param count    闪烁次数，0 表示持续
   * @return 无
   */
  static void startRedBlinkNormal(uint16_t periodMs, uint16_t count);

  /**
   * @brief 启动双速混合红灯闪烁
   * @param m1Period 第一段周期（ms）
   * @param m1Count  第一段次数
   * @param m2Period 第二段周期（ms）
   * @param m2Count  第二段次数
   * @return 无
   */
  static void startRedBlinkMixed(uint16_t m1Period, uint16_t m1Count,
                                 uint16_t m2Period, uint16_t m2Count);

  /** @brief 停止红灯闪烁
   *  @return 无 */
  static void stopRedBlink();

  /** @brief 红灯闪烁调度与 10ms tick
   *  @return 无 */
  static void pollRedBlink10ms();

  /**
   * @brief 启动白呼吸
   * @param brightnessPercent 峰值亮度百分比（0~100）
   * @return 无
   */
  static void startWhiteBreath(uint8_t brightnessPercent);

  /** @brief 停止白呼吸
   *  @return 无 */
  static void stopWhiteBreath();

  /** @brief 白呼吸 10ms tick
   *  @return 无 */
  static void pollWhiteBreath10ms();
};
