/**
 * @file PinHal.h
 * @brief GPIO 引脚 HAL：电源检测、电池使能、充电控制、红灯 GPIO
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note Service/Policy 层仅通过 PinHal 静态方法访问 GPIO，禁止直接操作寄存器。
 */
#pragma once

#include "em_cmu.h"
#include "sl_clock_manager.h"
#include "sl_gpio.h"
#include "sl_simple_button_btn0_config.h"
#include "sl_simple_rgb_pwm_led_instances.h"
#include "sl_simple_rgb_pwm_led_rgb_led0_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "em_gpio.h"
#include "gpiointerrupt.h"

#ifdef __cplusplus
}
#endif

#define PIN_HIGH 1
#define PIN_LOW  0

#define POWER_IN_DETECT_PORT (SL_GPIO_PORT_A)
#define POWER_IN_DETECT_PIN  8

#define INDIC_R_LED_PORT (SL_GPIO_PORT_A)
#define INDIC_R_LED_PIN  4

#define BAT_EN_PORT (SL_GPIO_PORT_C)
#define BAT_EN_PIN  0

#define LAMP_STATUS_PORT (SL_GPIO_PORT_C)
#define LAMP_STATUS_PIN  1

#define CHARGE_SPEED_PORT (SL_GPIO_PORT_C)
#define CHARGE_SPEED_PIN  2

#define CHARGE_EN_PORT (SL_GPIO_PORT_C)
#define CHARGE_EN_PIN  6

#ifdef __cplusplus

/** @brief GPIO 引脚 HAL，封装电源/充电/指示相关引脚 */
class PinHal
{
public:
  /** @brief 初始化所有电源相关 GPIO
   *  @return 无 */
  static void init();

  /** @brief 读取指定 GPIO 输入电平
   *  @param gpio 引脚描述
   *  @return true 为高电平 */
  static bool getPinValue(sl_gpio_t gpio);

  /** @brief 读取外部电源接入检测引脚
   *  @return true 已接入外部电源 */
  static bool getPowerInDetect();

  /** @brief 读取充电状态引脚
   *  @return 充电芯片状态引脚电平 */
  static bool readChargeStatus();

  /** @brief 控制红色指示灯 GPIO
   *  @param state true=亮，false=灭
   *  @return 无 */
  static void setRedIndicator(bool state);

  /** @brief 控制电池输出使能
   *  @param state true=使能，false=关闭
   *  @return 无 */
  static void setBatteryEnable(bool state);

  /** @brief 设置充电速率（快充/慢充）
   *  @param slow true=慢充，false=快充
   *  @return 无 */
  static void setChargeSpeed(bool slow);

  /** @brief 控制充电 Boost 使能
   *  @param state true=使能，false=关闭
   *  @return 无 */
  static void setChargeEnable(bool state);

  /** @brief 注入 BTN0 双沿中断（修补 SDK 单沿问题）
   *  @return 无 */
  static void injectBtn0DoubleEdgeInterrupt();

  /** @brief RGB PWM 硬件进入低功耗
   *  @return 无 */
  static void rgbEnterLowPower();
};

#endif
