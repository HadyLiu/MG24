/**
 * @file PwmHal.h
 * @brief HAL：主灯 WRGB 与指示灯 PWM 输出
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 封装 led_pwm_port 寄存器路径，业务层不得直接操作 PWM 外设。
 */
#pragma once

#include <stdint.h>

/** @brief PWM 硬件抽象，封装 led_pwm_port */
class PwmHal
{
public:
  /**
   * @brief 设置主灯四通道 PWM 占空（0~1023 原始分辨率）
   * @param w 白光通道占空
   * @param r 红光通道占空
   * @param g 绿光通道占空
   * @param b 蓝光通道占空
   * @return 无
   */
  static void setMainWrgb(uint16_t w, uint16_t r, uint16_t g, uint16_t b);

  /**
   * @brief 设置指示灯 PWM 占空
   * @param duty 占空值（0~1023）
   * @return 无
   */
  static void setIndicatorDuty(uint16_t duty);

  /**
   * @brief 进入低功耗前关闭 PWM 输出
   * @return 无
   */
  static void enterLowPower();
};
