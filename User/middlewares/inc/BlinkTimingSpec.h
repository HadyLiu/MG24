/**
 * @file BlinkTimingSpec.h
 * @brief PRD「注」闪烁周期与重置网络时序规格
 * @layer Middleware
 * @note GetBlink 为 50/50 占空：周期 C → 开 C/2、关 C/2。
 */
#pragma once

#include <cstdint>

namespace BlinkTimingSpec
{
    /** @brief 快闪：400ms 周期（200ms 开 → 200ms 关） */
    static constexpr uint16_t kFastBlinkCycleMs = 400U;

    /** @brief 正常闪：800ms 周期（400ms 开 → 400ms 关） */
    static constexpr uint16_t kNormalBlinkCycleMs = 800U;

    /** @brief 慢闪：2400ms 周期（1200ms 开 → 1200ms 关） */
    static constexpr uint16_t kSlowBlinkCycleMs = 2400U;

    /** @brief 重置网络：起始熄灭时长 */
    static constexpr uint16_t kResetOffLeadMs = 400U;

    /** @brief 重置网络：结尾熄灭时长 */
    static constexpr uint16_t kResetOffTailMs = 2000U;

    /** @brief 重置网络：正常闪额外重复次数（共 3 次，repeatCount=2） */
    static constexpr uint8_t kResetNormalBlinkExtraRepeats = 2U;
}
