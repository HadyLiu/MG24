/**
 * @file LightDimmingSpec.h
 * @brief §1 / §17 调光规格（PRD + 注解）
 * @layer Middleware
 * @note 逻辑亮度下限与物理占空比映射仅在 LightEffectEngine 输出侧生效，
 *       不修改 Matter MinLevel/ZAP。
 */
#pragma once

#include <cstdint>

namespace LightDimmingSpec
{
    static constexpr uint16_t kFadeInMs                = 200U;
    static constexpr uint16_t kFadeOutMs               = 400U;
    static constexpr uint16_t kUsbUnplugFadeInMs       = 400U; /**< 注解10：拔 USB 后淡入 */
    /** @brief Matter 已开灯仅调 Level：1ms 即时跟手（避免 200ms 叠加网络延迟） */
    static constexpr uint16_t kMatterLevelOnlyTransitionMs = 1U;
    static constexpr uint8_t  kHubMinBrightnessPercent = 1U;
    /** @brief §17：开灯态物理占空比下限百分比 */
    static constexpr uint8_t  kMinPhysicalDutyPercent  = 10U;
    static constexpr uint8_t  kMaxBrightness255        = 255U;

    /** @brief 逻辑亮度下限：255 × 1% 向上取整（§1.3 / ≈5lm） */
    static constexpr uint8_t kMinPhysicalBrightness255 =
        static_cast<uint8_t>((255U * kHubMinBrightnessPercent + 99U) / 100U);

    /** @brief §17：物理占空比缩放下限：255 × 10% 向上取整 */
    static constexpr uint8_t kMinPhysicalDutyScale255 =
        static_cast<uint8_t>((255U * kMinPhysicalDutyPercent + 99U) / 100U);

    /** @brief Matter CurrentLevel(0~254) → 驱动逻辑亮度(0~255)，不做下限钳位 */
    inline uint8_t MatterLevelToDriverBrightness(uint8_t matterLevel)
    {
        if (matterLevel == 0U)
        {
            return 0U;
        }

        return static_cast<uint8_t>((static_cast<uint16_t>(matterLevel) * 255U + 127U) / 254U);
    }

    /** @brief 逻辑亮度钳位：开灯态不低于 1%；0 仍为关 */
    inline uint8_t ClampPhysicalBrightness(uint8_t brightness)
    {
        if (brightness == 0U)
        {
            return 0U;
        }

        return (brightness < kMinPhysicalBrightness255) ? kMinPhysicalBrightness255 : brightness;
    }

    /**
     * @brief §17：逻辑亮度 → 物理占空比缩放（0~255）
     * @note 0→0；亮度 [1%,100%] 线性映射到占空比 [10%,100%]
     */
    inline uint8_t MapBrightnessToDutyScale(uint8_t brightness)
    {
        if (brightness == 0U)
        {
            return 0U;
        }

        const uint8_t clamped = ClampPhysicalBrightness(brightness);
        if (clamped >= kMaxBrightness255)
        {
            return kMaxBrightness255;
        }

        const uint16_t briSpan  = static_cast<uint16_t>(kMaxBrightness255 - kMinPhysicalBrightness255);
        const uint16_t dutySpan = static_cast<uint16_t>(kMaxBrightness255 - kMinPhysicalDutyScale255);
        const uint16_t offset   = static_cast<uint16_t>(clamped - kMinPhysicalBrightness255);

        return static_cast<uint8_t>(kMinPhysicalDutyScale255 +
                                    (offset * dutySpan + (briSpan / 2U)) / briSpan);
    }
}
