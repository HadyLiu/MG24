/** @file HalPwm.h
 *  @brief 白色 LED PWM 驱动接口
 *  @author hady
 *  @date 2026-06-12
 *  @layer HAL
 *  @note 系统启动阶段调用 init 一次；led_pwm 通过 LedWhitePwmInstance
 * 访问驱动。
 */

#pragma once

#include "sl_pwm.h"
#include "sl_pwm_instances.h"
#include <stdint.h>
#if defined(_SILICON_LABS_32B_SERIES_2)
#include "em_timer.h"
#else
#include "sl_hal_timer.h"
#endif

class HalPwm {
 public:
  HalPwm();
  /**
   * @brief 初始化白色 LED PWM 驱动
   * @param pwm_instance 指向 sl_pwm 实例的指针
   * @return 无
   */
  void Init(sl_pwm_instance_t* pwm_instance);

  /**
   * @brief 设置白色 LED PWM 占空比
   * @param duty 占空值（0~1023）
   * @return 无
   * @note 自动限幅；Series 2 使用 compare/top 比例换算
   */
  void PwmSetDuty(uint16_t duty);

 private:
  sl_pwm_instance_t* pwm_instance;

  /**
   * @brief 使用 10-bit 分辨率设置 PWM 占空比
   * @param duty 占空值（0~1023）
   * @return 无
   * @note 自动限幅；Series 2 使用 compare/top 比例换算
   */
  void PwmSetDutyCycle10bitResolutionRaw(uint16_t duty);
};
