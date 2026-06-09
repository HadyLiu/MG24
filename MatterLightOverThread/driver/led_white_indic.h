#pragma once

#include <stdint.h>

#include "pinManage.h"

#include "sl_pwm.h"
#include "sl_pwm_instances.h"
#include "led_pwm_port.h"

// 传入接口 0-1023
// void Indic_SetDuty(uint16_t w_led);

// 初始化/开启独立呼吸灯 (传入目标颜色索引和 App 全局最大亮度)
void Indic_W_Breath_Start(uint8_t brightness);

// 停止独立呼吸灯
void Indic_W_Breath_Stop(void);

// 🎯 核心独立驱动器：请把这个函数挂载在你的 10ms 硬件定时器中断、
// 或者系统 10ms 的 OS Task 循环中。它与原来的代码完全独立运转。
void Indic_W_Breath_Poll_10ms(void);
