#include "led_wrgb.h"

led_ctrl_t g_led;

/* 出厂默认 color2（约2450K），亮度100% */
const led_color_t g_color_table[LED_COLOR_COUNT] = {
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

// easeInOutQuad 缓动表：对应 0ms 到 800ms (步长 10ms，共 81 个元素)
// 专门针对嵌入式优化的 Sigmoid 12-bit (0 ~ 4096) 缓动表：对应 0ms 到 800ms (共 81 个元素)
#if 0
static const uint16_t g_ease_quad_table_4096[81] = {
    0,    4,    4,    8,    8,    12,   16,   20,   24,   32,   40,   48,   60,   72,   88,   108,  128,  156,  184,  216,  256,
    300,  348,  400,  460,  524,  600,  676,  764,  852,  952,  1056, 1164, 1280, 1404, 1528, 1660, 1792, 1928, 2068, 2204, 2340,
    2472, 2604, 2728, 2852, 2968, 3076, 3176, 3264, 3352, 3440, 3516, 3592, 3656, 3716, 3768, 3816, 3860, 3900, 3936, 3964, 3988,
    4008, 4024, 4036, 4048, 4056, 4064, 4068, 4072, 4076, 4080, 4084, 4088, 4092, 4092, 4092, 4096, 4096, 4096};
#endif
// 🎯 完美校准版：EaseInOutQuad cubic-bezier(0.45, 0, 0.55, 1) 缓动表
// 对应 0ms 到 800ms (步长 10ms，共 81 个元素)
// 数值严格闭环：第 0 步为 0，第 80 步精准为 4096
static const uint16_t g_ease_quad_table_4096[81] = {
    0,    1,    5,    11,   20,   32,   46,   62,   81,   103,  128,  154,  184,  216,  251,  288,  327,  370,  414,  462,  512,
    564,  620,  677,  737,  800,  865,  933,  1004, 1077, 1152, 1230, 1311, 1394, 1479, 1567, 1658, 1750, 1845, 1943, 2048, 2153,
    2251, 2346, 2438, 2529, 2617, 2702, 2785, 2866, 2944, 3019, 3092, 3163, 3231, 3296, 3359, 3419, 3476, 3532, 3584, 3634, 3682,
    3726, 3769, 3808, 3845, 3880, 3912, 3942, 3968, 3993, 4015, 4034, 4050, 4064, 4076, 4085, 4091, 4095, 4096};

MixedLightingEffects_t g_mixed_effects[5];

bool LED_LoadStateFromFlash(void);
void LED_SaveStateToFlash(void);
// static void     LED_LoadFirstCommissionFlagFromFlash(void);
static uint16_t led_clamp_u16(uint16_t v, uint16_t min, uint16_t max)
{ return (v <= min) ? min : ((v >= max) ? max : v); }
static uint8_t led_clamp_u8(uint8_t v, uint8_t min, uint8_t max)
{ return (v <= min) ? min : ((v >= max) ? max : v); }

bool led_get_status(void)
{ return g_led.is_on; }
void led_set_status(bool on)
{ g_led.is_on = on; }
uint8_t led_get_brightness(void)
{ return g_led.brightness; }
void led_set_brightness(uint8_t brightness)
{ g_led.brightness = brightness; }

led_change_origin_t led_get_change_origin(void)
{ return g_led.change_origin; }
void led_set_change_origin(led_change_origin_t origin)
{ g_led.change_origin = origin; }
uint8_t led_get_color_index(void)
{ return g_led.color_index; }
void led_set_color_index(uint8_t color_index)
{ g_led.color_index = led_clamp_u16(color_index, 0, LED_COLOR_COUNT - 1); }
led_color_source_t led_get_color_source(void)
{ return g_led.color_source; }
void led_set_color_source(led_color_source_t source)
{ g_led.color_source = source; }

led_color_t led_get_color_table(uint8_t color_index)
{ return g_color_table[color_index]; }

led_color_t led_get_raw_color(void)
{ return g_led.raw_color; }

void led_set_raw_color(led_color_t raw_color)
{ g_led.raw_color = raw_color; }

uint8_t led_get_history_brightness(void)
{ return g_led.history_brightness; }

void led_set_history_brightness(uint8_t history_brightness)
{ g_led.history_brightness = history_brightness; }

static void led_apply_output_struct(led_color_t color)
{
    color.w = led_clamp_u16(color.w, 0, LED_HW_MAX);
    color.r = led_clamp_u16(color.r, 0, LED_HW_MAX);
    color.g = led_clamp_u16(color.g, 0, LED_HW_MAX);
    color.b = led_clamp_u16(color.b, 0, LED_HW_MAX);
    g_led.cur_color = color; // 🎯 更新运行时实际输出值
    LED_HW_SetWRGB(color.w, color.r, color.g, color.b);
}

static uint32_t led_map_app_percent_to_output_scale_14bit(uint16_t app_brightness)
{
    if (app_brightness <= 1)
        return 204;
    if (app_brightness >= 255)
        return 10240;
    return 204 + ((uint32_t)(app_brightness - 1) * 10035) / 254;
}

static led_color_t led_scale_color(led_color_t raw_color, uint8_t brightness)
{
    led_color_t target = {0, 0, 0, 0};
    uint32_t    scale_10240 = led_map_app_percent_to_output_scale_14bit(brightness);
    target.w = (uint16_t)(((uint32_t)raw_color.w * scale_10240) >> 14);
    target.r = (uint16_t)(((uint32_t)raw_color.r * scale_10240) >> 14);
    target.g = (uint16_t)(((uint32_t)raw_color.g * scale_10240) >> 14);
    target.b = (uint16_t)(((uint32_t)raw_color.b * scale_10240) >> 14);
    return target;
}

void led_update_normal_state(bool is_on, uint8_t brightness, led_color_t raw_color)
{
    g_led.is_on = is_on;
    g_led.brightness = brightness;
    g_led.raw_color = raw_color;
}

// 🎯 平滑渐变启动：完美配合 start_color 与 target_color
void LED_Start_Fade_To_Current(uint16_t fade_ms)
{
    if (g_led.low_battery_protected)
        return;
    g_led.effect_mode = LED_EFFECT_NONE;
    g_led.start_color = g_led.cur_color;                                                                               // 锁死当前实际输出作为起点
    g_led.target_color = g_led.is_on ? led_scale_color(g_led.raw_color, g_led.brightness) : (led_color_t){0, 0, 0, 0}; // 计算终点
    g_led.fade_start_ms = LED_GetTickMs();
    g_led.fade_time_ms = (fade_ms == 0) ? 1 : fade_ms;
    g_led.fading = true;
}

void LED_StopEffect(void)
{
    g_led.effect_mode = LED_EFFECT_NONE;
    g_led.fading = false;
    LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);
}

void LED_SetBlink(uint8_t brightness, led_color_t raw_color, uint16_t period_ms, uint16_t count)
{
    if (g_led.low_battery_protected)
        return;
    g_led.effect_mode = LED_EFFECT_BLINK;
    g_led.effect_start_ms = LED_GetTickMs();
    g_led.effect_period_ms = (period_ms < 20) ? 20 : period_ms;
    g_led.effect_count = count;
    g_led.fading = false;

    g_led.raw_color = raw_color;
    g_led.brightness = brightness;
}

void LED_SetBreath(uint8_t brightness, led_color_t raw_color, uint16_t count)
{
    if (g_led.low_battery_protected)
    {
        return;
    }
    g_led.effect_mode = LED_EFFECT_BREATH;
    g_led.effect_start_ms = LED_GetTickMs();
    g_led.effect_period_ms = 3200;
    g_led.effect_count = count;
    g_led.fading = false;

    g_led.raw_color = raw_color;
    g_led.brightness = brightness;
}

void LED_SetHold(bool is_on, uint8_t brightness, led_color_t raw_color, uint16_t fade_ms)
{
    g_led.effect_mode = LED_EFFECT_HOLD;
    g_led.effect_start_ms = LED_GetTickMs();
    g_led.effect_period_ms = fade_ms;
    g_led.effect_count = (fade_ms == 0) ? 0 : 1;
    g_led.fading = false;

    led_color_t hold_color = is_on ? led_scale_color(raw_color, brightness) : (led_color_t){0, 0, 0, 0};
    led_apply_output_struct(hold_color);
}

static void led_execute_mixed_index(uint8_t index)
{
    MixedLightingEffects_t *p_effect = &g_mixed_effects[index];
    switch (p_effect->fuction_mode)
    {
    case LED_EFFECT_BLINK:
    {
        LED_SetBlink(p_effect->brightness, p_effect->raw_color, p_effect->fade_ms, p_effect->count);
        break;
    }
    case LED_EFFECT_BREATH:
    {
        LED_SetBreath(p_effect->brightness, p_effect->raw_color, p_effect->count);
        break;
    }
    case LED_EFFECT_HOLD:
    {
        LED_SetHold(p_effect->is_on, p_effect->brightness, p_effect->raw_color, p_effect->fade_ms);
        break;
    }
    default:
        g_led.mix_lighting_effects = false;
        LED_StopEffect();
        break;
    }
}

void led_mixed_lighting_effects_service(void)
{
    if (!g_led.mix_lighting_effects)
        return;
    g_led.mix_effect_index++;
    if (g_led.mix_effect_index >= g_led.mix_effect_end)
    {
        g_led.mix_lighting_effects = false;

        // 🎯 2. 核心改变：备份并清空指针，随后安全安全执行应用层回调
        led_mixed_callback_t cb = g_led.mix_end_callback;
        g_led.mix_end_callback = NULL; // 先清空，防止回调函数内部再次启动特效造成死循环
        if (cb != NULL)
        {
            cb();
        }
        else
        {
            // 💡 如果没有回调函数，启动平滑过渡，恢复到进入特效前备份的默认亮度和开关状态

            // 截取特效最后一帧的硬件实际输出作为渐变起点快照（防突变）
            led_color_t snapshot_start = g_led.cur_color;

            // 恢复系统本身的正常业务数据（开关、亮度、颜色）
            g_led.is_on = g_led.mix_end_is_on;
            g_led.brightness = g_led.mix_end_brightness;
            g_led.raw_color = g_led.mix_end_raw_color;

            // 洗白状态机模式，切回常规线性 Fade
            g_led.effect_mode = LED_EFFECT_NONE;

            // 锁定起点与目标终点，通过防溢出公式平滑切回去
            g_led.start_color = snapshot_start;
            g_led.target_color = g_led.is_on ? led_scale_color(g_led.raw_color, g_led.brightness) : (led_color_t){0, 0, 0, 0};

            // 使用设定的渐变时间开始过渡
            g_led.fade_start_ms = LED_GetTickMs();
            g_led.fade_time_ms = LED_FADE_COLOR_SWITCH_MS;
            g_led.fading = true;
        }
        return;
    }
    led_execute_mixed_index(g_led.mix_effect_index);
}

void Led_MixedLightingEffects_Start(uint8_t effect_end, led_mixed_callback_t callback)
{
    if (g_led.low_battery_protected || effect_end == 0 || effect_end > 5)
        return;
    g_led.mix_effect_index = 0;
    g_led.mix_effect_end = effect_end;
    g_led.mix_lighting_effects = true;
    g_led.mix_end_is_on = g_led.is_on;
    g_led.mix_end_brightness = g_led.brightness;
    g_led.mix_end_raw_color = g_led.raw_color;
    g_led.mix_end_callback = callback;

    led_execute_mixed_index(g_led.mix_effect_index);
}

/**
 * @brief 主动停止并退出当前的混合灯效链
 * @note  干净地清除所有混合标记，并平滑渐变恢复到进入混合灯效前记录的正常状态
 */
void LED_StopMixedEffects(void)
{
    // 如果本来就没在跑混合灯效，直接返回
    if (!g_led.mix_lighting_effects)
    {
        return;
    }

    // 1. 斩断混合灯效标志位，阻止 Tick 状态机继续链式轮询下一步
    g_led.mix_lighting_effects = false;

    // 2. 将全局的正常业务状态，恢复为进入混合特效前备份的原始数据
    g_led.is_on = g_led.mix_end_is_on;
    g_led.brightness = g_led.mix_end_brightness;
    g_led.raw_color = g_led.mix_end_raw_color;

    // 3. 退出特效运行模式
    g_led.effect_mode = LED_EFFECT_NONE;
    g_led.fading = false;

    // 4. 以标准的平滑渐变时间（400ms）过渡回原本的正常常亮/熄灭状态
    LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);
}

// =========================================================================
// 核心周期状态机（10ms 循环）
// =========================================================================
void LED_Tick10ms(void)
{
    if (g_led.low_battery_protected)
        return;

    bool led_change = false;
    bool step_done = false;

    do
    {
        uint32_t now = LED_GetTickMs();
        uint32_t elapsed = now - g_led.effect_start_ms;

        // 1. LED_EFFECT_HOLD: 保持模式
        if (g_led.effect_mode == LED_EFFECT_HOLD)
        {
            if (g_led.effect_count > 0 && elapsed >= g_led.effect_period_ms)
            {
                step_done = true;
            }
            break;
        }

        // 2. LED_EFFECT_BLINK: 闪烁模式
        if (g_led.effect_mode == LED_EFFECT_BLINK)
        {
            if (g_led.effect_count > 0 && elapsed >= ((uint32_t)g_led.effect_count * g_led.effect_period_ms))
            {
                step_done = true;
                break;
            }

            uint32_t current_period_tick = elapsed % g_led.effect_period_ms;
            if (current_period_tick < (g_led.effect_period_ms >> 1))
            {
                g_led.cur_color = led_scale_color(g_led.raw_color, g_led.brightness);
            }
            else
            {
                g_led.cur_color = (led_color_t){0, 0, 0, 0};
            }
            led_change = true;
            break;
        }

        // 3. LED_EFFECT_BREATH: 呼吸模式
        if (g_led.effect_mode == LED_EFFECT_BREATH)
        {
            uint32_t total_breath_cycles = elapsed / g_led.effect_period_ms;
            if (g_led.effect_count > 0 && total_breath_cycles >= g_led.effect_count)
            {
                step_done = true;
                break;
            }

            uint32_t breath_tick = (elapsed % g_led.effect_period_ms) / 10;
            uint16_t factor_4096 = 0;

            if (breath_tick < 80)
                factor_4096 = g_ease_quad_table_4096[breath_tick];
            else if (breath_tick < 160)
                factor_4096 = 4096;
            else if (breath_tick < 240)
                factor_4096 = g_ease_quad_table_4096[240 - breath_tick];
            else
                factor_4096 = 0;

            led_color_t base = led_scale_color(g_led.raw_color, g_led.brightness);
            g_led.cur_color.w = (uint16_t)(((uint32_t)base.w * factor_4096) >> 12);
            g_led.cur_color.r = (uint16_t)(((uint32_t)base.r * factor_4096) >> 12);
            g_led.cur_color.g = (uint16_t)(((uint32_t)base.g * factor_4096) >> 12);
            g_led.cur_color.b = (uint16_t)(((uint32_t)base.b * factor_4096) >> 12);
            led_change = true;
            break;
        }

        // 4. LED_EFFECT_NONE: 常规线性 Fade 渐变（🎯 核心算法完美联动运行期颜色变量）
        if (g_led.effect_mode == LED_EFFECT_NONE && g_led.fading)
        {
            uint32_t fade_elapsed = now - g_led.fade_start_ms;
            if (fade_elapsed >= g_led.fade_time_ms)
            {
                g_led.cur_color = g_led.target_color;
                g_led.fading = false;
            }
            else
            {
                // 严格安全的 32 位有符号平滑渐变算法
                int32_t diff_w = (int32_t)g_led.target_color.w - (int32_t)g_led.start_color.w;
                g_led.cur_color.w = (uint16_t)((int32_t)g_led.start_color.w + (diff_w * (int32_t)fade_elapsed) / (int32_t)g_led.fade_time_ms);

                int32_t diff_r = (int32_t)g_led.target_color.r - (int32_t)g_led.start_color.r;
                g_led.cur_color.r = (uint16_t)((int32_t)g_led.start_color.r + (diff_r * (int32_t)fade_elapsed) / (int32_t)g_led.fade_time_ms);

                int32_t diff_g = (int32_t)g_led.target_color.g - (int32_t)g_led.start_color.g;
                g_led.cur_color.g = (uint16_t)((int32_t)g_led.start_color.g + (diff_g * (int32_t)fade_elapsed) / (int32_t)g_led.fade_time_ms);

                int32_t diff_b = (int32_t)g_led.target_color.b - (int32_t)g_led.start_color.b;
                g_led.cur_color.b = (uint16_t)((int32_t)g_led.start_color.b + (diff_b * (int32_t)fade_elapsed) / (int32_t)g_led.fade_time_ms);
            }
            led_change = true;
            break;
        }
    } while (0);

    if (step_done)
    {
        if (g_led.mix_lighting_effects)
            led_mixed_lighting_effects_service();
        else
            LED_StopEffect();
        return;
    }

    if (led_change)
    {
        led_apply_output_struct(g_led.cur_color);
    }
}

void LED_Init(void)
{
    g_led.is_on = true;
    g_led.history_brightness = g_led.brightness = 100;
    g_led.color_index = 0;
    g_led.change_origin = LED_ORIGIN_UNKNOWN;
    g_led.color_source = LED_SOURCE_INDEX_TABLE;
    g_led.fading = false;
    g_led.low_battery_protected = false;
    g_led.raw_color = g_color_table[0];
    g_led.cur_color = (led_color_t){0, 0, 0, 0}; // 显式置空，等待 Flash 复原

    // LED_LoadFirstCommissionFlagFromFlash();
    LED_LoadStateFromFlash();
    g_led.change_origin = LED_ORIGIN_LOCAL_KEY;
    if (g_led.is_on || g_led.brightness > 0)
    {
        LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);
    }
    else
    {
        g_led.cur_color = (led_color_t){0, 0, 0, 0};
        LED_HW_SetWRGB(0, 0, 0, 0);
    }
}

uint8_t LED_SetLowBatteryProtection(bool protect)
{
    if (g_led.low_battery_protected == protect)
        return 0;

    g_led.low_battery_protected = protect;

    if (g_led.low_battery_protected)
    {
        g_led.effect_mode = LED_EFFECT_NONE;
        g_led.fading = false;
        g_led.cur_color = (led_color_t){0, 0, 0, 0};
        LED_HW_SetWRGB(0, 0, 0, 0);
    }
    else
    {
        if (g_led.is_on)
        {
            g_led.cur_color = (led_color_t){0, 0, 0, 0};
            g_led.start_color = (led_color_t){0, 0, 0, 0};
            LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);
            return 1;
        }
        else
        {
            g_led.cur_color = (led_color_t){0, 0, 0, 0};
            LED_HW_SetWRGB(0, 0, 0, 0);
            return 3;
        }
    }
    return 0;
}

bool LED_LoadStateFromFlash(void)
{
    Ecode_t              err;
    uint32_t             object_type = 0;
    size_t               data_len = sizeof(light_flash_memory_t);
    light_flash_memory_t saved_mem = {0};

    err = nvm3_getObjectInfo(nvm3_defaultHandle, NVM3_KEY_LIGHT_MEMORY_DATA, &object_type, &data_len);
    if (err != ECODE_NVM3_OK || object_type != NVM3_OBJECTTYPE_DATA || data_len != sizeof(light_flash_memory_t))
        return false;

    err = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_LIGHT_MEMORY_DATA, &saved_mem, sizeof(light_flash_memory_t));
    if (err != ECODE_NVM3_OK)
        return false;

    g_led.is_on = saved_mem.is_on;
    g_led.brightness = led_clamp_u8(saved_mem.brightness, 0, 255);
    g_led.color_index = led_clamp_u8(saved_mem.color_index, 0, LED_COLOR_COUNT - 1);
    g_led.color_source = (led_color_source_t)saved_mem.color_source;

    g_led.raw_color.w = led_clamp_u16(saved_mem.custom_w, 0, LED_HW_MAX);
    g_led.raw_color.r = led_clamp_u16(saved_mem.custom_r, 0, LED_HW_MAX);
    g_led.raw_color.g = led_clamp_u16(saved_mem.custom_g, 0, LED_HW_MAX);
    g_led.raw_color.b = led_clamp_u16(saved_mem.custom_b, 0, LED_HW_MAX);
    g_led.history_brightness = g_led.brightness;

    return true;
}

// static void LED_LoadFirstCommissionFlagFromFlash(void)
//{
//     uint8_t flag = 0;
//
//     Ecode_t err = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_FIRST_COMMISSION_DONE, &flag, sizeof(flag));
//     s_first_commission_done = (err == ECODE_NVM3_OK && flag == FIRST_COMMISSION_DONE_MAGIC);
// }
//
// bool LED_IsFirstCommissionDone(void)
//{
//     return s_first_commission_done;
// }
//
// void LED_SetFirstCommissionDone(void)
//{
//     if (s_first_commission_done)
//     {
//         return;
//     }
//
//     s_first_commission_done = true;
//     uint8_t flag = FIRST_COMMISSION_DONE_MAGIC;
//     Ecode_t err = nvm3_writeData(nvm3_defaultHandle, NVM3_KEY_FIRST_COMMISSION_DONE, &flag, sizeof(flag));
//     if (err == ECODE_NVM3_OK && nvm3_repackNeeded(nvm3_defaultHandle))
//     {
//         nvm3_repack(nvm3_defaultHandle);
//     }
// }
//
// bool LED_IsUserEffectIdle(void)
//{
//     return !g_led.mix_lighting_effects && g_led.effect_mode == LED_EFFECT_NONE && !g_led.fading;
// }

void LED_SaveStateToFlash(void)
{
    Ecode_t              err;
    light_flash_memory_t current_mem = {0};
    light_flash_memory_t old_mem = {0};
    uint32_t             object_type = 0;
    size_t               data_len = sizeof(light_flash_memory_t);

    current_mem.is_on = g_led.is_on;
    current_mem.brightness = g_led.brightness;
    current_mem.color_index = g_led.color_index;
    current_mem.color_source = (uint8_t)g_led.color_source;
    current_mem.custom_w = g_led.raw_color.w;
    current_mem.custom_r = g_led.raw_color.r;
    current_mem.custom_g = g_led.raw_color.g;
    current_mem.custom_b = g_led.raw_color.b;

    err = nvm3_getObjectInfo(nvm3_defaultHandle, NVM3_KEY_LIGHT_MEMORY_DATA, &object_type, &data_len);
    if (err == ECODE_NVM3_OK && object_type == NVM3_OBJECTTYPE_DATA && data_len != sizeof(light_flash_memory_t))
    {
        nvm3_deleteObject(nvm3_defaultHandle, NVM3_KEY_LIGHT_MEMORY_DATA);
    }

    if (err == ECODE_NVM3_OK && object_type == NVM3_OBJECTTYPE_DATA && data_len == sizeof(light_flash_memory_t))
    {
        err = nvm3_readData(nvm3_defaultHandle, NVM3_KEY_LIGHT_MEMORY_DATA, &old_mem, sizeof(light_flash_memory_t));
        if (err == ECODE_NVM3_OK && memcmp(&current_mem, &old_mem, sizeof(light_flash_memory_t)) == 0)
            return;
    }

    err = nvm3_writeData(nvm3_defaultHandle, NVM3_KEY_LIGHT_MEMORY_DATA, &current_mem, sizeof(light_flash_memory_t));
    if (err == ECODE_NVM3_OK && nvm3_repackNeeded(nvm3_defaultHandle))
    {
        nvm3_repack(nvm3_defaultHandle);
    }
}
