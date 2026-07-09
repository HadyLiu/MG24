/**
 * @file PinHal.cpp
 * @brief GPIO 引脚 HAL 实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note PA8 电源检测 GPIO 用于 EM2 唤醒；ISR 内禁止日志，仅调用已注册回调。
 */
#include "HalGpio.h"

HalGpio::HalGpio(uint8_t port, uint8_t pin) : gpio({.port = port, .pin = pin})
{
}

/** @brief 初始化所有电源相关 GPIO
 *  @return 无
 */
void HalGpio::Init(sl_gpio_mode_t mode, GpioPinStateEnum default_state)
{
    sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_GPIO);
    sl_gpio_set_pin_mode(&gpio, mode, static_cast<bool>(default_state));
}

/**
 * @brief 设置 GPIO 输出电平
 * @param state GPIO_PIN_SET=高电平, GPIO_PIN_RESET=低电平
 * @return 无
 */
void HalGpio::SetGpioPinState(GpioPinStateEnum state)
{
    if (state == GpioPinStateEnum::GPIO_PIN_SET)
    {
        sl_gpio_set_pin(&gpio);
    }
    else
    {
        sl_gpio_clear_pin(&gpio);
    }
}

/**
 * @brief 读取指定 GPIO 输入电平
 * @return HalGpio::GpioPinStateEnum=高电平, HalGpio::GpioPinStateEnum=低电平
 */
HalGpio::GpioPinStateEnum HalGpio::GetGpioPinState()
{
    bool value = false;
    sl_gpio_get_pin_input(&gpio, &value);
    return value ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
}
