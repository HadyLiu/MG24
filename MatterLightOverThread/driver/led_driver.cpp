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
static const uint16_t g_sin_table_1023[29] = {0,   50,  100, 150, 200, 249, 297, 345, 392, 438, 483, 527,  570,  611, 652,
                                              691, 729, 765, 800, 833, 865, 895, 923, 949, 972, 993, 1011, 1021, 1023};
#define SIN_TABLE_SIZE 29 //(sizeof(g_sin_table_1023) / sizeof(g_sin_table_1023[0]))

MixedLightingEffects_t g_mixed_effects[5];

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
    target.w = (uint16_t)(((uint32_t)g_color_table[color_index].w * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240
    target.r = (uint16_t)(((uint32_t)g_color_table[color_index].r * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240
    target.g = (uint16_t)(((uint32_t)g_color_table[color_index].g * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240
    target.b = (uint16_t)(((uint32_t)g_color_table[color_index].b * scale_10240) >> 14); // 右移14位相当于除以10000再乘以10240

    return target;
}

/**
 * @brief 从逻辑状态计算目标颜色值，并启动渐变
 * @param is_on 逻辑开关状态
 * @param brightness_percent 亮度百分比 1~100
 * @param color_index 颜色索引 0~12
 * @param fade_ms 渐变时间（毫秒）
 */
void LED_Start_Fade_Logic(bool is_on, uint8_t brightness_percent, uint8_t color_index, uint16_t fade_ms)
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
void LED_SetBlink(uint8_t brightness, uint8_t color_index, uint16_t period_ms, uint16_t count)
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
void LED_SetBreath(uint8_t brightness, uint8_t color_index, uint16_t period_ms, uint16_t count)
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

///**
// * @brief 设定保持模式（直接保持在目标状态，无闪烁或呼吸效果）
// * @param is_on 目标开关状态
// * @param fade_ms 渐变时间（毫秒），如果为0则立即切换
// */
// void LED_SetHold(bool is_on, uint16_t fade_ms)
//{
//    led_color_t target = {0, 0, 0, 0};
//    if (!is_on)
//    {
//        return;
//    }
//    g_led.is_on = is_on;
//    g_led.effect_start_ms = LED_GetTickMs();
//    g_led.effect_period_ms = fade_ms;
//    g_led.effect_mode = LED_EFFECT_HOLD;
//    g_led.fading = false;
//
//    g_led.effect_count = (fade_ms == 0) ? 0 : 1; // 如果 fade_ms 为0则无限循环，否则只运行1次
//
//    // 立即应用当前颜色
//    g_led.cur_color = led_calc_target_from_logic_struct(g_led.is_on, g_led.brightness, g_led.color_index);
//    led_apply_output_struct(g_led.cur_color);
//}

/**
 * @brief 设定保持模式（直接保持在目标状态，无闪烁或呼吸效果）
 * @param is_on 目标开关状态
 * @param brightness 目标亮度 (1~100)
 * @param color_index 目标颜色索引 (0~12)
 * @param fade_ms 保持的时间（毫秒），如果为0则代表无限保持
 */
void LED_SetHold(bool is_on, uint8_t brightness, uint8_t color_index, uint16_t fade_ms)
{
    if (!is_on)
    {
        // 如果是关灯保持，直接清零输出
        g_led.is_on = false;
        g_led.effect_mode = LED_EFFECT_HOLD;
        g_led.effect_start_ms = LED_GetTickMs();
        g_led.effect_period_ms = fade_ms;
        g_led.effect_count = (fade_ms == 0) ? 0 : 1;
        g_led.fading = false;

        led_color_t dark_color = {0, 0, 0, 0};
        led_apply_output_struct(dark_color);
        return;
    }

    // 限幅保护
    brightness = led_clamp_u8(brightness, 1, 100);
    color_index = led_clamp_u8(color_index, 0, LED_COLOR_COUNT - 1);

    // 🎯 更新灯效运行期间的临时上下文状态
    g_led.is_on = is_on;
    g_led.brightness = brightness;
    g_led.color_index = color_index;

    g_led.effect_mode = LED_EFFECT_HOLD;
    g_led.effect_start_ms = LED_GetTickMs();
    g_led.effect_period_ms = fade_ms;
    g_led.effect_count = (fade_ms == 0) ? 0 : 1;
    g_led.fading = false;

    // 立即计算并应用当前步骤配置的专属颜色与亮度
    g_led.cur_color = led_calc_target_from_logic_struct(is_on, brightness, color_index);
    led_apply_output_struct(g_led.cur_color);
}

static void led_execute_mixed_index(uint8_t index)
{
    uint8_t  mode = g_mixed_effects[index].fuction_mode;
    uint8_t  bri = g_mixed_effects[index].brightness;
    uint8_t  color = g_mixed_effects[index].color_index;
    uint16_t ms = g_mixed_effects[index].fade_ms;
    uint16_t cnt = g_mixed_effects[index].count;
    bool     on = g_mixed_effects[index].is_on;

    if (mode == LED_EFFECT_BLINK) // 闪烁 (对应 LED_EFFECT_BLINK 逻辑)
    {
        LED_SetBlink(bri, color, ms, cnt);
    }
    else if (mode == LED_EFFECT_BREATH) // 呼吸 (对应 LED_EFFECT_BREATH 逻辑)
    {
        LED_SetBreath(bri, color, ms, cnt);
    }
    else if (mode == LED_EFFECT_HOLD) // 保持 (对应 LED_EFFECT_HOLD 逻辑)
    {
        LED_SetHold(on, bri, color, ms);
    }
    else // 无特效 / 结束
    {
        // 如果混合特效中配置了 0 (NONE)，直接视为本段特效结束，切回常规常规
        g_led.mix_lighting_effects = false;
        LED_Start_Fade_Logic(g_led.is_on, g_led.brightness, g_led.color_index, LED_FADE_COLOR_SWITCH_MS);
    }
}

void Led_MixedLightingEffects_Start(uint8_t effect_end, bool is_on, uint8_t brightness, uint8_t color_index)
{
    if (effect_end == 0 || effect_end > 5)
    {
        return;
    }
    g_led.mix_effect_index = 0;
    g_led.mix_effect_end = effect_end;
    g_led.mix_lighting_effects = true; // 开启混合特效模式
    g_led.mix_end_is_on = is_on;
    g_led.mix_end_brightness = brightness;
    g_led.mix_end_color_index = color_index;

    led_execute_mixed_index(g_led.mix_effect_index);
}

/**
 * @brief 终止混合特效，立即切回常规状态
 */
void LED_StopMixedEffects(bool is_on, uint8_t brightness, uint8_t color_index)
{
    // 1. 关闭混合灯效使能开关
    g_led.mix_lighting_effects = false;
    g_led.mix_effect_index = 0;
    g_led.mix_effect_end = 0;

    // 2. 终止特效状态机，停止当前的闪烁或呼吸
    g_led.effect_mode = LED_EFFECT_NONE;
    g_led.effect_count = 0;
    g_led.effect_period_ms = 0;

    // 3. 重新平滑地过渡回当前的色温/常规颜色 (例如使用 400ms 渐变时间)
    // g_led.is_on, g_led.brightness, g_led.color_index 存储的是设备常规下的状态
    LED_Start_Fade_Logic(is_on, brightness, color_index, LED_FADE_COLOR_SWITCH_MS);
}

void led_mixed_lighting_effects_service(void)
{
    if (!g_led.mix_lighting_effects)
    {
        return;
    }
    g_led.mix_effect_index++;
    // 如果已经播放完所有指定的特效步骤
    if (g_led.mix_effect_index >= g_led.mix_effect_end)
    {
        g_led.mix_effect_index = 0;
        g_led.mix_lighting_effects = false;

        // 播放完毕，自然过渡回当前用户设定的常规状态
        g_led.is_on = g_led.mix_end_is_on;
        g_led.brightness = g_led.mix_end_brightness;
        g_led.color_index = g_led.mix_end_color_index;
        LED_Start_Fade_Logic(g_led.is_on, g_led.brightness, g_led.color_index, LED_FADE_COLOR_SWITCH_MS);
        return;
    }

    // 执行下一步效果
    led_execute_mixed_index(g_led.mix_effect_index);
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

    // led_color_t target_color = led_calc_target_from_logic_struct(is_on, brightness, color_index);
    // led_apply_output_struct(target_color);
    LED_Start_Fade_Logic(is_on, brightness, color_index, LED_FADE_COLOR_SWITCH_MS); // 上电时使用渐变效果切到目标状态
}

/**
 * @brief LED状态机主循环，建议每10ms调用一次
 */
void LED_Tick10ms(void)
{
    bool led_change = false;
    do
    {
        // -----------------------------------------------------------------
        // 特效灯效整数状态机
        // -----------------------------------------------------------------
        if (g_led.effect_mode != LED_EFFECT_NONE)
        {
            uint32_t now = LED_GetTickMs();
            uint32_t elapsed = now - g_led.effect_start_ms;

            // 1. 判断当前单步特效是否彻底完结
            bool effect_done = false;
            if (g_led.effect_mode == LED_EFFECT_HOLD)
            {
                // HOLD 模式: 到了指定的超时时间就算结束
                if (g_led.effect_count > 0 && elapsed >= g_led.effect_period_ms)
                {
                    effect_done = true;
                }
            }
            else // BLINK 或 BREATH 模式
            {
                // 🎯 核心改进：基于总耗时进行精准边界对齐，消除 Tick 10ms 带来的截断闪烁
                if (g_led.effect_count > 0)
                {
                    uint32_t target_total_ms = (uint32_t)g_led.effect_count * g_led.effect_period_ms;
                    if (elapsed >= target_total_ms)
                    {
                        effect_done = true;
                    }
                }
            }

            // 2. 特效结束后，驱动路由跳转
            if (effect_done)
            {
                if (g_led.mix_lighting_effects == true)
                {
                    led_mixed_lighting_effects_service(); // 步进切换至下一个特效
                }
                else
                {
                    LED_StopEffect(); // 独立运行结束
                }
                break;
            }

            // 3. 运行中特效刷新色彩波形
            if (g_led.effect_mode == LED_EFFECT_BLINK)
            {
                uint32_t ms_in_cycle = elapsed % g_led.effect_period_ms;
                if (ms_in_cycle < (g_led.effect_period_ms >> 1))
                {
                    g_led.cur_color = led_calc_target_from_logic_struct(true, g_led.brightness, g_led.color_index);
                }
                else
                {
                    led_color_t dark_color = {0, 0, 0, 0};
                    g_led.cur_color = dark_color;
                }
                led_change = true;
            }
            else if (g_led.effect_mode == LED_EFFECT_BREATH)
            {
                uint32_t ms_in_cycle = elapsed % g_led.effect_period_ms;
                uint16_t breath_scale_1023 = get_breath_scale_1023(ms_in_cycle, g_led.effect_period_ms);
                uint8_t  bri = led_clamp_u8(g_led.brightness, 1, 100);
                uint32_t out_scale_14bit = led_map_app_percent_to_output_scale_14bit(bri);
                uint64_t total_scale = (uint64_t)out_scale_14bit * breath_scale_1023;

                g_led.cur_color.w = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].w * total_scale) >> 24);
                g_led.cur_color.r = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].r * total_scale) >> 24);
                g_led.cur_color.g = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].g * total_scale) >> 24);
                g_led.cur_color.b = (uint16_t)(((uint64_t)g_color_table[g_led.color_index].b * total_scale) >> 24);
                led_change = true;
            }
            else if (g_led.effect_mode == LED_EFFECT_HOLD)
            {
                // HOLD 模式只需等时间自然截止，运行期间不重刷颜色，避免闪烁
            }
            break;
        }

        // -----------------------------------------------------------------
        // 常规 Fading 线性渐变逻辑（常规无特效控制）
        // -----------------------------------------------------------------
        if (g_led.fading)
        {
            uint32_t now = LED_GetTickMs();
            uint32_t elapsed = now - g_led.fade_start_ms;

            if (elapsed >= g_led.fade_time_ms)
            {
                led_change = true;
                g_led.cur_color = g_led.target_color;
                g_led.fading = false;
                break;
            }

            led_color_t intermediate_color;
            intermediate_color.w =
                g_led.start_color.w + (int32_t)(g_led.target_color.w - g_led.start_color.w) * (int32_t)elapsed / (int32_t)g_led.fade_time_ms;
            intermediate_color.r =
                g_led.start_color.r + (int32_t)(g_led.target_color.r - g_led.start_color.r) * (int32_t)elapsed / (int32_t)g_led.fade_time_ms;
            intermediate_color.g =
                g_led.start_color.g + (int32_t)(g_led.target_color.g - g_led.start_color.g) * (int32_t)elapsed / (int32_t)g_led.fade_time_ms;
            intermediate_color.b =
                g_led.start_color.b + (int32_t)(g_led.target_color.b - g_led.start_color.b) * (int32_t)elapsed / (int32_t)g_led.fade_time_ms;
            g_led.cur_color = intermediate_color;
            led_change = true;
            break;
        }
    } while (0);

    if (led_change)
    {
        led_apply_output_struct(g_led.cur_color);
    }
}