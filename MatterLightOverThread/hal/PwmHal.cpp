/**
 * @file PwmHal.cpp
 * @brief HAL：PWM 输出实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 走 TOP/Compare 原始分辨率路径，禁止低分辨率占空比接口。
 */
#include "PwmHal.h"
#include "PinHal.h"
#include "led_pwm.h"

/** @brief 设置主灯 W/R/G/B 四通道 PWM（0~1023） */
void PwmHal::setMainWrgb(uint16_t w, uint16_t r, uint16_t g, uint16_t b)
{
  LED_HW_SetWRGB(w, r, g, b);
}

/** @brief 设置指示灯 PWM 占空比 */
void PwmHal::setIndicatorDuty(uint16_t duty)
{
  Indic_SetDuty(duty);
}

/** @brief 进入 RGB/PWM 低功耗模式 */
void PwmHal::enterLowPower()
{
  PinHal::rgbEnterLowPower();
}
