#include "led_driver.h"

#define LED_FADE_KEY_100_TO_50_MS 400
#define LED_FADE_KEY_50_TO_0_MS   400
#define LED_FADE_KEY_0_TO_100_MS  800
#define LED_FADE_COLOR_SWITCH_MS  400

led_ctrl_t g_led;

/* 出厂默认 color2（约2450K），亮度100% */
static const led_color_t g_color_table[LED_COLOR_COUNT] = {
    //   W,    R,    G,    B
    {1023, 0, 0, 0},     // color0:  W100% R0%   G0%   B0%
    {409, 1023, 0, 0},   // color1:  W40%  R100% G0%   B0%
    {409, 0, 235, 563},  // color2:  W40%  R0%   G23%  B55%
    {327, 0, 235, 1023}, // color3:  W32%  R0%   G23%  B100%
    {0, 1023, 179, 0},   // color4:  W0%   R100% G17.5% B0%
    {0, 1023, 102, 0},   // color5:  W0%   R100% G10%  B0%
    {0, 1023, 0, 0},     // color6:  W0%   R100% G0%   B0%
    {0, 1023, 102, 59},  // color7:  W0%   R100% G10.0% B5.8%
    {0, 1023, 240, 404}, // color8:  W0%   R100% G23.5% B39.5%
    {0, 240, 240, 1023}, // color9:  W0%   R23.5% G23.5% B100%
    {0, 522, 844, 522},  // color10: W0%   R51%  G82.5% B51%
    {0, 522, 1023, 159}, // color11: W0%   R51%  G100% B15.5%
    {0, 1023, 322, 0}    // color12: W0%   R100% G31.5% B0%
};

// 🎯 呼吸灯正弦波全整数查表（1/4周期正弦，放大1023倍，对应 0~90 度）
// 范围从 0（灭） 到 1023（全亮）
static const uint16_t g_sin_table_1023[29] = {0,   50,  100, 150, 200, 249, 297,  345,  392, 438,
                                              483, 527, 570, 611, 652, 691, 729,  765,  800, 833,
                                              865, 895, 923, 949, 972, 993, 1011, 1021, 1023};
#define SIN_TABLE_SIZE 29 //(sizeof(g_sin_table_1023) / sizeof(g_sin_table_1023[0]))

static uint8_t led_clamp_u8(uint8_t v, uint8_t min, uint8_t max)
{
    if (v <= min)
    {
        return min;
    }

    if (v >= max)
    {
        return max;
    }
    return v;
}

static uint16_t led_clamp_u16(uint16_t v, uint16_t min, uint16_t max)
{
    if (v <= min)
    {
        return min;
    }
    if (v >= max)
    {
        return max;
    }
    return v;
}

/**
 * @brief 应用结构体 WRGB 输出到硬件
 */
static void led_apply_output_struct(led_color_t color)
{
    color.w = led_clamp_u16(color.w, 0, LED_HW_MAX);
    color.r = led_clamp_u16(color.r, 0, LED_HW_MAX);
    color.g = led_clamp_u16(color.g, 0, LED_HW_MAX);
    color.b = led_clamp_u16(color.b, 0, LED_HW_MAX);

    g_led.cur_color = color;
    LED_HW_SetWRGB(color.w, color.r, color.g, color.b);
}

/*
 * 🎯 纯整数亮度映射：App亮度 1~100% -> 实际输出比例 10240
 * 原公式: out_percent = 2.0 + (app - 1) * 98.0 / 99.0
 * 放大10000倍后变成整数运算，完全避免小数
 */
static uint32_t led_map_app_percent_to_output_scale_14bit(uint16_t app_percent)
{
    if (app_percent <= 1)
    {
        return 204; // 2% * 10240
    }
    if (app_percent >= 100)
    {
        return 10240; // 100% * 10240
    }

    // 纯整数算术公式（先乘后除防止丢精度）
    return 204 + ((uint32_t)(app_percent - 1) * 10035) / 99;
}

/**
 * @brief 纯整数逻辑状态目标值计算
 * @param is_on 逻辑开关状态
 * @param brightness_percent 亮度百分比 1~100
 * @param color_index 颜色索引 0~12
 * @return 计算得到的目标颜色值（已应用亮度缩放）
 */
static led_color_t led_calc_target_from_logic_struct(bool is_on, uint8_t brightness_percent, uint8_t color_index)
{
    led_color_t target = {0, 0, 0, 0};
    if (!is_on)
    {
        return target;
    }
    brightness_percent = led_clamp_u8(brightness_percent, 1, 100);
    color_index = led_clamp_u8(color_index, 0, LED_COLOR_COUNT - 1);

    // 拿到放大 10000 倍的亮度比例
    uint32_t scale_10240 = led_map_app_percent_to_output_scale_14bit(brightness_percent);

    // 纯整数等比缩放
    // 缩放公式：target = (原始值 * scale_10240) >> 14
    target.w =
        (uint16_t)(((uint32_t)g_color_table[color_index].w * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240
    target.r =
        (uint16_t)(((uint32_t)g_color_table[color_index].r * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240
    target.g =
        (uint16_t)(((uint32_t)g_color_table[color_index].g * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240
    target.b =
        (uint16_t)(((uint32_t)g_color_table[color_index].b * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240

    return target;
}

/**
 * @brief 从逻辑状态计算目标颜色值，并启动渐变
 * @param is_on 逻辑开关状态
 * @param brightness_percent 亮度百分比 1~100
 * @param color_index 颜色索引 0~12
 * @param fade_ms 渐变时间（毫秒）
 */
void LED_Start_Fade_Logic(bool is_on, uint8_t brightness_percent, uint8_t color_index, uint32_t fade_ms)
{
    g_led.effect_mode = LED_EFFECT_NONE;
    brightness_percent = led_clamp_u8(brightness_percent, 1, 100);
    color_index = color_index % LED_COLOR_COUNT;

    g_led.is_on = is_on;
    g_led.brightness = brightness_percent;
    g_led.color_index = color_index;

    g_led.start_color = g_led.cur_color;
    g_led.target_color = led_calc_target_from_logic_struct(g_led.is_on, g_led.brightness, g_led.color_index);

    g_led.fade_start_ms = LED_GetTickMs();
    g_led.fade_time_ms = (fade_ms == 0) ? 1 : fade_ms;
    g_led.fading = true;
}

/**
 * @brief 纯整数获取当前呼吸周期的放大比例
 * @return 0 ~ 1023 之间的整数比例值
 */
static uint16_t get_breath_scale_1023(uint32_t ms_in_cycle, uint32_t period_ms)
{
    // 将整个周期划分为 4 个象限，映射到查表索引
    // 总索引跨度 = (SIN_TABLE_SIZE - 1) * 4
    uint32_t total_steps = (SIN_TABLE_SIZE - 1) << 2; // 等同于 (SIN_TABLE_SIZE - 1) * 4
    uint32_t step = (ms_in_cycle * total_steps) / period_ms;

    uint32_t quadrant = step / (SIN_TABLE_SIZE - 1);
    uint32_t index = step % (SIN_TABLE_SIZE - 1);

    uint16_t value = 0;
    switch (quadrant)
    {
    case 0:
        value = g_sin_table_1023[index]; // 第一象限：0 -> 1024 (吸气变亮)
        break;
    case 1:
        value = g_sin_table_1023[SIN_TABLE_SIZE - 1 - index]; // 第二象限：1024 -> 0 (呼气变暗)
        break;
    case 2:
        value = g_sin_table_1023[index]; // 第三象限：0 -> 1024 (吸气变亮)
        break;
    default:
        value = g_sin_table_1023[SIN_TABLE_SIZE - 1 - index]; // 第四象限：1024 -> 0 (呼气变暗)
        break;
    }
    return value;
}

void LED_StopEffect(void)
{
    g_led.effect_mode = LED_EFFECT_NONE;
    LED_Start_Fade_Logic(g_led.is_on, g_led.brightness, g_led.color_index, LED_FADE_COLOR_SWITCH_MS);
}

/**
 * @brief 设定闪烁模式
 * @param period_ms 闪烁周期（毫秒），建议不小于20ms以避免过快闪烁
 * @param count 闪烁次数，0代表无限循环
 */
void LED_SetBlink(uint8_t brightness, uint8_t color_index, uint32_t period_ms, uint32_t count)
{
    if (period_ms < 20)
    {
        period_ms = 20;
    }
    brightness = led_clamp_u8(brightness, 1, 100);
    color_index = led_clamp_u8(color_index, 0, LED_COLOR_COUNT - 1);
    g_led.brightness = brightness;
    g_led.color_index = color_index; // 维持当前亮度和颜色
    g_led.effect_mode = LED_EFFECT_BLINK;
    g_led.effect_start_ms = LED_GetTickMs();
    g_led.effect_period_ms = period_ms;
    g_led.effect_count = count;
    g_led.fading = false;
}

/**
 * @brief 设定呼吸模式
 * @param period_ms 呼吸周期（毫秒），建议不小于100ms以避免过快变化
 * @param count 呼吸次数，0代表无限循环
 */
void LED_SetBreath(uint8_t brightness, uint8_t color_index, uint32_t period_ms, uint32_t count)
{
    if (period_ms < 100)
    {
        period_ms = 100;
    }
    brightness = led_clamp_u8(brightness, 1, 100);
    color_index = led_clamp_u8(color_index, 0, LED_COLOR_COUNT - 1);

    g_led.brightness = brightness;
    g_led.color_index = color_index;
    g_led.effect_mode = LED_EFFECT_BREATH;
    g_led.effect_start_ms = LED_GetTickMs();
    g_led.effect_period_ms = period_ms;
    g_led.effect_count = count;
    g_led.fading = false;
}

/**
 * @brief 设定保持模式（直接保持在目标状态，无闪烁或呼吸效果）
 * @param is_on 目标开关状态
 * @param fade_ms 渐变时间（毫秒），如果为0则立即切换
 */
void LED_SetHold(bool is_on, uint32_t fade_ms)
{
    led_color_t target = {0, 0, 0, 0};
    if (!is_on)
    {
        return;
    }
    g_led.hold_is_on = is_on;
    g_led.effect_start_ms = LED_GetTickMs();
    g_led.effect_period_ms = fade_ms;
    LED_StopEffect(); // 停止任何正在运行的特效，直接应用新的状态
}

/**
 * @brief 初始化LED驱动
 */
void LED_Init(void)
{
    uint8_t brightness = 100;
    uint8_t color_index = 0;
    bool    is_on = true;

    // 获取记忆中的数据
    // LED_LoadState(&brightness, &color_index, &is_on);

    brightness = led_clamp_u8(brightness, 1, 100);
    color_index = led_clamp_u8(color_index, 0, LED_COLOR_COUNT - 1);

    g_led.is_on = is_on;
    g_led.brightness = brightness;
    g_led.color_index = color_index;
    g_led.key_next_off = false;
    g_led.fading = false;

    //  led_sync_key_level_from_brightness(is_on, brightness);

    led_color_t target_color = led_calc_target_from_logic_struct(is_on, brightness, color_index);
    led_apply_output_struct(target_color);
}

void LED_Tick10ms(void)
{
    // 🎯 统一使用一个标志位来判断是否需要更新硬件输出，避免在函数中多处调用 led_apply_output_struct 导致的重复输出
    bool led_change = false;
    do
    {
        // 特效灯效纯整数状态机
        if (g_led.effect_mode != LED_EFFECT_NONE)
        {
            uint32_t now = LED_GetTickMs();
            uint32_t elapsed = now - g_led.effect_start_ms;
            uint32_t current_cycle = elapsed / g_led.effect_period_ms;

            if (g_led.effect_count > 0 && current_cycle >= g_led.effect_count)
            {
                LED_StopEffect();
                break; // 🎯 代替 return，直接跳出 do-while 统一出口
            }
            // 保持逻辑
            if (g_led.effect_mode == LED_EFFECT_HOLD)
            {
                // 保持模式直接应用目标颜色，无需额外逻辑
                if (g_led.effect_period_ms == 0)
                {
                    break;
                }
                else if (g_led.effect_period_ms - elapsed >= 0)
                {
                    if (g_led.hold_is_on == true)
                    {
                        led_color_t hold_color =
                            led_calc_target_from_logic_struct(true, g_led.brightness, g_led.color_index);
                        g_led.cur_color = hold_color;
                    }
                    else
                    {
                        led_color_t hold_color = {0, 0, 0, 0};
                        g_led.cur_color = hold_color;
                    }
                    break;
                }
            }

            if (g_led.effect_mode == LED_EFFECT_BLINK)
            {
                uint32_t ms_in_cycle = elapsed % g_led.effect_period_ms;
                if (ms_in_cycle < (g_led.effect_period_ms >> 1))
                {
                    led_color_t active_color =
                        led_calc_target_from_logic_struct(true, g_led.brightness, g_led.color_index);
                    led_change = true;
                    g_led.cur_color = active_color;
                }
                else
                {
                    led_color_t dark_color = {0, 0, 0, 0};
                    led_change = true;
                    g_led.cur_color = dark_color;
                }
            }
            else if (g_led.effect_mode == LED_EFFECT_BREATH)
            {
                uint32_t ms_in_cycle = elapsed % g_led.effect_period_ms;

                // 通过纯整数查表拿到 0~1023 比例
                uint16_t breath_scale_1023 = get_breath_scale_1023(ms_in_cycle, g_led.effect_period_ms);

                uint8_t  bri = led_clamp_u8(g_led.brightness, 1, 100);
                uint32_t out_scale_14bit = led_map_app_percent_to_output_scale_14bit(bri);

                // 混合后的纯整数最终比例分母：10000 * 1024 = 10240000
                uint64_t total_scale = (uint64_t)out_scale_14bit * breath_scale_1023;

                // 实际需要>>24
                led_color_t breath_color;
                breath_color.w = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].w * total_scale) >> 24);
                breath_color.r = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].r * total_scale) >> 24);
                breath_color.g = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].g * total_scale) >> 24);
                breath_color.b = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].b * total_scale) >> 24);
                g_led.cur_color = breath_color; // 更新当前颜色状态，保持数据一致性
                led_change = true;
            }
            break; // 🎯 特效运行时，拦截后续常规渐变，直接 break 出去
        }

        // -----------------------------------------------------------------
        // 常规 Fading 线性渐变逻辑
        // -----------------------------------------------------------------
        if (g_led.fading)
        {
            uint32_t now = LED_GetTickMs();
            uint32_t elapsed = now - g_led.fade_start_ms;

            if (elapsed >= g_led.fade_time_ms)
            {
                // 渐变结束，直接设置到目标颜色
                led_change = true;
                g_led.cur_color = g_led.target_color;
                g_led.fading = false;
                break;
            }

            // 纯整数线性插值：避免出现百分比小数 t，先乘 elapsed 后除 fade_time_ms
            led_color_t intermediate_color;
            intermediate_color.w = g_led.start_color.w
                                   + (int32_t)(g_led.target_color.w - g_led.start_color.w) * (int32_t)elapsed
                                         / (int32_t)g_led.fade_time_ms;
            intermediate_color.r = g_led.start_color.r
                                   + (int32_t)(g_led.target_color.r - g_led.start_color.r) * (int32_t)elapsed
                                         / (int32_t)g_led.fade_time_ms;
            intermediate_color.g = g_led.start_color.g
                                   + (int32_t)(g_led.target_color.g - g_led.start_color.g) * (int32_t)elapsed
                                         / (int32_t)g_led.fade_time_ms;
            intermediate_color.b = g_led.start_color.b
                                   + (int32_t)(g_led.target_color.b - g_led.start_color.b) * (int32_t)elapsed
                                         / (int32_t)g_led.fade_time_ms;
            g_led.cur_color = intermediate_color; // 更新当前颜色状态，保持数据一致性
            led_change = true;
            break;
        }
    } while (0);

    // 统一应用当前颜色状态到硬件
    if (led_change)
    {
        led_apply_output_struct(g_led.cur_color);
    }
}
