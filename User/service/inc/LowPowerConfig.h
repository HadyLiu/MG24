/**
 * @file LowPowerConfig.h
 * @brief 低功耗策略编译期开关
 * @author hady
 * @date 2026-07-30
 * @layer Service
 * @note 外设门控与芯片深睡分离：可先关 SPI/PWM/IADC，仍不进 EM2。
 */
#pragma once

#include <cstdint>

namespace LowPowerConfig
{
    /**
     * @brief 空闲后是否 Suspend 外设（关灯总线 / 指示灯 PWM / IADC 时钟）
     * @note true=进入 SleepPrepared 时关外设，唤醒/输出前 Resume；
     *       false=外设常备（仅跑状态机骨架）。
     */
    static constexpr bool kGatePeripheralsOnIdle = true;

    /**
     * @brief 是否进入芯片深层休眠（EM2 等）
     * @note 调试阶段保持 false；与外设门控独立。
     */
    static constexpr bool kEnterDeepSleepEnabled = false;

    /**
     * @brief 空闲后延迟再 Suspend（ms）
     * @note 防抖：避免灯灭瞬间误关总线。
     */
    static constexpr uint16_t kIdleSettleMs = 3000U;
}
