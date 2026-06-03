/**
 * @file main_light_control.h
 * @brief 极致优化版：主灯逻辑状态机与控制核心头文件
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>



// 原始高分辨率路径最大 TOP 值（禁止低分辨率占空比接口）
#define PWM_MAX_RESOLUTION          20000 
#define TOTAL_COLOR_MODES           13

/**
 * @brief 主灯高优先级状态枚举
 */
typedef enum {
    LAMP_STATE_NORMAL = 0,            // 正常用户控制 / Matter控制
    LAMP_STATE_FACTORY_RESET,         // 清除配网中（长按5s-10s闪烁红色）
    LAMP_STATE_CRITICAL_BATTERY,      // 电池极低警告
    LAMP_STATE_IDENTIFY,              // Matter 识别闪烁
    LAMP_STATE_CHARGE_ERROR,          // 充电异常
    LAMP_STATE_LOW_BATTERY_ALERT      // 正常低电量提醒
} Lamp_Priority_State_t;

/**
 * @brief 优化版平滑过渡结构体（内部改用全整数/定点数加速）
 */
typedef struct {
    // 物理 Compare 当前输出值 (0 - PWM_MAX_RESOLUTION)
    uint32_t current_w;
    uint32_t current_r;
    uint32_t current_g;
    uint32_t current_b;

    // 渐变起始物理值
    uint32_t start_w;
    uint32_t start_r;
    uint32_t start_g;
    uint32_t start_b;

    // 渐变目标物理值
    uint32_t target_w;
    uint32_t target_r;
    uint32_t target_g;
    uint32_t target_b;

    // 动画时间线（单位: ms）
    uint32_t total_duration;
    uint32_t elapsed_time;
    uint8_t  easing_type;       // 0: 线性, 1: EaseOut, 2: 呼吸(查表)
    bool     is_animating;

    // 记忆参数
    uint8_t  saved_brightness;  // 0, 50, 100 或 2-100
    uint8_t  saved_color_idx;   // 0 - 12
    bool     is_on;             // 开关状态
} Light_Control_t;

// ============================================================================
// API 接口
// ============================================================================

void Light_System_Init(void);
void Light_Logic_Tick_10ms(void);
void Light_Control_Short_Press(void);
void Light_Control_Double_Click(void);
void Light_Control_From_Matter(uint8_t app_brightness, uint8_t color_idx);
void Light_Set_Priority_State(Lamp_Priority_State_t new_state);

/**
 * @brief 供外部主循环调用的记忆保存检查函数（无感延迟保存，防止写Flash卡顿）
 */
void Light_Storage_Poll_100ms(void);

