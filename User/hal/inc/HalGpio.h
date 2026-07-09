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

#include "em_gpio.h"

/** @brief GPIO 引脚 HAL，封装电源/充电/指示相关引脚 */
class HalGpio
{
  public:
    enum class GpioPinStateEnum : uint8_t
    {
        GPIO_PIN_RESET = 0u, // 低电平
        GPIO_PIN_SET         // 高电平
    };

  public:
    HalGpio(uint8_t port, uint8_t pin);

    /** @brief 初始化所有电源相关 GPIO
     *  @return 无 */
    void Init(sl_gpio_mode_t mode, GpioPinStateEnum default_state);

    void             SetGpioPinState(GpioPinStateEnum state);
    GpioPinStateEnum GetGpioPinState();

  private:
    sl_gpio_t gpio;
};
