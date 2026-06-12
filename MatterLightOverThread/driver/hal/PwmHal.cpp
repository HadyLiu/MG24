/**
 * @file PwmHal.cpp
 * @brief HAL：PWM 输出实现
 * @layer HAL
 */
#include "PwmHal.h"
#include "../led_pwm_port.h"
#include "../pinManage.h"

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
    rgb_hardware_enter_low_power();
}
