/**
 * @file LowPowerCoordinator.cpp
 * @brief 应用层低功耗协调器实现
 * @author hady
 * @date 2026-07-30
 * @layer Service
 */
#include "LowPowerCoordinator.h"

#include "BspPeripheralSleep.h"
#include "BspPowerMonitor.h"
#include "BspTimer.h"
#include "DebugLog.h"
#include "LowPowerConfig.h"

#ifndef APP_LOG_CAT_LP
#define APP_LOG_CAT_LP 1
#endif
#define LOG_LP(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_LP, fmt, ##__VA_ARGS__)

/** @brief 用户交互保持时长（ms），超时后清 Button/UserMatter 位 */
static constexpr uint16_t kUserWakeHoldMs = 2000U;

/** @brief Matter 下行控制保持时长（ms），覆盖连续调光/色温指令间隔 */
static constexpr uint16_t kMatterControlWakeHoldMs = 8000U;

/** @brief 协调器 Poll 周期（与电源轮询同量级） */
static constexpr uint16_t kLpPollIntervalMs = 200U;

/** @brief 低功耗评估定时器 */
static BspTimer s_lpPollTimer;

/**
 * @brief 定时器桥接（禁止 Init 内写 Lambda）
 */
static void LpPollTimerBridge(uint16_t elapsedMs)
{
    LowPowerCoordinator::Instance().Poll(elapsedMs);
}

static uint8_t ToMask(LowPowerCoordinator::HoldReason reason)
{
    return static_cast<uint8_t>(reason);
}

/**
 * @brief 初始化协调器
 * @note kGatePeripheralsOnIdle=true 时空闲会 Suspend 外设；深睡开关独立。
 */
void LowPowerCoordinator::Init()
{
    m_runState       = RunState::Active;
    m_holdMask       = 0U;
    m_idleElapsedMs  = 0U;
    m_userWakeHoldMs = 0U;
    m_usbFallbackMs  = 0U;

    BspPeripheralSleep::Instance().MarkReadyAfterBoot();

    if (!LowPowerConfig::kGatePeripheralsOnIdle)
    {
        SetHoldRaw(HoldReason::DebugForce, true);
        LOG_LP("[LP] Init Active, peripheral-gate DISABLED");
    }
    else
    {
        LOG_LP("[LP] Init Active, peripheral-gate ENABLED (deep-sleep=%u)",
               static_cast<unsigned>(LowPowerConfig::kEnterDeepSleepEnabled));
    }

    SetRunStateRaw(RunState::Active);

    s_lpPollTimer.Init(LpPollTimerBridge, kLpPollIntervalMs, nullptr);
    s_lpPollTimer.TurnOnOff(true);
}

/**
 * @brief 主灯活动 → 保持位
 */
void LowPowerCoordinator::OnMainLightActivity(bool isActive)
{
    SetHoldRaw(HoldReason::MainLight, isActive);
    if (isActive)
    {
        EnterActiveRaw();
    }
    EvaluateSleepRaw();
}

/**
 * @brief 指示灯活动 → 保持位
 */
void LowPowerCoordinator::OnIndicatorActivity(bool isActive)
{
    SetHoldRaw(HoldReason::Indicator, isActive);
    if (isActive)
    {
        EnterActiveRaw();
    }
    EvaluateSleepRaw();
}

/**
 * @brief USB 活动 → 保持位
 * @param allowPeripheralIo false 时禁止 Suspend/Resume（供 GPIO ISR 调用）
 */
void LowPowerCoordinator::OnUsbPowerActivity(bool usbPresent, bool allowPeripheralIo)
{
    SetHoldRaw(HoldReason::UsbPower, usbPresent);
    if (!allowPeripheralIo)
    {
        if (usbPresent)
        {
            m_idleElapsedMs = 0U;
            SetRunStateRaw(RunState::Active);
        }
        else if (!HasBlockingHoldRaw() && (m_runState == RunState::Active))
        {
            /* 仅进入空闲结算，Suspend 留给 LP Poll */
            m_idleElapsedMs = 0U;
            SetRunStateRaw(RunState::IdleSettle);
        }
        return;
    }

    if (usbPresent)
    {
        EnterActiveRaw();
    }
    EvaluateSleepRaw();
}

/**
 * @brief 用户交互短暂唤醒
 */
void LowPowerCoordinator::RequestUserWake()
{
    SetHoldRaw(HoldReason::Button, true);
    SetHoldRaw(HoldReason::UserMatter, true);
    m_userWakeHoldMs = kUserWakeHoldMs;
    EnterActiveRaw();
    EvaluateSleepRaw();
}

/**
 * @brief Matter Hub 下行：立即 Resume 外设并延长 UserMatter 保持
 */
void LowPowerCoordinator::RequestMatterControlWake()
{
    EnsurePeripheralsReady();
    SetHoldRaw(HoldReason::UserMatter, true);
    if (m_userWakeHoldMs < kMatterControlWakeHoldMs)
    {
        m_userWakeHoldMs = kMatterControlWakeHoldMs;
    }
    EnterActiveRaw();
    EvaluateSleepRaw();
}

/**
 * @brief BLE 配网会话保持：防止 3s 空闲 Suspend 外设干扰长时配网
 */
void LowPowerCoordinator::SetCommissioningHold(bool enable)
{
    SetHoldRaw(HoldReason::Commissioning, enable);
    if (enable)
    {
        EnterActiveRaw();
    }
    EvaluateSleepRaw();
}

/**
 * @brief 周期评估空闲计时；并做 USB ADC 1s 兜底（分压脚数字沿可能不够陡）
 */
void LowPowerCoordinator::Poll(uint16_t elapsedMs)
{
    TickUserWakeHoldRaw(elapsedMs);

    m_usbFallbackMs = static_cast<uint16_t>(m_usbFallbackMs + elapsedMs);
    if (m_usbFallbackMs >= 1000U)
    {
        m_usbFallbackMs = 0U;
        BspPowerMonitor::Instance().PollUsbStatusRaw();
        OnUsbPowerActivity(BspPowerMonitor::Instance().GetUsbStatus() ==
                               UsbConnectionStatusEnum::UsbConnected,
                           true);
    }

    if (m_runState == RunState::IdleSettle)
    {
        m_idleElapsedMs =
            static_cast<uint16_t>(m_idleElapsedMs + elapsedMs);
        if (m_idleElapsedMs >= LowPowerConfig::kIdleSettleMs)
        {
            EvaluateSleepRaw();
        }
    }
}

/**
 * @brief 输出前确保外设可用
 */
void LowPowerCoordinator::EnsurePeripheralsReady()
{
    BspPeripheralSleep::Instance().EnsureReadyBeforeOutput();
}

/**
 * @brief 设置/清除保持位
 */
void LowPowerCoordinator::SetHoldRaw(HoldReason reason, bool enable)
{
    const uint8_t bit = ToMask(reason);
    if (enable)
    {
        m_holdMask = static_cast<uint8_t>(m_holdMask | bit);
    }
    else
    {
        m_holdMask = static_cast<uint8_t>(m_holdMask & static_cast<uint8_t>(~bit));
    }
}

/**
 * @brief 状态切换唯一入口
 */
void LowPowerCoordinator::SetRunStateRaw(RunState nextState)
{
    if (m_runState == nextState)
    {
        return;
    }

    LOG_LP("[LP] state %u -> %u hold=0x%02X",
           static_cast<unsigned>(m_runState),
           static_cast<unsigned>(nextState),
           static_cast<unsigned>(m_holdMask));
    m_runState = nextState;
}

/**
 * @brief 是否存在阻止休眠的保持源
 */
bool LowPowerCoordinator::HasBlockingHoldRaw() const
{
    return m_holdMask != ToMask(HoldReason::None);
}

/**
 * @brief 进入 Active：必要时 Resume 外设
 */
void LowPowerCoordinator::EnterActiveRaw()
{
    if (BspPeripheralSleep::Instance().IsSuspended())
    {
        BspPeripheralSleep::Instance().ResumeFromSleep();
    }
    m_idleElapsedMs = 0U;
    SetRunStateRaw(RunState::Active);
}

/**
 * @brief 进入 SleepPrepared：按配置 Suspend 外设
 * @note 深睡（EM2）由 kEnterDeepSleepEnabled 另控，当前不进。
 */
void LowPowerCoordinator::EnterSleepPreparedRaw()
{
    if (LowPowerConfig::kGatePeripheralsOnIdle)
    {
        BspPeripheralSleep::Instance().SuspendForSleep();
    }

    SetRunStateRaw(RunState::SleepPrepared);

    if (LowPowerConfig::kEnterDeepSleepEnabled)
    {
        /* 预留：后续接 power_manager / ICD 深睡入口 */
        LOG_LP("[LP] deep-sleep requested (not wired yet)");
    }
}

/**
 * @brief 统一评估是否可进入空闲挂起（唯一决策入口）
 */
void LowPowerCoordinator::EvaluateSleepRaw()
{
    if (!LowPowerConfig::kGatePeripheralsOnIdle)
    {
        /* 外设门控关闭：永不 Suspend，始终保持 Active */
        EnterActiveRaw();
        return;
    }

    if (HasBlockingHoldRaw())
    {
        EnterActiveRaw();
        return;
    }

    if (m_runState == RunState::SleepPrepared)
    {
        return;
    }

    if (m_runState != RunState::IdleSettle)
    {
        m_idleElapsedMs = 0U;
        SetRunStateRaw(RunState::IdleSettle);
        return;
    }

    if (m_idleElapsedMs < LowPowerConfig::kIdleSettleMs)
    {
        return;
    }

    EnterSleepPreparedRaw();
}

/**
 * @brief 用户唤醒保持倒计时
 */
void LowPowerCoordinator::TickUserWakeHoldRaw(uint16_t elapsedMs)
{
    if (m_userWakeHoldMs == 0U)
    {
        return;
    }

    if (elapsedMs >= m_userWakeHoldMs)
    {
        m_userWakeHoldMs = 0U;
        SetHoldRaw(HoldReason::Button, false);
        SetHoldRaw(HoldReason::UserMatter, false);
        EvaluateSleepRaw();
        return;
    }

    m_userWakeHoldMs = static_cast<uint16_t>(m_userWakeHoldMs - elapsedMs);
}
