/**
 * @file LedWhitePwm.cpp
 * @author Hady
 * @date 2026-06-12
 * @brief 白色 LED PWM 驱动实现
 * @layer HAL
 * @note 由 bsp_rgbw_led 调用，提供单色 LED PWM 设置接口
 */

#include "HalPwm.h"

/**
 * @brief 默认构造函数
 * @return 无
 */
HalPwm::HalPwm()
{
  pwm_instance = nullptr;
}

/**
 * @brief 初始化 PWM 输出
 * @param pwm_instance 指向 sl_pwm 实例的指针
 * @return 无
 */
void HalPwm::Init(sl_pwm_instance_t* pwm_instance)
{
  this->pwm_instance = pwm_instance;
  // 已在 sl_pwm_init 初始化，无需重复初始化
}

/**
 * @brief 设置白色 LED PWM 占空比
 * @param duty 占空值（0~1023）
 * @return 无
 * @note 自动限幅；Series 2 使用 compare/top 比例换算
 */
void HalPwm::PwmSetDuty(uint16_t duty)
{
  if (duty == 0)
  {
    sl_pwm_stop(pwm_instance);
  }
  else
  {
    PwmSetDutyCycle10bitResolutionRaw(duty);
    sl_pwm_start(pwm_instance);
  }
}

/**
 * @brief 设置白色 LED PWM 占空比
 * @param duty 占空值（0~1023）
 * @return 无
 * @note 自动限幅；Series 2 使用 compare/top 比例换算
 */
void HalPwm::PwmSetDutyCycle10bitResolutionRaw(uint16_t duty)
{
  if (duty >= 1023)
  {
    duty = 1023; // 限幅防越界
  }

#if defined(_SILICON_LABS_32B_SERIES_2)
  // 适用于 Series 2 芯片 (如 EFR32MG24)
  uint32_t top         = TIMER_TopGet(pwm_instance->timer);
  uint32_t compare_val = (top * duty); // 右移9位相当于除以512
  compare_val /= 638;
  TIMER_CompareBufSet(pwm_instance->timer, pwm_instance->channel, compare_val);
#else
  // 适用于 Series 3 芯片
  uint32_t top = sl_hal_timer_get_top(pwm_instance->timer);
  // 右移10位相当于除以1024
  sl_hal_timer_channel_set_compare_buffer(
      pwm_instance->timer, pwm_instance->channel, (top * duty) >> 10);
#endif
}
