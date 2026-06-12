/**
 * @file LightTypes.h
 * @brief 跨层共享类型：WRGB 颜色、淡入策略、Flash 结构体、兼容旧 led_wrgb 命名
 * @layer Common
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define LED_HW_MAX                 1023
#define LED_BRIGHTNESS_MIN         1
#define LED_BRIGHTNESS_MAX         255
#define LED_FADE_COLOR_SWITCH_MS   400
#define LED_FADE_KEY_TOTAL_MS      400

#define NVM3_KEY_LIGHT_MEMORY_DATA     0x00011
#define NVM3_KEY_FIRST_COMMISSION_DONE 0x00012
#define FIRST_COMMISSION_DONE_MAGIC    0xA5

/** @brief WRGB 四通道颜色（0~1023） */
typedef struct
{
    uint16_t w;
    uint16_t r;
    uint16_t g;
    uint16_t b;
} WrgbColor;

/** @brief NVM3 灯光记忆结构体 */
typedef struct
{
    uint8_t  is_on;
    uint8_t  brightness;
    uint8_t  color_cycle_index;
    uint8_t  color_source;
    uint16_t custom_w;
    uint16_t custom_r;
    uint16_t custom_g;
    uint16_t custom_b;
} __attribute__((packed)) LightFlashMemory;

typedef enum
{
    ColorSourceIndexTable = 0,
    ColorSourceCustomWrgb = 1
} ColorSource;

typedef enum
{
    ChangeOriginUnknown = 0,
    ChangeOriginLocalKey,
    ChangeOriginMatterApp,
    ChangeOriginAutoTimer
} ChangeOrigin;

typedef enum
{
    EffectModeNone = 0,
    EffectModeBlink,
    EffectModeBreath,
    EffectModeHold
} EffectMode;

typedef enum
{
    FadePolicyFixedMs = 0,
    FadePolicyProportionalBrightness,
    FadePolicyEaseOutQuad
} FadePolicy;

typedef void (*MixedEffectCallback)(void);

/** @brief 混合灯效单步配置 */
typedef struct
{
    uint8_t   function_mode;
    bool      is_on;
    uint8_t   brightness;
    WrgbColor raw_color;
    uint16_t  fade_ms;
    uint16_t  count;
} MixedEffectStep;

// 兼容旧 led_wrgb 命名
typedef WrgbColor            led_color_t;
typedef LightFlashMemory       light_flash_memory_t;
typedef ColorSource            led_color_source_t;
typedef ChangeOrigin           led_change_origin_t;
typedef EffectMode             led_effect_mode;
typedef MixedEffectStep        MixedLightingEffects_t;
typedef MixedEffectCallback    led_mixed_callback_t;

#define LED_SOURCE_INDEX_TABLE  ColorSourceIndexTable
#define LED_SOURCE_CUSTOM_RGBW  ColorSourceCustomWrgb
#define LED_ORIGIN_UNKNOWN      ChangeOriginUnknown
#define LED_ORIGIN_LOCAL_KEY    ChangeOriginLocalKey
#define LED_ORIGIN_MATTER_APP   ChangeOriginMatterApp
#define LED_ORIGIN_AUTO_TIMER   ChangeOriginAutoTimer
#define LED_EFFECT_NONE         EffectModeNone
#define LED_EFFECT_BLINK        EffectModeBlink
#define LED_EFFECT_BREATH       EffectModeBreath
#define LED_EFFECT_HOLD         EffectModeHold
#define LED_LEVEL_OFF           0
#define LED_LEVEL_50            128
#define LED_LEVEL_100           255
