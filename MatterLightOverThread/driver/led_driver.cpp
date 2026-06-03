#include "led_driver.h"

#define LED_FADE_KEY_100_TO_50_MS    400
#define LED_FADE_KEY_50_TO_0_MS      400
#define LED_FADE_KEY_0_TO_100_MS     800
#define LED_FADE_COLOR_SWITCH_MS     400

#define LED_HW_MAX                   1000   // 最终输出给硬件的最大值，可改成PWM满量程

led_ctrl_t g_led;

/* 出厂默认 color2（约2450K），亮度100% */
static const led_color_t g_color_table[LED_COLOR_COUNT] =
{
    {100,   0,   0,   0},   // color0
    { 40, 100,   0,   0},   // color1
    { 40,   0,  23,  55},   // color2
    { 32,   0,  23, 100},   // color3
    {  0, 100,  18,   0},   // color4
    {  0, 100,  10,   0},   // color5
    {  0, 100,   0,   0},   // color6
    {  0, 100,  10,   6},   // color7
    {  0, 100,  24,  40},   // color8
    {  0,  24,  24, 100},   // color9
    {  0,  51,  83,  51},   // color10
    {  0,  51, 100,  16},   // color11
    {  0, 100,  32,   0},   // color12
};

//static void led_sync_key_level_from_brightness(bool is_on, uint8_t brightness);

static uint8_t led_clamp_u8(uint8_t v, uint8_t min, uint8_t max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static float led_clamp_f(float v, float min, float max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

/*
 * App亮度 1~100%
 * 实际输出限制 2%~100%
 * 且保证从2%到100%线性调节
 *
 * 例如：
 * app=1   -> out=2
 * app=100 -> out=100
 *
 * out = 2 + (app-1)*(98/99)
 */
static float led_map_app_percent_to_output_percent(uint8_t app_percent)
{
    if (app_percent <= 1)
        return 2.0f;
    if (app_percent >= 100)
        return 100.0f;

    return 2.0f + ((float)(app_percent - 1) * 98.0f / 99.0f);
}

static void led_apply_output(float w, float r, float g, float b)
{
    w = led_clamp_f(w, 0.0f, 100.0f);
    r = led_clamp_f(r, 0.0f, 100.0f);
    g = led_clamp_f(g, 0.0f, 100.0f);
    b = led_clamp_f(b, 0.0f, 100.0f);

    g_led.cur_w = w;
    g_led.cur_r = r;
    g_led.cur_g = g;
    g_led.cur_b = b;

    uint16_t hw_w = (uint16_t)(w * LED_HW_MAX / 100.0f + 0.5f);
    uint16_t hw_r = (uint16_t)(r * LED_HW_MAX / 100.0f + 0.5f);
    uint16_t hw_g = (uint16_t)(g * LED_HW_MAX / 100.0f + 0.5f);
    uint16_t hw_b = (uint16_t)(b * LED_HW_MAX / 100.0f + 0.5f);

    LED_HW_SetWRGB(hw_w, hw_r, hw_g, hw_b);
}

void led_calc_target_from_logic(bool is_on, uint8_t brightness_percent, uint8_t color_index,
                                       float *w, float *r, float *g, float *b)
{
    if (!is_on)
    {
        *w = 0;
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    brightness_percent = led_clamp_u8(brightness_percent, 1, 100);
    color_index = color_index % LED_COLOR_COUNT;

    float out_percent = led_map_app_percent_to_output_percent(brightness_percent);
    float scale = out_percent / 100.0f;

    *w = g_color_table[color_index].w * scale;
    *r = g_color_table[color_index].r * scale;
    *g = g_color_table[color_index].g * scale;
    *b = g_color_table[color_index].b * scale;
}

void led_start_fade_to_logic(bool is_on, uint8_t brightness_percent, uint8_t color_index, uint32_t fade_ms)
{
    brightness_percent = led_clamp_u8(brightness_percent, 1, 100);
    color_index = color_index % LED_COLOR_COUNT;

    // Keep logical state in sync with fade target so any field change is persisted.
    g_led.is_on = is_on;
    g_led.brightness = brightness_percent;
    g_led.color_index = color_index;
   // led_sync_key_level_from_brightness(g_led.is_on, g_led.brightness);

    g_led.start_w = g_led.cur_w;
    g_led.start_r = g_led.cur_r;
    g_led.start_g = g_led.cur_g;
    g_led.start_b = g_led.cur_b;

    led_calc_target_from_logic(g_led.is_on, g_led.brightness, g_led.color_index,
                               &g_led.target_w, &g_led.target_r, &g_led.target_g, &g_led.target_b);

    g_led.fade_start_ms = LED_GetTickMs();
    g_led.fade_time_ms = (fade_ms == 0) ? 1 : fade_ms;
    g_led.fading = true;
}



void LED_Init(void)
{
    uint8_t brightness = 100;
    uint8_t color_index = 2;
    bool is_on = true;

    //获取记忆中的数据
    //LED_LoadState(&brightness, &color_index, &is_on);

    brightness = led_clamp_u8(brightness, 1, 100);
    color_index %= LED_COLOR_COUNT;

    g_led.is_on = is_on;
    g_led.brightness = brightness;
    g_led.color_index = color_index;
    g_led.key_next_off = false;
    g_led.fading = false;

  //  led_sync_key_level_from_brightness(is_on, brightness);

    float w, r, g, b;
    led_calc_target_from_logic(g_led.is_on, g_led.brightness, g_led.color_index, &w, &r, &g, &b);
    led_apply_output(w, r, g, b);
}

void LED_Tick10ms(void)
{
    if (!g_led.fading)
        return;

    uint32_t now = LED_GetTickMs();
    uint32_t elapsed = now - g_led.fade_start_ms;

    if (elapsed >= g_led.fade_time_ms)
    {
        led_apply_output(g_led.target_w, g_led.target_r, g_led.target_g, g_led.target_b);
        g_led.fading = false;
        return;
    }

    float t = (float)elapsed / (float)g_led.fade_time_ms;

    float w = g_led.start_w + (g_led.target_w - g_led.start_w) * t;
    float r = g_led.start_r + (g_led.target_r - g_led.start_r) * t;
    float g = g_led.start_g + (g_led.target_g - g_led.start_g) * t;
    float b = g_led.start_b + (g_led.target_b - g_led.start_b) * t;
    led_apply_output(w, r, g, b);
}
