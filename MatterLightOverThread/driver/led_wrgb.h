#pragma once

#include "AppConfig.h"
#include <stdint.h>
#include <stdbool.h>
#include "time.h"

#if defined(SL_CATALOG_KERNEL_PRESENT)
#include <cmsis_os2.h>
#endif

#define LED_FADE_KEY_TOTAL_MS    400
#define LED_FADE_COLOR_SWITCH_MS 400

#define LED_BRIGHTNESS_MIN 1
#define LED_BRIGHTNESS_MAX 100
#define LED_HW_MAX         1023
#define LED_COLOR_COUNT    13

typedef struct
{
    uint16_t w; // 0~100
    uint16_t r; // 0~100
    uint16_t g; // 0~100
    uint16_t b; // 0~100
} led_color_t;

typedef enum
{
    LED_LEVEL_OFF = 0,
    LED_LEVEL_50 = 50,
    LED_LEVEL_100 = 100,
} led_key_level_t;

// 🎯 新增：灯效模式枚举
typedef enum
{
    LED_EFFECT_NONE = 0, // 无特效（走你原本的常规 Fade/静态 逻辑）
    LED_EFFECT_BLINK,    // 闪烁模式
    LED_EFFECT_BREATH,   // 呼吸模式
    LED_EFFECT_HOLD      // 保持模式
} led_effect_mode_t;

// 🎯 新增：区分当前的色彩来源是颜色表索引还是自定义 RGBW
typedef enum
{
    LED_SOURCE_COLOR_INDEX = 0,
    LED_SOURCE_CUSTOM_RGBW
} led_color_source_t;

enum class StateChangeOrigin : uint8_t
{
    UNKNOWN = 0, // 未知来源（初始化默认）
    LOCAL_KEY,   // 本地物理按键触发
    MATTER_APP,  // 外部 Matter / App 远程下发触发
    AUTO_TIMER   // 定时器或其他自动化逻辑触发（预留扩展）
};

typedef struct
{
    bool    low_battery_protected; // 低电量保护状态
    bool    is_on;                 // 当前逻辑开关状态
    uint8_t brightness;            // 记忆亮度 0~100，默认100
    uint8_t history_brightness;    // 历史亮度，用于短按时的亮度切换逻辑
    uint8_t color_index;           // 0~12，默认0

    // 若被 Matter/App 外部开灯，则下次短按优先关灯
    // 🎯 核心修改：用枚举替代原有的 bool key_next_off;
    StateChangeOrigin change_origin;

    // 🎯 新增：色彩来源管理
    led_color_source_t color_source; // 标记当前色彩来源
    led_color_t        custom_raw;   // 用于缓存未经亮度缩放的自定义原始 RGBW，供呼吸/闪烁灯效使用

    // 🎯 变更：全部使用纯整数结构体单元
    led_color_t cur_color;    // 当前实际输出的颜色值（已应用亮度缩放）
    led_color_t start_color;  // 渐变起始颜色（已应用亮度缩放）
    led_color_t target_color; // 渐变目标颜色（已应用亮度缩放）

    uint32_t fade_start_ms;
    uint32_t fade_time_ms; //
    bool     fading;       // 是否正在渐变中

    bool    mix_lighting_effects; // 混合特效时是否应用
    uint8_t mix_effect_end;       // 结束等效
    uint8_t mix_effect_index;     // 当前特效索引（如果需要区分多种特效）
    bool    mix_end_is_on;        // 混合特效结束时的开关状态
    uint8_t mix_end_brightness;   // 混合特效结束时的亮度
    uint8_t mix_end_color_index;  // 混合特效结束时的颜色索引

    // 🎯 升级扩展：灯效控制上下文
    led_effect_mode_t effect_mode;      // 当前灯效
    uint32_t          effect_start_ms;  // 特效开始时间
    uint32_t          effect_period_ms; // 特效总周期（毫秒）
    uint32_t          effect_count;     // 剩余闪烁/呼吸次数（0代表无限循环）
    bool              blink_toggle;     // 闪烁亮灭翻转标志
    bool              hold_is_on;       // 保持模式的开关状态
} led_ctrl_t;

typedef struct
{
    uint8_t  fuction_mode; // 0=无功能，1=闪烁，2=呼吸，3=保持
    uint8_t  is_on;
    uint8_t  brightness;
    uint8_t  color_index;
    uint16_t fade_ms;
    uint16_t count;
} MixedLightingEffects_t;

extern const led_color_t      g_color_table[LED_COLOR_COUNT];
extern MixedLightingEffects_t g_mixed_effects[5];
// extern led_ctrl_t             g_led;

/* 初始化 */
void LED_Init(void);

/* 10ms 调用一次 */
void LED_Tick10ms(void);

/* 状态读取和写入 */
bool               led_Get_status(void);
void               led_Set_status(bool on);
uint8_t            led_Get_brightness(void);
void               led_Set_brightness(uint8_t brightness);
uint8_t            led_Get_color_index(void);
void               led_Set_color_index(uint8_t color_index);
led_color_source_t led_Get_color_source(void);
void               led_Set_color_source(led_color_source_t source);
StateChangeOrigin  led_Get_change_origin(void);
void               led_Set_change_origin(StateChangeOrigin origin);
uint8_t            led_Get_history_brightness(void);
void               led_Set_history_brightness(uint8_t brightness);
led_color_t        led_Get_custom_raw(void);
void               led_Set_custom_raw(led_color_t raw);

void custom_raw_color_safeguard(uint8_t color_index);

/* 持久化加载/保存接口（需用户实现或自行替换） */
void LED_LoadState(uint8_t *brightness, uint8_t *color_index, bool *is_on);
void LED_SaveState(uint8_t brightness, uint8_t color_index, bool is_on);

/* 底层唯一输出接口：用户实现 */
void    LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b);
uint8_t LED_SetLowBatteryProtection(bool protect);
void    LED_Start_Fade_Color_Index(bool is_on, uint8_t brightness_percent, uint8_t color_index, uint16_t fade_ms);
void    LED_Start_Fade_RGBW_8bit(uint8_t W, uint8_t R, uint8_t G, uint8_t B, uint16_t fade_ms);
void    LED_Start_Fade_RGBW(bool is_on, uint8_t brightness_percent, led_color_t custom_raw, uint16_t fade_ms);
void    LED_SetBlink(uint8_t brightness, uint8_t color_index, uint16_t period_ms, uint16_t count);
void    LED_SetBreath(uint8_t brightness, uint8_t color_index, uint16_t count);
void    LED_SetHold(bool is_on, uint8_t brightness, uint8_t color_index, uint16_t fade_ms);
void    LED_StopEffect(void);

void Led_MixedLightingEffects_Start(uint8_t effect_end, bool is_on, uint8_t brightness, uint8_t color_index);
void LED_StopMixedEffects(bool is_on, uint8_t brightness, uint8_t color_index);
