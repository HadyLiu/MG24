#include "pinManage.h"

// PA4 Indic_R 输出 高电平示灯亮，低电平示灯灭
// PA8 模拟输入 USB状态
// PD02  RGB  输出使能脚
// PD01  W    输出PWM控制W灯亮度
// PD00  RGB  协议芯片控制RGB灯亮度

// PC00 BAT_EN      输出
// PC01 LAMP_STATUS  输入
// PC02 CHARGE_SPEED 输入模式
// PC03 NC
// PC04 VBT_AD
// PC05 BT_T_AD
// PC06 CHARGE_EN  输出
// PC07

void inject_double_edge_interrupt_ext(sl_gpio_port_t port, uint8_t pin);

void gpio_init(void)
{
    sl_gpio_t gpio;

    // 使能 GPIO 时钟
    sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_GPIO);

    gpio.port = POWER_IN_DETECT_PORT;
    gpio.pin = POWER_IN_DETECT_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT, PIN_LOW);

    gpio.port = INDIC_R_LED_PORT;
    gpio.pin = INDIC_R_LED_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);

    gpio.port = BAT_EN_PORT;
    gpio.pin = BAT_EN_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);

    gpio.port = LAMP_STATUS_PORT;
    gpio.pin = LAMP_STATUS_PIN; // 配置上拉输入
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL_FILTER, PIN_LOW);

    gpio.port = CHARGE_SPEED_PORT;
    gpio.pin = CHARGE_SPEED_PIN; // 开漏输出
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_WIRED_AND, PIN_HIGH);

    gpio.port = CHARGE_EN_PORT;
    gpio.pin = CHARGE_EN_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);

    // gpio.port = SL_SIMPLE_BUTTON_BTN0_PORT;
    // gpio.pin = SL_SIMPLE_BUTTON_BTN0_PIN;
    // sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL_FILTER, PIN_LOW);
    // inject_double_edge_interrupt_ext(SL_SIMPLE_BUTTON_BTN0_PORT, SL_SIMPLE_BUTTON_BTN0_PIN);
}

/**
 * @brief 读取指定 GPIO 引脚的输入值
 */
bool gpio_get_pin_value(sl_gpio_t gpio)
{
    bool pin_value;
    sl_gpio_get_pin_input(&gpio, &pin_value);
    return pin_value;
}

bool get_power_in_detect(void)
{
    sl_gpio_t gpio = {.port = POWER_IN_DETECT_PORT, .pin = POWER_IN_DETECT_PIN};
    bool      power_in_detected = false;
    sl_gpio_get_pin_input(&gpio, &power_in_detected);
    return power_in_detected;
}

bool charge_status_read(void)
{
    sl_gpio_t gpio = {.port = LAMP_STATUS_PORT, .pin = LAMP_STATUS_PIN};
    bool      is_fast_charge = false;
    sl_gpio_get_pin_input(&gpio, &is_fast_charge);
    return is_fast_charge;
}

/**
 * @brief 控制红色指示灯亮灭的函数
 */
void indic_r_led(bool state)
{
    sl_gpio_t gpio = {.port = INDIC_R_LED_PORT, .pin = INDIC_R_LED_PIN};
    if (state)
    {
        sl_gpio_set_pin(&gpio);
    }
    else
    {
        sl_gpio_clear_pin(&gpio);
    }
}

void bat_en(bool state)
{
    sl_gpio_t gpio = {.port = BAT_EN_PORT, .pin = BAT_EN_PIN};
    if (state)
    {
        sl_gpio_set_pin(&gpio);
    }
    else
    {
        sl_gpio_clear_pin(&gpio);
    }
}

void charge_speed_set(bool fast)
{
    sl_gpio_t gpio = {.port = CHARGE_SPEED_PORT, .pin = CHARGE_SPEED_PIN};
    if (fast)
    {
        sl_gpio_set_pin(&gpio);
    }
    else
    {
        sl_gpio_clear_pin(&gpio);
    }
}

void charge_en(bool state)
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

// 重写 btn中断
void inject_btn0_double_edge_interrupt_ext(void)
{
    sl_gpio_t gpio = {.port = SL_SIMPLE_BUTTON_BTN0_PORT, .pin = SL_SIMPLE_BUTTON_BTN0_PIN};

    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL_FILTER, PIN_LOW);

    //// 1. 动态查询：拿到原厂初始化时为该引脚分配的内部外部中断号
    // int32_t int_no = (int32_t)GPIOINT_CallbackRegisterExt(SL_SIMPLE_BUTTON_BTN0_PIN, NULL, NULL);

    //// 2. 精准注入：如果中断号有效（>= 0），强行覆盖为双沿
    // if (int_no >= 0)
    //{
    //     sl_gpio_configure_external_interrupt(&gpio, &int_no,
    //                                          SL_GPIO_INTERRUPT_RISING_FALLING_EDGE, // 🎯 强行注入双沿！
    //                                          NULL, NULL);
    // }
    uint32_t mask = (1u << SL_SIMPLE_BUTTON_BTN0_PIN);

    // ⚡ 针对 EFR32MG24 (Series 2) 芯片硬件寄存器的终极绝对值注入
    // 直接操作指针，任何人、任何SDK函数都拦截不住，无视低功耗状态，直接强刷硬件

    // 1. 强开上升沿检测
    GPIO->EXTIRISE_SET = mask;

    // 2. 强开下降沿检测 (精准修补原厂Bug丢掉的松开边沿)
    GPIO->EXTIFALL_SET = mask;

    // 3. 确保该通道的外部中断使能没有被关闭
    GPIO->IEN_SET = mask;
}

// ===========================================================================
// 🌙 关闭 RGB
// ===========================================================================
void rgb_hardware_enter_low_power(void)
{
    // 1. 关闭应用层输出并释放功耗锁
    sl_led_turn_off((sl_led_t *)&sl_simple_rgb_pwm_led_rgb_led0);

    CMU_ClockEnable(cmuClock_TIMER4, false);

    // 3. 将 RGB 涉及的三个物理引脚（PB2, PB3, PB4）强制 Disabled，彻底阻断漏电
    GPIO_PinModeSet((GPIO_Port_TypeDef)SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet((GPIO_Port_TypeDef)SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet((GPIO_Port_TypeDef)SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PIN, gpioModeDisabled, 0);
}