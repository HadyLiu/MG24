/**
 * @file led_wrgb.cpp
 * @brief 兼容层：旧 C API 实现，全部委托 LightEngine / ColorLibrary
 * @layer Driver (legacy wrapper)
 */
#include "led_wrgb.h"
#include "../service/ColorLibrary.h"
#include "../service/LightEngine.h"

MixedLightingEffects_t g_mixed_effects[5];

bool led_get_status(void) { return LightEngine::instance().isOn(); }
void led_set_status(bool on) { LightEngine::instance().updateNormalState(on, LightEngine::instance().brightness(), LightEngine::instance().rawColor()); }
uint8_t led_get_brightness(void) { return LightEngine::instance().brightness(); }
void led_set_brightness(uint8_t brightness) { LightEngine::instance().updateNormalState(LightEngine::instance().isOn(), brightness, LightEngine::instance().rawColor()); }
uint8_t led_get_color_index(void) { return LightEngine::instance().colorCycleIndex(); }
void led_set_color_index(uint8_t color_index) { LightEngine::instance().setColorFromCycleIndex(color_index, 0); }
led_change_origin_t led_get_change_origin(void) { return LightEngine::instance().changeOrigin(); }
void led_set_change_origin(led_change_origin_t origin) { LightEngine::instance().setChangeOrigin(origin); }
led_color_source_t led_get_color_source(void) { return LightEngine::instance().colorSource(); }
void led_set_color_source(led_color_source_t source) { LightEngine::instance().setColorSource(source); }
led_color_t led_get_color_table(uint8_t color_index) { return ColorLibrary::instance().colorAtCycleIndex(color_index); }
uint8_t led_get_history_brightness(void) { return LightEngine::instance().historyBrightness(); }
void led_set_history_brightness(uint8_t v) { LightEngine::instance().setHistoryBrightness(v); }
void led_set_raw_color(led_color_t raw_color) { LightEngine::instance().updateNormalState(LightEngine::instance().isOn(), LightEngine::instance().brightness(), raw_color); }
led_color_t led_get_raw_color(void) { return LightEngine::instance().rawColor(); }

void LED_Init(void) { LightEngine::instance().init(); }
void LED_Tick10ms(void) { LightEngine::instance().tick10ms(); }
void LED_SaveStateToFlash(void) { LightEngine::instance().saveStateToFlash(); }
bool LED_IsFirstCommissionDone(void) { return LightEngine::instance().isFirstCommissionDone(); }
void LED_SetFirstCommissionDone(void) { LightEngine::instance().setFirstCommissionDone(); }
bool LED_IsUserEffectIdle(void) { return LightEngine::instance().isUserEffectIdle(); }
void LED_StopEffect(void) { LightEngine::instance().stopEffect(); }
uint8_t LED_SetLowBatteryProtection(bool protect) { return LightEngine::instance().setLowBatteryProtection(protect); }
void led_update_normal_state(bool is_on, uint8_t brightness, led_color_t raw_color) { LightEngine::instance().updateNormalState(is_on, brightness, raw_color); }
void LED_Start_Fade_To_Current(uint16_t fade_ms) { LightEngine::instance().startFadeToCurrent(fade_ms, FadePolicyProportionalBrightness); }
void LED_SetBlink(uint8_t brightness, led_color_t raw_color, uint16_t period_ms, uint16_t count) { LightEngine::instance().setBlink(brightness, raw_color, period_ms, count); }
void LED_SetBreath(uint8_t brightness, led_color_t raw_color, uint16_t count) { LightEngine::instance().setBreath(brightness, raw_color, count); }
void LED_SetHold(bool is_on, uint8_t brightness, led_color_t raw_color, uint16_t fade_ms) { LightEngine::instance().setHold(is_on, brightness, raw_color, fade_ms); }
void Led_MixedLightingEffects_Start(uint8_t effect_end, led_mixed_callback_t callback) { LightEngine::instance().runMixedSequence(g_mixed_effects, effect_end, callback); }
void LED_StopMixedEffects(void) { LightEngine::instance().stopMixedEffects(); }
