/**
 * @file led_pwm.h
 * @brief 主灯 W 通道与指示灯 PWM 底层驱动（C 链接）
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 仅供 PwmHal / IndicatorHal 调用，Service 层禁止直接 include。
 */
#ifndef LED_PWM_PORT_H
#define LED_PWM_PORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "sl_pwm.h"
#include "sl_pwm_instances.h"
#if defined(_SILICON_LABS_32B_SERIES_2)
#include "em_timer.h"
#else
#include "sl_hal_timer.h"
#endif

/**
 * @brief 按 10-bit 分辨率设置 PWM 占空比
 * @param pwm      PWM 实例指针
 * @param duty_val 占空比 0~1023
 * @return 无
 */
void my_pwm_set_duty_cycle_10bit_resolution(sl_pwm_instance_t* pwm,
                                            uint16_t duty_val);

/**
 * @brief 读取当前 10-bit 占空比
 * @param pwm PWM 实例指针
 * @return 占空比 0~1023
 */
uint16_t my_pwm_get_duty_cycle_10bit_resolution(sl_pwm_instance_t* pwm);

/**
 * @brief 设置指示灯 PWM 占空比
 * @param w_led 占空比 0~1023
 * @return 无
 */
void Indic_SetDuty(uint16_t w_led);

/**
 * @brief 设置主灯 W 通道 PWM 占空比
 * @param w 占空比 0~1023
 * @return 无
 */
void LED_W_SetDuty(uint16_t w);

/**
 * @brief 设置主灯 WRGB 四通道 PWM
 * @param w 白光 0~1023
 * @param r 红光 0~1023
 * @param g 绿光 0~1023
 * @param b 蓝光 0~1023
 * @return 无
 */
void LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b);

#ifdef __cplusplus
}
#endif

#endif
