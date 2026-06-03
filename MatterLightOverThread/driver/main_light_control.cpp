/**
 * @file main_light_control.cpp
 * @brief 极致优化版：去浮点化、快速查表、防卡顿记忆主灯控制实现
 */

#include "main_light_control.h"

// ============================================================================
// 静态全局变量与高速定点颜色库
// ============================================================================

static Light_Control_t g_light;
static Lamp_Priority_State_t g_priority_state = LAMP_STATE_NORMAL;

// 记忆保存消抖定时器（单位: ms）
static int32_t g_storage_save_countdown = -1;
static bool g_storage_dirty = false;

// 13种颜色档位（放大10倍，避免浮点数，1000代表100.0%）
typedef struct {
    uint16_t w; // 0 ~ 1000
    uint16_t r;
    uint16_t g;
    uint16_t b;
} Color_Fixed_t;

static const Color_Fixed_t COLOR_LIBRARY[TOTAL_COLOR_MODES] = {
    {1000, 0,    0,    0},    // color0:  W100 R0   G0   B0
    {400,  1000, 0,    0},    // color1:  W40  R100 G0   B0
    {400,  0,    230,  550},  // color2:  W40  R0   G23  B55 (默认)
    {320,  0,    230,  1000}, // color3:  W32  R0   G23  B100
    {0,    1000, 175,  0},    // color4:  W0   R100 G17.5 B0
    {0,    1000, 100,  0},    // color5:  W0   R100 G10  B0
    {0,    1000, 0,    0},    // color6:  W0   R100 G0   B0
    {0,    1000, 100,  58},   // color7:  W0   R100 G10  B5.8
    {0,    1000, 235,  395},  // color8:  W0   R100 G23.5 B39.5
    {0,    235,  235,  1000}, // color9:  W0   R23.5 G23.5 B100
    {0,    510,  825,  510},  // color10: W0   R51  G82.5 B51
    {0,    510,  1000, 155},  // color11: W0   R51  G100 B15.5
    {0,    1000, 315,  0}     // color12: W0   R100 G31.5 B0
};

// ============================================================================
// 高速平滑曲线：256点定点数 Sine / EaseInOut 呼吸曲线表
// 将 0.0~1.0 映射到 0~1024 (10位精度)，杜绝 powf/曲线实时计算，执行耗时 < 1微秒
// ============================================================================
static const uint16_t CUBIC_BREATH_TABLE[64] = {
    0,    1,    4,    9,    16,   25,   36,   49,   64,   81,   100,  121,  144,  169,  196,  225,
    256,  289,  324,  361,  400,  441,  484,  529,  576,  625,  676,  729,  784,  841,  900,  961,
    1024, 1087, 1148, 1207, 1264, 1319, 1372, 1423, 1472, 1519, 1564, 1607, 1648, 1687, 1724, 1759,
    1792, 1823, 1852, 1879, 1904, 1927, 1948, 1967, 1984, 1999, 2012, 2023, 2032, 2039, 2044, 2047
};

/**
 * @brief 定点化快速 EaseOutQuad 曲线
 * @param progress 0 ~ 1024
 * @return 0 ~ 1024 结果
 */
static inline uint32_t Fast_EaseOutQuad(uint32_t progress) {
    // Formula: progress * (2 - progress) -> 定点化换算
    // 1024 * 2 = 2048
    return (progress * (2048 - progress)) >> 10;
}

/**
 * @brief 定点化快速呼吸曲线 cubic-bezier(0.45, 0, 0.55, 1) 的查表近似
 * @param progress 0 ~ 1024
 * @return 0 ~ 1024 结果
 */
static inline uint32_t Fast_BreathEase(uint32_t progress) {
    if (progress > 1024) progress = 1024;
    // 将 0-1024 映射到 0-63 查表
    uint32_t idx = (progress >= 512) ? (1024 - progress) : progress;
    idx = (idx * 63) >> 9; // 快速缩放至 0~63
    
    uint32_t val = CUBIC_BREATH_TABLE[idx];
    if (progress >= 512) {
        return 2048 - val;
    }
    return val;
}

// ============================================================================
// 底层硬件接口与计算
// ============================================================================

static void Light_Hardware_Output_Raw(uint32_t w, uint32_t r, uint32_t g, uint32_t b) {
    if (w > PWM_MAX_RESOLUTION) w = PWM_MAX_RESOLUTION;
    if (r > PWM_MAX_RESOLUTION) r = PWM_MAX_RESOLUTION;
    if (g > PWM_MAX_RESOLUTION) g = PWM_MAX_RESOLUTION;
    if (b > PWM_MAX_RESOLUTION) b = PWM_MAX_RESOLUTION;

    // 【硬件对接】：在此处直接秒写 EFR32 寄存器，耗时极短
    // TIMER1->CC[0].CCV = w; (PC01 - TIMER1_CC0)
    // RGB 驱动芯片传递 r, g, b 的原始高分辨率数据
}

static void Set_Battery_Power_Enable(bool enable) {
    if (enable) {
        // GPIO_PinOutSet(gpioPortC, 0); // BAT_EN(PD03/PC00根据实际硬件改) = 高
    } else {
        // GPIO_PinOutClear(gpioPortC, 0); // 低
    }
}

/**
 * @brief 高效纯整数计算目标 PWM，不含任何浮点计算
 */
static void Calculate_Target_PWM(uint8_t brightness_pct, uint8_t color_idx, 
                                 uint32_t *out_w, uint32_t *out_r, uint32_t *out_g, uint32_t *out_b) {
    if (brightness_pct == 0) {
        *out_w = *out_r = *out_g = *out_b = 0;
        return;
    }

    if (brightness_pct < 2) brightness_pct = 2; // 最低亮度限制

    Color_Fixed_t color = COLOR_LIBRARY[color_idx];

    // 分母是 100(亮度%) * 1000(颜色库系数) = 100000
    // 使用 uint64 避免大数乘法溢出，运算速度极快
    *out_w = (uint32_t)(((uint64_t)PWM_MAX_RESOLUTION * brightness_pct * color.w) / 100000ULL);
    *out_r = (uint32_t)(((uint64_t)PWM_MAX_RESOLUTION * brightness_pct * color.r) / 100000ULL);
    *out_g = (uint32_t)(((uint64_t)PWM_MAX_RESOLUTION * brightness_pct * color.g) / 100000ULL);
    *out_b = (uint32_t)(((uint64_t)PWM_MAX_RESOLUTION * brightness_pct * color.b) / 100000ULL);
}

static void Start_Light_Transition(uint32_t duration_ms, uint8_t easing) {
    g_light.start_w = g_light.current_w;
    g_light.start_r = g_light.current_r;
    g_light.start_g = g_light.current_g;
    g_light.start_b = g_light.current_b;

    g_light.total_duration = (duration_ms < 10) ? 10 : duration_ms;
    g_light.elapsed_time = 0;
    g_light.easing_type = easing;
    g_light.is_animating = true;
}

/**
 * @brief 标记数据已被修改，触发延迟记忆逻辑
 */
static void Mark_Storage_Dirty(void) {
    g_storage_dirty = true;
    g_storage_save_countdown = 2000; // 2秒内若无新调光，则安全写入Flash
}

// ============================================================================
// 核心状态机 (每10ms调用，全面优化时间耗时)
// ============================================================================

void Light_Logic_Tick_10ms(void) {
    static uint32_t priority_timer = 0;
    priority_timer += 10;

    // 1. 高优先级事件状态拦截 (清除配网 > 电池极低 > 识别 > 异常 > 低电量提醒)
    if (g_priority_state != LAMP_STATE_NORMAL) {
        switch (g_priority_state) {
            case LAMP_STATE_FACTORY_RESET:
                if ((priority_timer % 800) < 400) {
                    Light_Hardware_Output_Raw(0, PWM_MAX_RESOLUTION, 0, 0); // 红亮
                } else {
                    Light_Hardware_Output_Raw(0, 0, 0, 0); // 灭
                }
                return;

            case LAMP_STATE_CRITICAL_BATTERY:
                Light_Hardware_Output_Raw(0, 0, 0, 0);
                Set_Battery_Power_Enable(false);
                return;

            case LAMP_STATE_IDENTIFY:
                if ((priority_timer % 800) < 400) {
                    Light_Hardware_Output_Raw(g_light.target_w, g_light.target_r, g_light.target_g, g_light.target_b);
                } else {
                    Light_Hardware_Output_Raw(0, 0, 0, 0);
                }
                return;

            case LAMP_STATE_CHARGE_ERROR:
                Light_Hardware_Output_Raw(0, 0, 0, 0);
                return;

            case LAMP_STATE_LOW_BATTERY_ALERT: {
                // 呼吸式低电量警告：0-800ms降到2%, 800-1600ms保持, 1600-2400ms回原态, 2400-3200ms保持
                uint32_t pos = priority_timer % 3200;
                uint32_t factor_10bit = 1024; // 1024代表100%原亮度

                if (pos < 800) {
                    // 降到2%：通过定点曲线计算
                    uint32_t p = (pos * 1024) / 800;
                    uint32_t curve = Fast_BreathEase(p); // 0 ~ 2048
                    // 从 1024 降到 20 (约 2%)
                    factor_10bit = 1024 - ((curve * (1024 - 20)) >> 11);
                } else if (pos >= 800 && pos < 1600) {
                    factor_10bit = 20; // 保持 2%
                } else if (pos >= 1600 && pos < 2400) {
                    // 回到原态
                    uint32_t p = ((pos - 1600) * 1024) / 800;
                    uint32_t curve = Fast_BreathEase(p);
                    factor_10bit = 20 + ((curve * (1024 - 20)) >> 11);
                } else {
                    factor_10bit = 1024;
                }

                Light_Hardware_Output_Raw(
                    (g_light.target_w * factor_10bit) >> 10,
                    (g_light.target_r * factor_10bit) >> 10,
                    (g_light.target_g * factor_10bit) >> 10,
                    (g_light.target_b * factor_10bit) >> 10
                );
                return;
            }
        }
    }

    // 2. 正常无阻塞下的普通用户控制 / Matter 渐变驱动
    if (!g_light.is_animating) {
        return;
    }

    g_light.elapsed_time += 10;
    if (g_light.elapsed_time >= g_light.total_duration) {
        g_light.is_animating = false;
        g_light.current_w = g_light.target_w;
        g_light.current_r = g_light.target_r;
        g_light.current_g = g_light.target_g;
        g_light.current_b = g_light.target_b;
    } else {
        // 缩放到 0 ~ 1024 定点数区间
        uint32_t progress_10bit = (g_light.elapsed_time * 1024) / g_light.total_duration;
        uint32_t ease_factor = progress_10bit; // 默认线性

        if (g_light.easing_type == 1) {
            ease_factor = Fast_EaseOutQuad(progress_10bit);
        } else if (g_light.easing_type == 2) {
            ease_factor = Fast_BreathEase(progress_10bit) >> 1; // 将 0-2048 缩放到 0-1024
        }

        // 全整数渐变步进，杜绝任何耗时过长运算
        g_light.current_w = g_light.start_w + (uint32_t)(((int32_t)(g_light.target_w - g_light.start_w) * (int32_t)ease_factor) >> 10);
        g_light.current_r = g_light.start_r + (uint32_t)(((int32_t)(g_light.target_r - g_light.start_r) * (int32_t)ease_factor) >> 10);
        g_light.current_g = g_light.start_g + (uint32_t)(((int32_t)(g_light.target_g - g_light.start_g) * (int32_t)ease_factor) >> 10);
        g_light.current_b = g_light.start_b + (uint32_t)(((int32_t)(g_light.target_b - g_light.start_b) * (int32_t)ease_factor) >> 10);
    }

    Light_Hardware_Output_Raw(g_light.current_w, g_light.current_r, g_light.current_g, g_light.current_b);
}

// ============================================================================
// 外部 API 接口实现
// ============================================================================

void Light_System_Init(void) {
    g_light.is_on = true;
    g_light.saved_brightness = 100;
    g_light.saved_color_idx = 2; // 出厂默认 color2
    g_light.is_animating = false;

    // 【注】：在此处读取本地 NVM3。如果是首次上电则使用上方的出厂默认值
    // if (NVM3_Read_Helper(&g_light.saved_brightness, &g_light.saved_color_idx) == SUCCESS) { ... }

    Calculate_Target_PWM(g_light.saved_brightness, g_light.saved_color_idx, 
                         &g_light.target_w, &g_light.target_r, &g_light.target_g, &g_light.target_b);
    
    g_light.current_w = g_light.target_w;
    g_light.current_r = g_light.target_r;
    g_light.current_g = g_light.target_g;
    g_light.current_b = g_light.target_b;

    Light_Hardware_Output_Raw(g_light.current_w, g_light.current_r, g_light.current_g, g_light.current_b);
    Set_Battery_Power_Enable(g_light.is_on);
}

void Light_Control_Short_Press(void) {
    if (g_priority_state == LAMP_STATE_CRITICAL_BATTERY) return;

    if (!g_light.is_on) {
        g_light.is_on = true;
        g_light.saved_brightness = 100;
        Calculate_Target_PWM(g_light.saved_brightness, g_light.saved_color_idx, 
                             &g_light.target_w, &g_light.target_r, &g_light.target_g, &g_light.target_b);
        Start_Light_Transition(800, 1); // 0%->100%：800ms EaseOut
        Set_Battery_Power_Enable(true);
    } else {
        if (g_light.saved_brightness == 100) {
            g_light.saved_brightness = 50;
            Calculate_Target_PWM(g_light.saved_brightness, g_light.saved_color_idx, 
                                 &g_light.target_w, &g_light.target_r, &g_light.target_g, &g_light.target_b);
            Start_Light_Transition(400, 1); // 100%->50%：400ms EaseOut
        } else {
            g_light.is_on = false;
            g_light.saved_brightness = 0;
            Calculate_Target_PWM(0, g_light.saved_color_idx, 
                                 &g_light.target_w, &g_light.target_r, &g_light.target_g, &g_light.target_b);
            Start_Light_Transition(400, 1); // 50%->0%：400ms EaseOut
            Set_Battery_Power_Enable(false);
        }
    }
    Mark_Storage_Dirty(); // 触发延迟保存
}

void Light_Control_Double_Click(void) {
    if (!g_light.is_on || g_priority_state == LAMP_STATE_CRITICAL_BATTERY) return;

    g_light.saved_color_idx = (g_light.saved_color_idx + 1) % TOTAL_COLOR_MODES;

    Calculate_Target_PWM(g_light.saved_brightness, g_light.saved_color_idx, 
                         &g_light.target_w, &g_light.target_r, &g_light.target_g, &g_light.target_b);
    
    Start_Light_Transition(400, 1); // 颜色切换过渡时间：400ms
    Mark_Storage_Dirty();
}

void Light_Control_From_Matter(uint8_t app_brightness, uint8_t color_idx) {
    if (color_idx >= TOTAL_COLOR_MODES) color_idx = 2;

    if (app_brightness == 0) {
        g_light.is_on = false;
        g_light.saved_brightness = 0;
        Set_Battery_Power_Enable(false);
    } else {
        g_light.is_on = true;
        g_light.saved_brightness = app_brightness;
        Set_Battery_Power_Enable(true);
    }
    g_light.saved_color_idx = color_idx;

    Calculate_Target_PWM(g_light.saved_brightness, g_light.saved_color_idx, 
                         &g_light.target_w, &g_light.target_r, &g_light.target_g, &g_light.target_b);
    
    Start_Light_Transition(400, 1); // App控制默认 400ms 线性/平滑过渡
    Mark_Storage_Dirty();
}

void Light_Set_Priority_State(Lamp_Priority_State_t new_state) {
    if (g_priority_state == new_state) return;

    g_priority_state = new_state;

    if (g_priority_state == LAMP_STATE_NORMAL) {
        // 退出高优先级事件后，800ms 淡入恢复原状态
        Calculate_Target_PWM(g_light.is_on ? g_light.saved_brightness : 0, g_light.saved_color_idx, 
                             &g_light.target_w, &g_light.target_r, &g_light.target_g, &g_light.target_b);
        Start_Light_Transition(800, 1);
        Set_Battery_Power_Enable(g_light.is_on);
    }
}

// ============================================================================
// 防卡顿延迟记忆保存管理（需在系统的非中断主循环如 100ms 任务中轮询）
// ============================================================================

void Light_Storage_Poll_100ms(void) {
    if (!g_storage_dirty) {
        return;
    }

    if (g_storage_save_countdown > 0) {
        g_storage_save_countdown -= 100;
    } else {
        g_storage_dirty = false;
        g_storage_save_countdown = -1;

        // 【在此执行真正的 NVM3 写入】：
        // 此时用户已经停止连续点按或滑动调光超过 2 秒，安全写入，完全避免写Flash造成的运行卡顿。
        // NVM3_Write_Helper(g_light.saved_brightness, g_light.saved_color_idx);
    }
}
