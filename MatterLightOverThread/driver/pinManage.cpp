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
    gpio.pin = LAMP_STATUS_PIN; //配置上拉输入
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL_FILTER, PIN_LOW);
    
    gpio.port = CHARGE_SPEED_PORT;
    gpio.pin = CHARGE_SPEED_PIN; //开漏输出 
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_WIRED_AND, PIN_HIGH);
    
    gpio.port = CHARGE_EN_PORT;
    gpio.pin = CHARGE_EN_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);

    gpio.port = SL_SIMPLE_BUTTON_BTN0_PORT;
    gpio.pin = SL_SIMPLE_BUTTON_BTN0_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL_FILTER, PIN_LOW);
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
    sl_gpio_t gpio = {
        .port = POWER_IN_DETECT_PORT,
        .pin = POWER_IN_DETECT_PIN
    };
    bool power_in_detected = false;
    sl_gpio_get_pin_input(&gpio, &power_in_detected);
    return power_in_detected;
}

bool charge_status_read(void)
{
    sl_gpio_t gpio = {
        .port = LAMP_STATUS_PORT,
        .pin = LAMP_STATUS_PIN
    };
    bool is_fast_charge =false;
    sl_gpio_get_pin_input(&gpio, &is_fast_charge);
    return is_fast_charge;
}

/**
 * @brief 控制红色指示灯亮灭的函数
 */
void indic_r_led(bool state)
{
    sl_gpio_t gpio={
        .port = INDIC_R_LED_PORT,
        .pin = INDIC_R_LED_PIN
    };
    if (state) {
        sl_gpio_set_pin(&gpio);
    } else {
        sl_gpio_clear_pin(&gpio);
    }
}

void bat_en(bool state)
{
    sl_gpio_t gpio={
        .port = BAT_EN_PORT,
        .pin = BAT_EN_PIN
    };
    if (state) {
        sl_gpio_set_pin(&gpio);
    } else {
        sl_gpio_clear_pin(&gpio);
    }
}

void charge_speed_set(bool fast)
{
    sl_gpio_t gpio={
        .port = CHARGE_SPEED_PORT,
        .pin = CHARGE_SPEED_PIN
    };
    if (fast) {
        sl_gpio_clear_pin(&gpio); // 快充：拉低引脚
    } else {
        sl_gpio_set_pin(&gpio);   // 慢充：拉高引脚
    }
}

void charge_en(bool state)
{
    sl_gpio_t gpio={
        .port = CHARGE_EN_PORT,
        .pin = CHARGE_EN_PIN
    };
    if (state) {
        sl_gpio_set_pin(&gpio);
    } else {
        sl_gpio_clear_pin(&gpio);
    }
    sl_gpio_set_pin(&gpio);
}

