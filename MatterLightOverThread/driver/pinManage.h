#pragma once

#include "sl_simple_button_btn0_config.h"
#include "sl_clock_manager.h"
#include "sl_gpio.h"

#define PIN_HIGH 1
#define PIN_LOW 0


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


#define POWER_IN_DETECT_PORT (SL_GPIO_PORT_A)
#define POWER_IN_DETECT_PIN 8

#define INDIC_R_LED_PORT (SL_GPIO_PORT_A)
#define INDIC_R_LED_PIN 4

#define BAT_EN_PORT (SL_GPIO_PORT_C)
#define BAT_EN_PIN 0

#define LAMP_STATUS_PORT (SL_GPIO_PORT_C)
#define LAMP_STATUS_PIN 1

#define CHARGE_SPEED_PORT (SL_GPIO_PORT_C)
#define CHARGE_SPEED_PIN 2

#define CHARGE_EN_PORT (SL_GPIO_PORT_C)
#define CHARGE_EN_PIN 6

extern void gpio_init(void);
extern bool gpio_get_pin_value(sl_gpio_t gpio);

extern void indic_r(bool state);
extern void bat_en(bool state);
extern void charge_speed_set(bool fast);
extern void charge_en(bool state);
extern bool charge_status_read(void);
extern bool get_power_in_detect(void);

#define INDIC_R_LED_ON() indic_r(true)
#define INDIC_R_LED_OFF() indic_r(false)

#define BAT_EN_ON() bat_en(true)
#define BAT_EN_OFF() bat_en(false)

#define CHARGE_EN_ON() charge_en(true)
#define CHARGE_EN_OFF() charge_en(false)

#define CHARGE_STATUS_READ() charge_status_read()

// 快充：拉低引脚，慢充：保持开漏输出的引脚处于高电平状态
#define CHARGE_SPEED_FAST() charge_speed_set(false)
#define CHARGE_SPEED_SLOW() charge_speed_set(true)

#define GET_POWER_IN_DETECT() get_power_in_detect()



//#define INDIC_R_LED_ON() sl_gpio_set_pin(&INDIC_R_LED_PORT_PIN)
//#define INDIC_R_LED_OFF() sl_gpio_clear_pin(&INDIC_R_LED_PORT_PIN)
//
//#define INDIC_R_LED_ON() sl_gpio_set_pin(&INDIC_R_LED_PORT_PIN)
//#define INDIC_R_LED_OFF() sl_gpio_clear_pin(&INDIC_R_LED_PORT_PIN)
//
//#define BAT_EN_ON() sl_gpio_set_pin(&BAT_EN_PORT_PIN)
//#define BAT_EN_OFF() sl_gpio_clear_pin(&BAT_EN_PORT_PIN)
//
//#define CHARGE_EN_ON() sl_gpio_set_pin(&CHARGE_EN_PORT_PIN)
//#define CHARGE_EN_OFF() sl_gpio_clear_pin(&CHARGE_EN_PORT_PIN)
