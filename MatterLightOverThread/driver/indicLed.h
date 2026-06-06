#pragma once

#include <stdint.h>

#include "pinManage.h"

#include "sl_pwm.h"
#include "sl_pwm_instances.h"
#include "led_pwm_port.h"

// 初始化/开启独立呼吸灯 (传入目标颜色索引和 App 全局最大亮度)
void Indic_W_Breath_Start(uint8_t brightness);

// 停止独立呼吸灯
void Indic_W_Breath_Stop(void);

// 🎯 核心独立驱动器：请把这个函数挂载在你的 10ms 硬件定时器中断、
// 或者系统 10ms 的 OS Task 循环中。它与原来的代码完全独立运转。
void Indic_W_Breath_Poll_10ms(void);

/**
 * @brief 功能 1：基础常规闪烁（向下兼容，blink_count 为 0 代表无限闪烁）
 * @param duration_ms  一个完整闪烁周期（亮+灭）的时间 (ms)
 * @param blink_count  闪烁的次数
 */
void Indic_Red_Blink_Start(uint16_t duration_ms, uint16_t blink_count);

/**
 * @brief 功能 2：双速单向混合多次闪烁（模式1闪 X 次 -> 模式2闪 Y 次 -> 自动结束）
 * @param m1_duration_ms  模式1速度：一个完整闪烁周期（亮+灭）的时间 (ms)
 * @param m1_count        模式1速度：闪烁的次数
 * @param m2_duration_ms  模式2速度：一个完整闪烁周期（亮+灭）的时间 (ms)
 * @param m2_count        🎯 模式2速度：闪烁的次数
 */
void Indic_Red_Mixed_Blink_Start(uint16_t m1_duration_ms, uint16_t m1_count, uint16_t m2_duration_ms, uint16_t m2_count);

/**
 * @brief 随时一键叫停/关闭红色指示灯
 */
void Indic_Red_Blink_Stop(void);

/**
 * @brief 🎯 核心融合驱动器：请挂载在系统的 10ms 定时器中断服务函数（ISR）中
 */
void Indic_Red_Blink_Poll_10ms(void);