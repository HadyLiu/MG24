/**
 * @file LightEngine.cpp
 * @brief 灯光引擎实现：淡入淡出 FSM、特效链、NVM3 持久化
 * @layer Service
 */
#include "LightEngine.h"
#include "../driver/hal/PwmHal.h"
#include "../driver/time.h"
#include "AppConfig.h"
#include <cstring>

// EaseOutQuad 缓动表：0-800ms
static const uint16_t kEaseQuadTable4096[81] = {
    0,    1,    5,    11,   20,   32,   46,   62,   81,   103,  128,  154,  184,  216,  251,  288,  327,  370,  414,  462,  512,
    564,  620,  677,  737,  800,  865,  933,  1004, 1077, 1152, 1230, 1311, 1394, 1479, 1567, 1658, 1750, 1845, 1943, 2048, 2153,
    2251, 2346, 2438, 2529, 2617, 2702, 2785, 2866, 2944, 3019, 3092, 3163, 3231, 3296, 3359, 3419, 3476, 3532, 3584, 3634, 3682,
    3726, 3769, 3808, 3845, 3880, 3912, 3942, 3968, 3993, 4015, 4034, 4050, 4064, 4076, 4085, 4091, 4095, 4096};

LightEngine &LightEngine::instance()
{
    static LightEngine s_engine;
    return s_engine;
}

uint16_t LightEngine::clampU16(uint16_t v, uint16_t minV, uint16_t maxV)
{
    return (v <= minV) ? minV : ((v >= maxV) ? maxV : v);
}

uint8_t LightEngine::clampU8(uint8_t v, uint8_t minV, uint8_t maxV)
{
    return (v <= minV) ? minV : ((v >= maxV) ? maxV : v);
}

uint16_t LightEngine::calcFadeMsForBrightnessDelta(uint8_t from, uint8_t to) const
{
    uint8_t delta = (from > to) ? static_cast<uint8_t>(from - to) : static_cast<uint8_t>(to - from);
    // 50% 跨度(128) = 400ms，100% 跨度(255) ≈ 800ms
    return static_cast<uint16_t>((static_cast<uint32_t>(delta) * 400U) / 128U);
}

uint16_t LightEngine::resolveFadeMs(FadePolicy policy, uint8_t fromBrightness, uint8_t toBrightness, uint16_t fixedMs) const
{
    switch (policy)
    {
    case FadePolicyFixedMs:
        return (fixedMs == 0) ? 1 : fixedMs;
    case FadePolicyEaseOutQuad:
        return (fixedMs == 0) ? 800 : fixedMs;
    case FadePolicyProportionalBrightness:
    default:
        return calcFadeMsForBrightnessDelta(fromBrightness, toBrightness);
    }
}

void LightEngine::applyOutput(const WrgbColor &color)
{
    WrgbColor c = color;
    c.w = clampU16(c.w, 0, LED_HW_MAX);
    c.r = clampU16(c.r, 0, LED_HW_MAX);
    c.g = clampU16(c.g, 0, LED_HW_MAX);
    c.b = clampU16(c.b, 0, LED_HW_MAX);
    m_state.cur_color = c;
    PwmHal::setMainWrgb(c.w, c.r, c.g, c.b);
}

WrgbColor LightEngine::scaleColor(const WrgbColor &raw, uint8_t brightness) const
{
    WrgbColor target = {0, 0, 0, 0};
    uint32_t  scale = 204 + ((static_cast<uint32_t>(brightness <= 1 ? 1 : brightness) - 1) * 10035U) / 254U;
    if (brightness <= 1)
        scale = 204;
    else if (brightness >= 255)
        scale = 10240;
    target.w = static_cast<uint16_t>((static_cast<uint32_t>(raw.w) * scale) >> 14);
    target.r = static_cast<uint16_t>((static_cast<uint32_t>(raw.r) * scale) >> 14);
    target.g = static_cast<uint16_t>((static_cast<uint32_t>(raw.g) * scale) >> 14);
    target.b = static_cast<uint16_t>((static_cast<uint32_t>(raw.b) * scale) >> 14);
    return target;
}

void LightEngine::updateNormalState(bool on, uint8_t brightness, const WrgbColor &raw)
{
    m_state.is_on = on;
    m_state.brightness = brightness;
    m_state.raw_color = raw;
}

void LightEngine::setOnOff(bool on, FadePolicy fade)
{
    uint8_t prev = m_state.brightness;
    m_state.is_on = on;
    if (!on)
        m_state.brightness = 0;
    startFadeToCurrent(resolveFadeMs(fade, prev, m_state.brightness, LED_FADE_COLOR_SWITCH_MS), fade);
}

void LightEngine::setBrightness(uint8_t level, FadePolicy fade)
{
    uint8_t prev = m_state.brightness;
    m_state.brightness = level;
    startFadeToCurrent(resolveFadeMs(fade, prev, level, LED_FADE_COLOR_SWITCH_MS), fade);
}

void LightEngine::setColorFromCycleIndex(uint8_t cycleIndex, uint16_t fadeMs)
{
    m_state.color_cycle_index = (cycleIndex >= ColorLibrary::kCycleCount) ? 0 : cycleIndex;
    m_state.color_source = ColorSourceIndexTable;
    m_state.raw_color = ColorLibrary::instance().colorAtCycleIndex(m_state.color_cycle_index);
    startFadeToCurrent(fadeMs, FadePolicyFixedMs);
}

void LightEngine::setColorRaw(const WrgbColor &c, ColorSource src, uint16_t fadeMs)
{
    m_state.raw_color = c;
    m_state.color_source = src;
    startFadeToCurrent(fadeMs, FadePolicyFixedMs);
}

void LightEngine::startFadeInternal(uint16_t fadeMs, FadePolicy policy)
{
    if (m_state.low_battery_protected)
        return;
    m_state.effect_mode = EffectModeNone;
    m_state.start_color = m_state.cur_color;
    m_state.target_color = m_state.is_on ? scaleColor(m_state.raw_color, m_state.brightness) : WrgbColor{0, 0, 0, 0};
    m_state.fade_start_ms = LED_GetTickMs();
    m_state.fade_time_ms = (fadeMs == 0) ? 1 : fadeMs;
    m_state.fade_policy = policy;
    m_state.fading = true;
}

void LightEngine::startFadeToCurrent(uint16_t fadeMs, FadePolicy policy)
{
    startFadeInternal(fadeMs, policy);
}

void LightEngine::stopEffect()
{
    m_state.effect_mode = EffectModeNone;
    m_state.fading = false;
    startFadeToCurrent(LED_FADE_COLOR_SWITCH_MS, FadePolicyFixedMs);
}

void LightEngine::setBlink(uint8_t brightness, const WrgbColor &raw, uint16_t periodMs, uint16_t count)
{
    if (m_state.low_battery_protected)
        return;
    m_state.effect_mode = EffectModeBlink;
    m_state.effect_start_ms = LED_GetTickMs();
    m_state.effect_period_ms = (periodMs < 20) ? 20 : periodMs;
    m_state.effect_count = count;
    m_state.fading = false;
    m_state.raw_color = raw;
    m_state.brightness = brightness;
}

void LightEngine::setBreath(uint8_t brightness, const WrgbColor &raw, uint16_t count)
{
    if (m_state.low_battery_protected)
        return;
    m_state.effect_mode = EffectModeBreath;
    m_state.effect_start_ms = LED_GetTickMs();
    m_state.effect_period_ms = 3200;
    m_state.effect_count = count;
    m_state.fading = false;
    m_state.raw_color = raw;
    m_state.brightness = brightness;
}

void LightEngine::setHold(bool on, uint8_t brightness, const WrgbColor &raw, uint16_t fadeMs)
{
    m_state.effect_mode = EffectModeHold;
    m_state.effect_start_ms = LED_GetTickMs();
    m_state.effect_period_ms = fadeMs;
    m_state.effect_count = (fadeMs == 0) ? 0 : 1;
    m_state.fading = false;
    WrgbColor hold = on ? scaleColor(raw, brightness) : WrgbColor{0, 0, 0, 0};
    applyOutput(hold);
}

void LightEngine::executeMixedIndex(uint8_t index)
{
    MixedEffectStep *p = &m_mixedEffects[index];
    switch (p->function_mode)
    {
    case EffectModeBlink:
        setBlink(p->brightness, p->raw_color, p->fade_ms, p->count);
        break;
    case EffectModeBreath:
        setBreath(p->brightness, p->raw_color, p->count);
        break;
    case EffectModeHold:
        setHold(p->is_on, p->brightness, p->raw_color, p->fade_ms);
        break;
    default:
        m_state.mix_lighting_effects = false;
        stopEffect();
        break;
    }
}

void LightEngine::mixedEffectsService()
{
    if (!m_state.mix_lighting_effects)
        return;
    m_state.mix_effect_index++;
    if (m_state.mix_effect_index >= m_state.mix_effect_end)
    {
        m_state.mix_lighting_effects = false;
        MixedEffectCallback cb = m_state.mix_end_callback;
        m_state.mix_end_callback = nullptr;
        if (cb)
        {
            cb();
        }
        else
        {
            WrgbColor snap = m_state.cur_color;
            m_state.is_on = m_state.mix_end_is_on;
            m_state.brightness = m_state.mix_end_brightness;
            m_state.raw_color = m_state.mix_end_raw_color;
            m_state.effect_mode = EffectModeNone;
            m_state.start_color = snap;
            m_state.target_color = m_state.is_on ? scaleColor(m_state.raw_color, m_state.brightness) : WrgbColor{0, 0, 0, 0};
            m_state.fade_start_ms = LED_GetTickMs();
            m_state.fade_time_ms = LED_FADE_COLOR_SWITCH_MS;
            m_state.fading = true;
        }
        return;
    }
    executeMixedIndex(m_state.mix_effect_index);
}

void LightEngine::runMixedSequence(const MixedEffectStep *steps, uint8_t stepCount, MixedEffectCallback cb)
{
    if (m_state.low_battery_protected || stepCount == 0 || stepCount > 5)
        return;
    SILABS_LOG("[LightEngine] mixed sequence steps=%u", stepCount);
    memcpy(m_mixedEffects, steps, stepCount * sizeof(MixedEffectStep));
    m_state.mix_effect_index = 0;
    m_state.mix_effect_end = stepCount;
    m_state.mix_lighting_effects = true;
    m_state.mix_end_is_on = m_state.is_on;
    m_state.mix_end_brightness = m_state.brightness;
    m_state.mix_end_raw_color = m_state.raw_color;
    m_state.mix_end_callback = cb;
    executeMixedIndex(0);
}

void LightEngine::stopMixedEffects()
{
    if (!m_state.mix_lighting_effects)
        return;
    m_state.mix_lighting_effects = false;
    m_state.is_on = m_state.mix_end_is_on;
    m_state.brightness = m_state.mix_end_brightness;
    m_state.raw_color = m_state.mix_end_raw_color;
    m_state.effect_mode = EffectModeNone;
    m_state.fading = false;
    startFadeToCurrent(LED_FADE_COLOR_SWITCH_MS, FadePolicyFixedMs);
}

LightSnapshot LightEngine::captureSnapshot() const
{
    LightSnapshot s{};
    s.is_on = m_state.is_on;
    s.brightness = m_state.brightness;
    s.color_cycle_index = m_state.color_cycle_index;
    s.color_source = m_state.color_source;
    s.raw_color = m_state.raw_color;
    return s;
}

void LightEngine::restoreSnapshot(const LightSnapshot &s, uint16_t fadeMs)
{
    m_state.is_on = s.is_on;
    m_state.brightness = s.brightness;
    m_state.color_cycle_index = s.color_cycle_index;
    m_state.color_source = s.color_source;
    m_state.raw_color = s.raw_color;
    startFadeToCurrent(fadeMs, FadePolicyEaseOutQuad);
}

void LightEngine::tick10ms()
{
    if (m_state.low_battery_protected)
        return;

    bool ledChange = false;
    bool stepDone = false;

    do
    {
        uint32_t now = LED_GetTickMs();
        uint32_t elapsed = now - m_state.effect_start_ms;

        if (m_state.effect_mode == EffectModeHold)
        {
            if (m_state.effect_count > 0 && elapsed >= m_state.effect_period_ms)
                stepDone = true;
            break;
        }

        if (m_state.effect_mode == EffectModeBlink)
        {
            if (m_state.effect_count > 0 && elapsed >= (static_cast<uint32_t>(m_state.effect_count) * m_state.effect_period_ms))
            {
                stepDone = true;
                break;
            }
            uint32_t tick = elapsed % m_state.effect_period_ms;
            m_state.cur_color = (tick < (m_state.effect_period_ms >> 1)) ? scaleColor(m_state.raw_color, m_state.brightness)
                                                                       : WrgbColor{0, 0, 0, 0};
            ledChange = true;
            break;
        }

        if (m_state.effect_mode == EffectModeBreath)
        {
            uint32_t cycles = elapsed / m_state.effect_period_ms;
            if (m_state.effect_count > 0 && cycles >= m_state.effect_count)
            {
                stepDone = true;
                break;
            }
            uint32_t breathTick = (elapsed % m_state.effect_period_ms) / 10;
            uint16_t factor = 0;
            if (breathTick < 80)
                factor = kEaseQuadTable4096[breathTick];
            else if (breathTick < 160)
                factor = 4096;
            else if (breathTick < 240)
                factor = kEaseQuadTable4096[240 - breathTick];
            WrgbColor base = scaleColor(m_state.raw_color, m_state.brightness);
            m_state.cur_color.w = static_cast<uint16_t>((static_cast<uint32_t>(base.w) * factor) >> 12);
            m_state.cur_color.r = static_cast<uint16_t>((static_cast<uint32_t>(base.r) * factor) >> 12);
            m_state.cur_color.g = static_cast<uint16_t>((static_cast<uint32_t>(base.g) * factor) >> 12);
            m_state.cur_color.b = static_cast<uint16_t>((static_cast<uint32_t>(base.b) * factor) >> 12);
            ledChange = true;
            break;
        }

        if (m_state.effect_mode == EffectModeNone && m_state.fading)
        {
            uint32_t fadeElapsed = now - m_state.fade_start_ms;
            if (fadeElapsed >= m_state.fade_time_ms)
            {
                m_state.cur_color = m_state.target_color;
                m_state.fading = false;
            }
            else if (m_state.fade_policy == FadePolicyEaseOutQuad)
            {
                uint32_t idx = (fadeElapsed * 80U) / m_state.fade_time_ms;
                if (idx > 80)
                    idx = 80;
                uint16_t t = kEaseQuadTable4096[idx];
                auto lerp = [&](uint16_t a, uint16_t b) {
                    return static_cast<uint16_t>(a + ((static_cast<int32_t>(b) - a) * t) / 4096);
                };
                m_state.cur_color.w = lerp(m_state.start_color.w, m_state.target_color.w);
                m_state.cur_color.r = lerp(m_state.start_color.r, m_state.target_color.r);
                m_state.cur_color.g = lerp(m_state.start_color.g, m_state.target_color.g);
                m_state.cur_color.b = lerp(m_state.start_color.b, m_state.target_color.b);
            }
            else
            {
                auto lerpLin = [&](uint16_t a, uint16_t b) {
                    return static_cast<uint16_t>(static_cast<int32_t>(a) +
                                                 (static_cast<int32_t>(b) - a) * static_cast<int32_t>(fadeElapsed) /
                                                     static_cast<int32_t>(m_state.fade_time_ms));
                };
                m_state.cur_color.w = lerpLin(m_state.start_color.w, m_state.target_color.w);
                m_state.cur_color.r = lerpLin(m_state.start_color.r, m_state.target_color.r);
                m_state.cur_color.g = lerpLin(m_state.start_color.g, m_state.target_color.g);
                m_state.cur_color.b = lerpLin(m_state.start_color.b, m_state.target_color.b);
            }
            ledChange = true;
            break;
        }
    } while (0);

    if (stepDone)
    {
        if (m_state.mix_lighting_effects)
            mixedEffectsService();
        else
            stopEffect();
        return;
    }

    if (ledChange)
        applyOutput(m_state.cur_color);
}

bool LightEngine::loadStateFromFlash()
{
    LightFlashMemory saved{};
    uint32_t         objType = 0;
    size_t           len = sizeof(saved);
    NvmStorage      &nvm = NvmStorage::instance();

    Ecode_t err = nvm.getObjectInfo(NVM3_KEY_LIGHT_MEMORY_DATA, &objType, &len);
    if (err != ECODE_NVM3_OK || objType != NVM3_OBJECTTYPE_DATA)
        return false;

    // 兼容旧版结构（color_index 字段）
    if (len == sizeof(LightFlashMemory))
    {
        err = nvm.readData(NVM3_KEY_LIGHT_MEMORY_DATA, &saved, sizeof(saved));
    }
    else
    {
        struct LegacyMem
        {
            uint8_t  is_on;
            uint8_t  brightness;
            uint8_t  color_index;
            uint8_t  color_source;
            uint16_t custom_w, custom_r, custom_g, custom_b;
        } __attribute__((packed)) legacy{};
        err = nvm.readData(NVM3_KEY_LIGHT_MEMORY_DATA, &legacy, len);
        if (err != ECODE_NVM3_OK)
            return false;
        saved.is_on = legacy.is_on;
        saved.brightness = legacy.brightness;
        saved.color_cycle_index = legacy.color_index;
        saved.color_source = legacy.color_source;
        saved.custom_w = legacy.custom_w;
        saved.custom_r = legacy.custom_r;
        saved.custom_g = legacy.custom_g;
        saved.custom_b = legacy.custom_b;
    }

    if (err != ECODE_NVM3_OK)
        return false;

    m_state.is_on = saved.is_on;
    m_state.brightness = clampU8(saved.brightness, 0, 255);
    m_state.color_cycle_index = clampU8(saved.color_cycle_index, 0, ColorLibrary::kCycleCount - 1);
    m_state.color_source = static_cast<ColorSource>(saved.color_source);
    m_state.raw_color.w = clampU16(saved.custom_w, 0, LED_HW_MAX);
    m_state.raw_color.r = clampU16(saved.custom_r, 0, LED_HW_MAX);
    m_state.raw_color.g = clampU16(saved.custom_g, 0, LED_HW_MAX);
    m_state.raw_color.b = clampU16(saved.custom_b, 0, LED_HW_MAX);
    m_state.history_brightness = m_state.brightness;

    if (m_state.color_source == ColorSourceIndexTable)
        m_state.raw_color = ColorLibrary::instance().colorAtCycleIndex(m_state.color_cycle_index);

    return true;
}

void LightEngine::saveStateToFlash()
{
    LightFlashMemory current{};
    LightFlashMemory old{};
    uint32_t         objType = 0;
    size_t           len = sizeof(current);
    NvmStorage      &nvm = NvmStorage::instance();

    current.is_on = m_state.is_on;
    current.brightness = m_state.brightness;
    current.color_cycle_index = m_state.color_cycle_index;
    current.color_source = static_cast<uint8_t>(m_state.color_source);
    current.custom_w = m_state.raw_color.w;
    current.custom_r = m_state.raw_color.r;
    current.custom_g = m_state.raw_color.g;
    current.custom_b = m_state.raw_color.b;

    Ecode_t err = nvm.getObjectInfo(NVM3_KEY_LIGHT_MEMORY_DATA, &objType, &len);
    if (err == ECODE_NVM3_OK && objType == NVM3_OBJECTTYPE_DATA && len != sizeof(current))
        nvm.deleteObject(NVM3_KEY_LIGHT_MEMORY_DATA);

    if (err == ECODE_NVM3_OK && objType == NVM3_OBJECTTYPE_DATA && len == sizeof(current))
    {
        err = nvm.readData(NVM3_KEY_LIGHT_MEMORY_DATA, &old, sizeof(old));
        if (err == ECODE_NVM3_OK && memcmp(&current, &old, sizeof(current)) == 0)
            return;
    }

    nvm.writeData(NVM3_KEY_LIGHT_MEMORY_DATA, &current, sizeof(current));
    nvm.repackIfNeeded();
}

bool LightEngine::isFirstCommissionDone() const
{
    return m_firstCommissionDone;
}

void LightEngine::setFirstCommissionDone()
{
    if (m_firstCommissionDone)
        return;
    m_firstCommissionDone = true;
    uint8_t flag = FIRST_COMMISSION_DONE_MAGIC;
    NvmStorage::instance().writeData(NVM3_KEY_FIRST_COMMISSION_DONE, &flag, sizeof(flag));
    NvmStorage::instance().repackIfNeeded();
}

bool LightEngine::isUserEffectIdle() const
{
    return !m_state.mix_lighting_effects && m_state.effect_mode == EffectModeNone && !m_state.fading;
}

void LightEngine::init()
{
    SILABS_LOG("[LightEngine] init");
    m_state.is_on = true;
    m_state.history_brightness = m_state.brightness = 255;
    m_state.color_cycle_index = ColorLibrary::kDefaultCycleIndex;
    m_state.change_origin = ChangeOriginUnknown;
    m_state.color_source = ColorSourceIndexTable;
    m_state.fading = false;
    m_state.low_battery_protected = false;
    m_state.raw_color = ColorLibrary::instance().colorAtCycleIndex(m_state.color_cycle_index);
    m_state.cur_color = {0, 0, 0, 0};

    uint8_t flag = 0;
    if (NvmStorage::instance().readData(NVM3_KEY_FIRST_COMMISSION_DONE, &flag, sizeof(flag)) == ECODE_NVM3_OK &&
        flag == FIRST_COMMISSION_DONE_MAGIC)
    {
        m_firstCommissionDone = true;
    }

    loadStateFromFlash();
    m_state.change_origin = ChangeOriginLocalKey;

    SILABS_LOG("[LightEngine] state on=%d bright=%u colorIdx=%u", m_state.is_on, m_state.brightness, m_state.color_cycle_index);

    if (m_state.is_on || m_state.brightness > 0)
        startFadeToCurrent(LED_FADE_COLOR_SWITCH_MS, FadePolicyFixedMs);
    else
    {
        m_state.cur_color = {0, 0, 0, 0};
        PwmHal::setMainWrgb(0, 0, 0, 0);
    }
}

uint8_t LightEngine::setLowBatteryProtection(bool protect)
{
    if (m_state.low_battery_protected == protect)
        return 0;

    SILABS_LOG("[LightEngine] low battery protect=%d", protect);
    m_state.low_battery_protected = protect;

    if (protect)
    {
        m_state.effect_mode = EffectModeNone;
        m_state.fading = false;
        m_state.cur_color = {0, 0, 0, 0};
        PwmHal::setMainWrgb(0, 0, 0, 0);
    }
    else if (m_state.is_on)
    {
        m_state.cur_color = {0, 0, 0, 0};
        m_state.start_color = {0, 0, 0, 0};
        startFadeToCurrent(LED_FADE_COLOR_SWITCH_MS, FadePolicyFixedMs);
        return 1;
    }
    else
    {
        m_state.cur_color = {0, 0, 0, 0};
        PwmHal::setMainWrgb(0, 0, 0, 0);
        return 3;
    }
    return 0;
}
