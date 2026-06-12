/**
 * @file PwmHal.h
 * @brief HAL：主灯 WRGB 与指示灯 PWM 输出
 * @layer HAL
 */
#pragma once

#include <stdint.h>

/** @brief PWM 硬件抽象，封装 led_pwm_port */
class PwmHal
{
public:
    static void setMainWrgb(uint16_t w, uint16_t r, uint16_t g, uint16_t b);
    static void setIndicatorDuty(uint16_t duty);
    static void enterLowPower();
};
