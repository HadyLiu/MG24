/**
 * @file HalGpio.h
 * @brief GPIO 引脚 HAL，封装电源/充电/指示相关引脚
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note PA8 电源检测 GPIO 用于 EM2 唤醒；ISR 内禁止日志，仅调用已注册回调。
 */
#pragma once

#include "HalGpio.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define POWER_IN_DETECT_PORT (SL_GPIO_PORT_A)
#define POWER_IN_DETECT_PIN (8)

#define INDIC_R_LED_PORT (SL_GPIO_PORT_A)
#define INDIC_R_LED_PIN (4)

// 电源管理相关引脚(充电/电池使能、充电状态、灯状态等)
// ADC 引脚
#define USB_IN_PORT (SL_GPIO_PORT_A)
#define USB_IN_PIN (8)

#define BAT_VOLTAGE_PORT (SL_GPIO_PORT_C)
#define BAT_VOLTAGE_PIN (4)

#define BAT_NTC_PORT (SL_GPIO_PORT_C)
#define BAT_NTC_PIN (5)

// GPIO 引脚
#define BAT_EN_PORT (SL_GPIO_PORT_C)
#define BAT_EN_PIN (0)

#define LAMP_STATUS_PORT (SL_GPIO_PORT_C)
#define LAMP_STATUS_PIN (1)

#define CHARGE_SPEED_PORT (SL_GPIO_PORT_C)
#define CHARGE_SPEED_PIN (2)

#define CHARGE_EN_PORT (SL_GPIO_PORT_C)
#define CHARGE_EN_PIN (6)

#if defined(__cplusplus)
}
#endif
