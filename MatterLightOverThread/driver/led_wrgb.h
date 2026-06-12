/**
 * @file led_wrgb.h
 * @brief 兼容层：旧 C API 声明，实现委托 LightEngine
 * @layer Driver (legacy wrapper)
 */
#pragma once

#include "app/LightTypes.h"
#include "../service/LightEngine.h"
#include "../service/ColorLibrary.h"
#include <stdint.h>

#define LED_COLOR_COUNT 12

extern MixedLightingEffects_t g_mixed_effects[5];

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
void                led_set_raw_color(led_color_t raw_color);
led_color_t         led_get_raw_color(void);

void LED_Init(void);
void LED_Tick10ms(void);
void LED_SaveStateToFlash(void);
bool LED_IsFirstCommissionDone(void);
void LED_SetFirstCommissionDone(void);
bool LED_IsUserEffectIdle(void);
void LED_StopEffect(void);
uint8_t LED_SetLowBatteryProtection(bool protect);
void led_update_normal_state(bool is_on, uint8_t brightness, led_color_t raw_color);
void LED_Start_Fade_To_Current(uint16_t fade_ms);
void LED_SetBlink(uint8_t brightness, led_color_t raw_color, uint16_t period_ms, uint16_t count);
void LED_SetBreath(uint8_t brightness, led_color_t raw_color, uint16_t count);
void LED_SetHold(bool is_on, uint8_t brightness, led_color_t raw_color, uint16_t fade_ms);
void Led_MixedLightingEffects_Start(uint8_t effect_end, led_mixed_callback_t callback);
void LED_StopMixedEffects(void);
