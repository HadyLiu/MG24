/**
 * @file LightDimmingSpec.h
 * @brief §1 调光/渐变规格（PRD 第一部分）
 * @layer Middleware
 * @note 网关 1% 下限仅在 LightEffectEngine 物理 PWM 输出侧钳位，不修改 Matter MinLevel/ZAP。
 */
#pragma once

#include <cstdint>

namespace LightDimmingSpec
{
    static constexpr uint16_t kFadeInMs                = 200U;
    static constexpr uint16_t kFadeOutMs               = 400U;
    static constexpr uint16_t kUsbUnplugFadeInMs       = 400U; /**< 注解10：拔 USB 后淡入 */
    static constexpr uint8_t  kHubMinBrightnessPercent = 1U;

    /** @brief 物理 PWM 最低亮度：255 × 1% 向上取整（≈5lm 产品标定） */
    static constexpr uint8_t kMinPhysicalBrightness255 =
        static_cast<uint8_t>((255U * kHubMinBrightnessPercent + 99U) / 100U);

    /** @brief Matter CurrentLevel(0~254) → 驱动逻辑亮度(0~255)，不做下限钳位 */
    inline uint8_t MatterLevelToDriverBrightness(uint8_t matterLevel)
    {
        if (matterLevel == 0U)
        {
            return 0U;
        }

        return static_cast<uint8_t>((static_cast<uint16_t>(matterLevel) * 255U + 127U) / 254U);
    }

    /** @brief 物理输出钳位：开灯态 PWM 不低于 1%；0 仍为关 */
    inline uint8_t ClampPhysicalBrightness(uint8_t brightness)
    {
        if (brightness == 0U)
        {
            return 0U;
        }

        return (brightness < kMinPhysicalBrightness255) ? kMinPhysicalBrightness255 : brightness;
    }
}
