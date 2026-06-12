/**
 * @file LightEngine.h
 * @brief 灯光引擎服务：开关/亮度/颜色/淡入淡出/混合特效/Flash 记忆
 * @layer Service
 */
#pragma once

#include "app/LightTypes.h"
#include "../driver/hal/NvmStorage.h"
#include "ColorLibrary.h"
#include <stdint.h>

/** @brief 灯光状态快照，用于 Identify 等场景的保存/恢复 */
struct LightSnapshot
{
    bool        is_on;
    uint8_t     brightness;
    uint8_t     color_cycle_index;
    ColorSource color_source;
    WrgbColor   raw_color;
};

/**
 * @class LightEngine
 * @brief 主灯 WRGB 状态机：替代原 led_ctrl_t 全局状态
 */
class LightEngine
{
public:
    static LightEngine &instance();

    void init();
    void tick10ms();

    void setOnOff(bool on, FadePolicy fade = FadePolicyProportionalBrightness);
    void setBrightness(uint8_t level, FadePolicy fade = FadePolicyProportionalBrightness);
    void setColorFromCycleIndex(uint8_t cycleIndex, uint16_t fadeMs = LED_FADE_COLOR_SWITCH_MS);
    void setColorRaw(const WrgbColor &c, ColorSource src, uint16_t fadeMs = LED_FADE_COLOR_SWITCH_MS);

    uint16_t calcFadeMsForBrightnessDelta(uint8_t from, uint8_t to) const;
    uint16_t resolveFadeMs(FadePolicy policy, uint8_t fromBrightness, uint8_t toBrightness, uint16_t fixedMs) const;

    void runMixedSequence(const MixedEffectStep *steps, uint8_t stepCount, MixedEffectCallback cb);
    void stopMixedEffects();
    LightSnapshot captureSnapshot() const;
    void        restoreSnapshot(const LightSnapshot &s, uint16_t fadeMs);

    void startFadeToCurrent(uint16_t fadeMs, FadePolicy policy = FadePolicyProportionalBrightness);
    void stopEffect();
    void setBlink(uint8_t brightness, const WrgbColor &raw, uint16_t periodMs, uint16_t count);
    void setBreath(uint8_t brightness, const WrgbColor &raw, uint16_t count);
    void setHold(bool on, uint8_t brightness, const WrgbColor &raw, uint16_t fadeMs);
    uint8_t setLowBatteryProtection(bool protect);

    void saveStateToFlash();
    bool isFirstCommissionDone() const;
    void setFirstCommissionDone();
    bool isUserEffectIdle() const;

    bool         isOn() const { return m_state.is_on; }
    uint8_t      brightness() const { return m_state.brightness; }
    uint8_t      colorCycleIndex() const { return m_state.color_cycle_index; }
    uint8_t      historyBrightness() const { return m_state.history_brightness; }
    void         setHistoryBrightness(uint8_t v) { m_state.history_brightness = v; }
    ChangeOrigin changeOrigin() const { return m_state.change_origin; }
    void         setChangeOrigin(ChangeOrigin o) { m_state.change_origin = o; }
    ColorSource  colorSource() const { return m_state.color_source; }
    void         setColorSource(ColorSource s) { m_state.color_source = s; }
    WrgbColor    rawColor() const { return m_state.raw_color; }
    bool         isLowBatteryProtected() const { return m_state.low_battery_protected; }

    void updateNormalState(bool on, uint8_t brightness, const WrgbColor &raw);
    MixedEffectStep *mixedEffectsBuffer() { return m_mixedEffects; }

private:
    LightEngine() = default;

    struct State
    {
        bool                is_on;
        uint8_t             brightness;
        uint8_t             history_brightness;
        bool                low_battery_protected;
        uint8_t             color_cycle_index;
        ChangeOrigin        change_origin;
        ColorSource         color_source;
        WrgbColor           raw_color;
        EffectMode          effect_mode;
        uint32_t            effect_start_ms;
        uint32_t            effect_period_ms;
        uint16_t              effect_count;
        WrgbColor           cur_color;
        WrgbColor           start_color;
        WrgbColor           target_color;
        bool                fading;
        uint32_t            fade_start_ms;
        uint16_t            fade_time_ms;
        FadePolicy          fade_policy;
        bool                mix_lighting_effects;
        uint8_t             mix_effect_index;
        uint8_t             mix_effect_end;
        bool                mix_end_is_on;
        uint8_t             mix_end_brightness;
        WrgbColor           mix_end_raw_color;
        MixedEffectCallback mix_end_callback;
    };

    State           m_state{};
    MixedEffectStep m_mixedEffects[5]{};
    bool            m_firstCommissionDone = false;

    static uint16_t clampU16(uint16_t v, uint16_t minV, uint16_t maxV);
    static uint8_t  clampU8(uint8_t v, uint8_t minV, uint8_t maxV);
    void            applyOutput(const WrgbColor &color);
    WrgbColor       scaleColor(const WrgbColor &raw, uint8_t brightness) const;
    void            executeMixedIndex(uint8_t index);
    void            mixedEffectsService();
    bool            loadStateFromFlash();
    void            startFadeInternal(uint16_t fadeMs, FadePolicy policy);
};
