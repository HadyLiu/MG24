#pragma once

#include "AppConfig.h"
#include <cstring> // 🎯 修复1：引入 string 库，解决 'memcmp' was not declared 错误
#include "nvm3.h"
#include <stdint.h>
#include <stdbool.h>
#include "time.h"
#include "led_pwm_port.h"

#if defined(SL_CATALOG_KERNEL_PRESENT)
#include <cmsis_os2.h>
#endif

#define LED_FADE_KEY_TOTAL_MS    400
#define LED_FADE_COLOR_SWITCH_MS 400

#define LED_BRIGHTNESS_MIN 1
#define LED_BRIGHTNESS_MAX 255
#define LED_HW_MAX         1023
#define LED_COLOR_COUNT    13

#define NVM3_KEY_LIGHT_MEMORY_DATA      0x00011 // 灯光状态记忆
#define NVM3_KEY_FIRST_COMMISSION_DONE  0x00012 // 首次配网完成标志（独立 key，避免破坏旧结构体布局）
#define FIRST_COMMISSION_DONE_MAGIC     0xA5

// 混合灯效结束后触发回调
typedef void (*led_mixed_callback_t)(void);

// 专门用于断电记忆的结构体（紧凑对齐）
typedef struct
{
    uint8_t  is_on;        // 开关状态
    uint8_t  brightness;   // 亮度 (0~100)
    uint8_t  color_index;  // 颜色索引表 ID (0~12)
    uint8_t  color_source; // 色彩来源 (索引表还是自定义：led_color_source_t)
    uint16_t custom_w;     // 备份的自定义 WRGB 原始值
    uint16_t custom_r;
    uint16_t custom_g;
    uint16_t custom_b;
} __attribute__((packed)) light_flash_memory_t;

typedef struct
{
    uint16_t w; // 0~1023
    uint16_t r; // 0~1023
    uint16_t g; // 0~1023
    uint16_t b; // 0~1023
} led_color_t;

typedef enum
{
    LED_LEVEL_OFF = 0,
    LED_LEVEL_50 = 128,
    LED_LEVEL_100 = 255,
} led_key_level;

// 🎯 新增：灯效模式枚举
typedef enum
{
    LED_EFFECT_NONE = 0, // 无特效（走你原本的常规 Fade/静态 逻辑）
    LED_EFFECT_BLINK,    // 闪烁模式
    LED_EFFECT_BREATH,   // 呼吸模式
    LED_EFFECT_HOLD      // 保持模式
} led_effect_mode;

// 统一规范命名，消除原本混乱的 StateChangeOrigin 编译报错
typedef enum
{
    LED_ORIGIN_UNKNOWN = 0,
    LED_ORIGIN_LOCAL_KEY,
    LED_ORIGIN_MATTER_APP,
    LED_ORIGIN_AUTO_TIMER
} led_change_origin_t;

// 色彩来源类型枚举
typedef enum
{
    LED_SOURCE_INDEX_TABLE = 0,
    LED_SOURCE_CUSTOM_RGBW = 1
} led_color_source_t;

typedef struct
{
    bool                is_on;
    uint8_t             brightness;            // 0~255
    uint8_t             history_brightness;    // 记忆亮度，用于短按时的亮度切换逻辑
    bool                low_battery_protected; // 低电量保护状态
    uint8_t             color_index;           // 0~12，默认0
    led_change_origin_t change_origin;         // Led变化来源
    led_color_source_t  color_source;          // 色彩来源属性
    led_color_t         raw_color;             // 🎯 常规非特效状态下的原始基准色（已移除多余色彩成员）

    // 特效控制变量
    led_effect_mode effect_mode; // 当前特效模式（闪烁、呼吸、保持）
    uint32_t        effect_start_ms;
    uint32_t        effect_period_ms;
    uint16_t        effect_count;

    // 状态机运行时颜色变量
    led_color_t cur_color;    // 当前硬件实际输出值
    led_color_t start_color;  // 常规 Fade 渐变起点
    led_color_t target_color; // 常规 Fade 渐变终点

    // 常规 Fade 渐变控制
    bool     fading;        // 是否正在渐变中
    uint32_t fade_start_ms; // 渐变开始时间
    uint16_t fade_time_ms;  // 渐变总时间

    // 混合灯效链路控制
    bool        mix_lighting_effects; // 是否正在执行混合特效
    uint8_t     mix_effect_index;     // 当前混合特效索引
    uint8_t     mix_effect_end;       // 结束混合特效
    bool        mix_end_is_on;        // 中断混合特效结束后恢复的开关状态
    uint8_t     mix_end_brightness;   // 中断混合特效结束后
    led_color_t mix_end_raw_color;    // 中断混合特效结束后恢复的原始颜色

    led_mixed_callback_t mix_end_callback; // 混合特效结束回调

} led_ctrl_t;

// 混合特效单步配置结构体（完全基于计算好的绝对色彩）
typedef struct
{
    uint8_t     fuction_mode; //
    bool        is_on;
    uint8_t     brightness; // 0~255
    led_color_t raw_color;  //
    uint16_t    fade_ms;
    uint16_t    count;
} MixedLightingEffects_t;

extern const led_color_t      g_color_table[LED_COLOR_COUNT];
extern MixedLightingEffects_t g_mixed_effects[5];

/* API 接口 */
bool                led_get_status(void);
void                led_set_status(bool on);
uint8_t             led_get_brightness(void);
void                led_set_brightness(uint8_t brightness);
uint8_t             led_get_color_index(void);
void                led_set_color_index(uint8_t color_index);
led_change_origin_t led_get_change_origin(void);
void                led_set_change_origin(led_change_origin_t origin);
led_color_source_t  led_get_color_source(void);
void                led_set_color_source(led_color_source_t source);
led_color_t         led_get_color_table(uint8_t color_index);
uint8_t             led_get_history_brightness(void);
void                led_set_history_brightness(uint8_t history_brightness);

void        led_set_raw_color(led_color_t raw_color);
led_color_t led_get_raw_color(void);

void LED_SaveStateToFlash(void);
bool LED_IsFirstCommissionDone(void);
void LED_SetFirstCommissionDone(void);
bool LED_IsUserEffectIdle(void);

void LED_Init(void);
void LED_Tick10ms(void);

void    LED_Init(void);
void    LED_Tick10ms(void);
void    LED_StopEffect(void);
uint8_t LED_SetLowBatteryProtection(bool protect);
void    led_update_normal_state(bool is_on, uint8_t brightness, led_color_t raw_color);
void    LED_Start_Fade_To_Current(uint16_t fade_ms);

void LED_SetBlink(uint8_t brightness, led_color_t raw_color, uint16_t period_ms, uint16_t count);
void LED_SetBreath(uint8_t brightness, led_color_t raw_color, uint16_t count);
void LED_SetHold(bool is_on, uint8_t brightness, led_color_t raw_color, uint16_t fade_ms);

void Led_MixedLightingEffects_Start(uint8_t effect_end, led_mixed_callback_t callback);
void led_mixed_lighting_effects_service(void);
void LED_StopMixedEffects();
///* 初始化 */
// void LED_Init(void);
//
///* 10ms 调用一次 */
// void LED_Tick10ms(void);
//
///* 状态读取和写入 */
// bool               led_get_status(void);
// void               led_set_status(bool on);
// uint8_t            led_get_brightness(void);
// void               led_set_brightness(uint8_t brightness);
// uint8_t            led_get_color_index(void);
// void               led_set_color_index(uint8_t color_index);
// led_color_source_t led_get_color_source(void);
// void               led_set_color_source(led_color_source_t source);
// StateChangeOrigin  led_get_change_origin(void);
// void               led_set_change_origin(StateChangeOrigin origin);
// uint8_t            led_get_history_brightness(void);
// void               led_set_history_brightness(uint8_t brightness);
// led_color_t        led_get_custom_raw(void);
// void               led_set_custom_raw(led_color_t raw);
//
// void custom_raw_color_safeguard(uint8_t color_index);
//
///* 持久化加载/保存接口（需用户实现或自行替换） */
// void LED_SaveStateToFlash(void);
//
///* 底层唯一输出接口：用户实现 */
// void    LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b);
// uint8_t LED_SetLowBatteryProtection(bool protect);
// void    LED_Start_Fade_Color_Index(bool is_on, uint8_t brightness_percent, uint8_t color_index, uint16_t fade_ms);
// void    LED_Start_Fade_RGBW_8bit(uint8_t W, uint8_t R, uint8_t G, uint8_t B, uint16_t fade_ms);
// void    LED_Start_Fade_RGBW(bool is_on, uint8_t brightness_percent, led_color_t custom_raw, uint16_t fade_ms);
// void    LED_SetBlink(uint8_t brightness, uint8_t color_index, uint16_t period_ms, uint16_t count);
// void    LED_SetBreath(uint8_t brightness, uint8_t color_index, uint16_t count);
// void    LED_SetHold(bool is_on, uint8_t brightness, uint8_t color_index, uint16_t fade_ms);
// void    LED_StopEffect(void);
//
// void Led_MixedLightingEffects_Start(uint8_t effect_end, bool is_on, uint8_t brightness, uint8_t color_index);
// void LED_StopMixedEffects(bool is_on, uint8_t brightness, uint8_t color_index);
