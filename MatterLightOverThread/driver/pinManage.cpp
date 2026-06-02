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



void GPIOInit(void)
{
    sl_gpio_t gpio;

  // 使能 GPIO 时钟
    sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_GPIO);
  
    gpio.port = INDIC_R_LED_PORT;
    gpio.pin = INDIC_R_LED_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);
    
    gpio.port = BAT_EN_PORT;
    gpio.pin = BAT_EN_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);
    
    gpio.port = LAMP_STATUS_PORT;
    gpio.pin = LAMP_STATUS_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT, PIN_LOW);
    
    gpio.port = CHARGE_SPEED_PORT;
    gpio.pin = CHARGE_SPEED_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT, PIN_LOW);
    
    gpio.port = CHARGE_EN_PORT;
    gpio.pin = CHARGE_EN_PIN;
    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_PUSH_PULL, PIN_LOW);
}





bool gpio_get_pin_value(sl_gpio_t gpio)
{
    bool pin_value;
    sl_gpio_get_pin_input(&gpio, &pin_value);
    return pin_value;
}

