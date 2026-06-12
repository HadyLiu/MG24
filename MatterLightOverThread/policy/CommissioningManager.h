/**
 * @file CommissioningManager.h
 * @brief Matter 配网策略：自动/手动进入、窗口计时、首次白呼吸、配对成功动画
 * @layer Policy
 */
#pragma once

#include "LightEngine.h"
#include <stdint.h>

#ifndef COMMISSIONING_WINDOW_MS
#define COMMISSIONING_WINDOW_MS (15U * 1000U) ///< 原型 15s；量产改为 15*60*1000
#endif

/** @brief Matter 配网窗口与首次配网视觉反馈管理 */
class CommissioningManager
{
public:
    static CommissioningManager &instance();

    void tick10ms(uint32_t elapsedMs);
    void onLightTurnedOn();
    void onManualTrigger();
    void onCommissioningComplete();
    void stopFirstBreath();

    bool isWindowActive() const { return m_windowActive; }

private:
    CommissioningManager() = default;
    void enterCommissioningWindow();
    void restartWindowTimer();

    bool     m_windowActive = false;
    uint32_t m_windowStartMs = 0;
    bool     m_firstBreathActive = false;
};
