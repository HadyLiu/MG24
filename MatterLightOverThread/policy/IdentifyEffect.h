/**
 * @file IdentifyEffect.h
 * @brief Matter Identify 灯效策略：400/400ms 闪烁，结束 800ms 恢复原态
 * @layer Policy
 */
#pragma once

#include "LightEngine.h"

/** @brief 设备识别（Identify）期间的灯光特效与恢复 */
class IdentifyEffect
{
public:
    static IdentifyEffect &instance();

    void onStart();
    void onStop();

private:
    IdentifyEffect() = default;
    LightSnapshot m_saved{};
    bool          m_active = false;
};
