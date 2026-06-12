/**
 * @file PinHal.cpp
 * @brief GPIO 引脚 HAL 实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 */
#include "PinHal.h"

namespace
{

/** @brief 写 GPIO 输出电平
 *  @param port GPIO 端口
 *  @param pin  引脚号
 *  @param high true=高电平
 *  @return 无 */
void writeGpioOutput(sl_gpio_port_t port, uint8_t pin, bool high)
{
  sl_gpio_t gpio = {.port = port, .pin = pin};
  if (high)
  {
    sl_gpio_set_pin(&gpio);
  }
  else
  {
    sl_gpio_clear_pin(&gpio);
  }
}

} // namespace

/** @brief 初始化所有电源相关 GPIO */
void PinHal::init()
{
  sl_gpio_t gpio;

  sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_GPIO);

  gpio.port = POWER_IN_DETECT_PORT;
  gpio.pin  = POWER_IN_DETECT_PIN;
  sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT, PIN_LOW);

  gpio.port = INDIC_R_LED_PORT;
  gpio.pin  = INDIC_R_LED_PIN;
  sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);

  gpio.port = BAT_EN_PORT;
  gpio.pin  = BAT_EN_PIN;
  sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);

  gpio.port = LAMP_STATUS_PORT;
  gpio.pin  = LAMP_STATUS_PIN;
  sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL_FILTER, PIN_LOW);

  gpio.port = CHARGE_SPEED_PORT;
  gpio.pin  = CHARGE_SPEED_PIN;
  sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_WIRED_AND, PIN_HIGH);

  gpio.port = CHARGE_EN_PORT;
  gpio.pin  = CHARGE_EN_PIN;
  sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);
}

/** @brief 读取指定 GPIO 输入电平 */
bool PinHal::getPinValue(sl_gpio_t gpio)
{
  bool pin_value;
  sl_gpio_get_pin_input(&gpio, &pin_value);
  return pin_value;
}

/** @brief 读取外部电源接入检测引脚 */
bool PinHal::getPowerInDetect()
{
  sl_gpio_t gpio = {.port = POWER_IN_DETECT_PORT, .pin = POWER_IN_DETECT_PIN};
  bool      power_in_detected = false;
  sl_gpio_get_pin_input(&gpio, &power_in_detected);
  return power_in_detected;
}

/** @brief 读取充电状态引脚 */
bool PinHal::readChargeStatus()
{
  sl_gpio_t gpio = {.port = LAMP_STATUS_PORT, .pin = LAMP_STATUS_PIN};
  bool      level = false;
  sl_gpio_get_pin_input(&gpio, &level);
  return level;
}

/** @brief 控制红色指示灯 GPIO */
void PinHal::setRedIndicator(bool state)
{
  writeGpioOutput(INDIC_R_LED_PORT, INDIC_R_LED_PIN, state);
}

/** @brief 控制电池输出使能 */
void PinHal::setBatteryEnable(bool state)
{
  writeGpioOutput(BAT_EN_PORT, BAT_EN_PIN, state);
}

/** @brief 设置充电速率 */
void PinHal::setChargeSpeed(bool slow)
{
  writeGpioOutput(CHARGE_SPEED_PORT, CHARGE_SPEED_PIN, slow);
}

/** @brief 控制充电 Boost 使能 */
void PinHal::setChargeEnable(bool state)
{
  sl_gpio_t gpio = {.port = CHARGE_EN_PORT, .pin = CHARGE_EN_PIN};
  if (state)
  {
    sl_gpio_set_pin(&gpio);
  }
  else
  {
    sl_gpio_clear_pin(&gpio);
  }
  sl_gpio_set_pin(&gpio);
}

/** @brief 注入 BTN0 双沿中断 */
void PinHal::injectBtn0DoubleEdgeInterrupt()
{
  sl_gpio_t gpio = {.port = SL_SIMPLE_BUTTON_BTN0_PORT,
                    .pin  = SL_SIMPLE_BUTTON_BTN0_PIN};

  sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL_FILTER, PIN_LOW);

  uint32_t mask      = (1u << SL_SIMPLE_BUTTON_BTN0_PIN);
  GPIO->EXTIRISE_SET = mask;
  GPIO->EXTIFALL_SET = mask;
  GPIO->IEN_SET      = mask;
}

/** @brief RGB PWM 硬件进入低功耗 */
void PinHal::rgbEnterLowPower()
{
  sl_led_turn_off((sl_led_t*)&sl_simple_rgb_pwm_led_rgb_led0);
  CMU_ClockEnable(cmuClock_TIMER4, false);
  GPIO_PinModeSet((GPIO_Port_TypeDef)SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PORT,
                  SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet((GPIO_Port_TypeDef)SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PORT,
                  SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet((GPIO_Port_TypeDef)SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PORT,
                  SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PIN, gpioModeDisabled, 0);
}

/** @brief AppTask 兼容入口：注入 BTN0 双沿中断
 *  @return 无
 *  @note 保留供 AppTask.cpp 链接，内部委托 PinHal::injectBtn0DoubleEdgeInterrupt
 */
void inject_btn0_double_edge_interrupt_ext(void)
{
  PinHal::injectBtn0DoubleEdgeInterrupt();
}
