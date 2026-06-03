#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "time.h"

#if defined(SL_CATALOG_KERNEL_PRESENT)
#include <cmsis_os2.h>
#endif

#define LED_BRIGHTNESS_MIN 1
#define LED_BRIGHTNESS_MAX 100
#define LED_HW_MAX         1000
#define LED_COLOR_COUNT    13

typedef struct
{
    uint8_t w;   // 0~100
    uint8_t r;   // 0~100
    uint8_t g;   // 0~100
    uint8_t b;   // 0~100
} led_color_t;

typedef enum
{
    LED_LEVEL_OFF = 0,
    LED_LEVEL_50  = 50,
    LED_LEVEL_100 = 100,
} led_key_level_t;

typedef struct
{
    bool     is_on;              // 当前逻辑开关状态
    uint8_t  brightness;         // 记忆亮度 1~100，默认100
    uint8_t  color_index;        // 0~12，默认2

    bool     key_next_off;       // 若被 Matter/App 外部开灯，则下次短按优先关灯
    uint8_t  key_cycle_level;    // 按键循环档位：0/50/100

    // 当前输出值（0~100）
    float    cur_w;
    float    cur_r;
    float    cur_g;
    float    cur_b;

    // 渐变起始值
    float    start_w;
    float    start_r;
    float    start_g;
    float    start_b;

    // 渐变目标值
    float    target_w;
    float    target_r;
    float    target_g;
    float    target_b;

    uint32_t fade_start_ms;
    uint32_t fade_time_ms;
    bool     fading;
} led_ctrl_t;

extern led_ctrl_t g_led;

/* 初始化 */
void LED_Init(void);

/* 10ms 调用一次 */
void LED_Tick10ms(void);

/* 按键事件 */
void LED_KeyShortPress(void);
void LED_KeyDoublePress(void);

/* Matter/App 控制接口 */
void LED_SetOnOff(bool on);
void LED_SetBrightnessPercent(uint8_t percent);   // App 1~100
void LED_SetColorIndex(uint8_t index);            // 0~12

/* 状态读取 */
bool    LED_IsOn(void);
uint8_t LED_GetBrightnessPercent(void);
uint8_t LED_GetColorIndex(void);

/* 持久化加载/保存接口（需用户实现或自行替换） */
void LED_LoadState(uint8_t *brightness, uint8_t *color_index, bool *is_on);
void LED_SaveState(uint8_t brightness, uint8_t color_index, bool is_on);

/* 底层唯一输出接口：用户实现 */
void LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b);


void led_calc_target_from_logic(bool is_on, uint8_t brightness_percent, uint8_t color_index,
                               float *w, float *r, float *g, float *b);

void led_start_fade_to_logic(bool is_on, uint8_t brightness_percent, uint8_t color_index, uint32_t fade_ms);



#endif
