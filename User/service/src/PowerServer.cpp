/**
 * @file PowerServer.cpp
 * @brief 电源服务实现：USB/电池双模式、充电安全、电量轮询
 * @author hady
 * @date 2026-06-26
 *
 * @details 模块五段分区
 * ┌──────────────────────────────────────────────────────────────┐
 * │ §1 定时器     周期驱动 PowerPoll，按运行状态按需启停           │
 * │ §2 BSP 桥接   读：FetchPowerMonitorSnapshotRaw 唯一漏斗      │
 * │               写：SetBatteryOut/ChargeEnableRaw 独立出口     │
 * │ §3 回调注册   entry 接线：电量 / 充电灯 / PWM 刷新             │
 * │ §4 对外 API   薄封装，转发至 Raw 实现                         │
 * │ §5 主逻辑     显式状态机 + Settle/会话计时 + 充放电策略       │
 * └──────────────────────────────────────────────────────────────┘
 *
 * @details 单入口收拢（.cursorrules）
 * - FetchPowerMonitorSnapshotRaw() 是唯一 BSP 读入口。
 * - 电池模式：只读 USB + 放电且 settle 结束后的电压。
 * - USB 模式：读温度/电压/充电态/充电芯片。
 *
 * 状态机转换（RefreshRunStateRaw）：
 *   USB 接入     → UsbChargeManage
 *   电池 + 灯亮  → BatteryDischarging
 *   电池 + 灯灭  → BatteryIdle
 */
#include "PowerServer.h"
#include "DebugLog.h"
#include "LowPowerCoordinator.h"
#include <cassert>

/////////////////////////////////////////////////////////////////
// §1 定时器
/////////////////////////////////////////////////////////////////

/** @brief 电源轮询硬件定时器（周期 kPollIntervalMs） */
static BspTimer s_pollTimer;

/**
 * @brief 定时器到期桥接，转发至 PowerPoll
 * @note 静态成员实现，禁止 Init 内写 Lambda
 */
void PowerServer::PollTimerBridgeImpl(uint16_t elapsedMs)
{
    PowerServer::Instance().PowerPoll(elapsedMs);
}

/** @brief USB 拔插中断桥接，转发至 OnUsbConnectionChangedRaw */
void PowerServer::UsbNotifyBridgeImpl(UsbConnectionStatusEnum usbStatus)
{
    PowerServer::Instance().OnUsbConnectionChangedRaw(usbStatus);
}

/**
 * @brief 启动或停止电源轮询定时器
 * @param enable true=启动, false=停止
 */
void PowerServer::StartPollTimerRaw(bool enable)
{
    if (enable)
    {
        s_pollTimer.TurnOnOff(true);
    }
    else if (s_pollTimer.IsRunning())
    {
        s_pollTimer.TurnOnOff(false);
    }
}

/**
 * @brief 根据运行状态同步轮询定时器启停
 * @note UsbChargeManage / BatteryDischarging 需轮询；BatteryIdle 停表省电
 */
void PowerServer::SyncPollTimerFromRunStateRaw()
{
    const bool needTimer =
        (m_runState == PowerRunState::UsbChargeManage) || (m_runState == PowerRunState::BatteryDischarging);

    if (needTimer == m_pollTimerEnabled)
    {
        return;
    }

    m_pollTimerEnabled = needTimer;
    StartPollTimerRaw(m_pollTimerEnabled);
    LOG_BAT("Poll timer %s, runState=%u", m_pollTimerEnabled ? "on" : "off", static_cast<uint8_t>(m_runState));
}

/////////////////////////////////////////////////////////////////
// §2 BSP 桥接（读：唯一漏斗；写：独立出口）
/////////////////////////////////////////////////////////////////

/**
 * @brief 【唯一硬件读取入口】按供电模式按需收拢 BSP 数据
 * @note 电池模式：USB 由 EXTI 维护（Fetch 仅 GPIO 漏沿同步）+ 放电且 settle 结束后读电压。
 * @note USB 模式：温度/电压/充电使能/充电芯片状态全读。
 * @note assert(!m_fetchInProgress) 防重入。
 */
void PowerServer::FetchPowerMonitorSnapshotRaw()
{
    assert(!m_fetchInProgress);
    m_fetchInProgress = true;

    BspPowerMonitor& monitor = BspPowerMonitor::Instance();
    /* USB：EXTI 主路径；此处 GPIO 同步防漏沿，不再做 ADC */
    monitor.PollUsbStatusRaw();

    PowerMonitorSnapshot snapshot{};
    snapshot.usbStatus = monitor.GetUsbStatus();
    m_supplyMode =
        (snapshot.usbStatus == UsbConnectionStatusEnum::UsbConnected) ? SupplyMode::UsbPowered : SupplyMode::Battery;

    if (m_supplyMode == SupplyMode::UsbPowered)
    {
        snapshot.tempStatus    = monitor.GetBatteryTempStatus();
        snapshot.voltStatus    = monitor.GetBatteryVoltStatus();
        snapshot.chargeEnabled = monitor.IsChargeEnabled();

        if (snapshot.chargeEnabled)
        {
            snapshot.chipStatus = monitor.GetChargeStatus();
        }
        else
        {
            snapshot.chipStatus = ChargeChipStatusEnum::CHARGE_INIT;
        }
    }
    else
    {
        // 电池模式：不读 NTC/充电芯片，避免多余 ADC 与充电通路操作
        snapshot.tempStatus    = BatteryTempStatusEnum::TEMP_NORMAL;
        snapshot.chargeEnabled = false;
        snapshot.chipStatus    = ChargeChipStatusEnum::CHARGE_INIT;

        if (m_batteryVoltPollEnabled && (m_batterySettleMs == 0U))
        {
            snapshot.voltStatus = monitor.GetBatteryVoltStatus();
        }
        else
        {
            snapshot.voltStatus = BatteryVoltStatusEnum::VOLT_NORMAL;
        }
    }

    m_powerSnapshot      = snapshot;
    m_powerSnapshotValid = true;
    m_fetchInProgress    = false;
    LOG_POWER("Fetch snapshot: usb=%u, temp=%u, volt=%u, chip=%u, chargeEn=%d",
              static_cast<uint8_t>(snapshot.usbStatus), static_cast<uint8_t>(snapshot.tempStatus),
              static_cast<uint8_t>(snapshot.voltStatus), static_cast<uint8_t>(snapshot.chipStatus),
              snapshot.chargeEnabled);
}

/**
 * @brief 获取本轮已收拢的电源监测快照（只读）
 * @note 必须先 Fetch；assert 保护未初始化与 Fetch 重入
 */
const PowerServer::PowerMonitorSnapshot& PowerServer::GetPowerSnapshotRaw() const
{
    assert(m_powerSnapshotValid);
    assert(!m_fetchInProgress);
    return m_powerSnapshot;
}

/**
 * @brief 判断当前时刻充电芯片状态是否可信
 * @note 充电已使能且 settle 倒计时归零；Fetch 后由本函数结合 m_chargeSettleMs 判定
 */
bool PowerServer::IsChargeChipReadableNowRaw(const PowerMonitorSnapshot& snapshot) const
{
    return snapshot.chargeEnabled && (m_chargeSettleMs == 0U);
}

/** @brief 控制电池对外放电通路 */
void PowerServer::SetBatteryOutEnableRaw(bool enable)
{
    BspPowerMonitor::Instance().SetBatteryOutEnable(enable);
}

/**
 * @brief 控制充电使能与快慢充档位
 * @param fastCharge 1=快充(灯灭), 0=慢充(灯亮)
 */
void PowerServer::SetBatteryChargeEnableRaw(bool enable, uint8_t fastCharge)
{
    BspPowerMonitor::Instance().SetBatteryChargeEnable(enable, fastCharge);
}

/////////////////////////////////////////////////////////////////
// §3 回调注册
/////////////////////////////////////////////////////////////////

/** @brief 注册电池电量等级变化回调 */
void PowerServer::RegisterBatteryVoltHandler(BatteryVoltHandler handler)
{
    m_batteryVoltHandler = handler;
}

/**
 * @brief 注册充电综合状态变化回调
 * @note Init 早于 entry 接线时，USB 模式下此处补发当前快照
 */
void PowerServer::RegisterChargeStatusHandler(ChargeStatusHandler handler)
{
    m_chargeStatusHandler = handler;

    if ((handler == nullptr) || (m_supplyMode != SupplyMode::UsbPowered))
    {
        return;
    }

    FetchPowerMonitorSnapshotRaw();
    RefreshChargeSnapshotRaw();
    handler(m_currentChargeSnapshot);
    m_lastReportedChargeSnapshot = m_currentChargeSnapshot;
    m_chargeSnapshotValid        = true;
}

/** @brief 注册供电通路就绪回调 */
void PowerServer::RegisterLightPowerPathReadyHandler(LightPowerPathReadyHandler handler)
{
    m_powerPathReadyHandler = handler;
}

void PowerServer::RegisterUsbUnplugLightPrepareHandler(UsbUnplugLightPrepareHandler handler)
{
    m_usbUnplugPrepareHandler = handler;
}

/////////////////////////////////////////////////////////////////
// §4 对外 API（薄封装）
/////////////////////////////////////////////////////////////////

void PowerServer::OnLightOutputChanged(bool mainLightActive)
{
    OnMainLightChangedRaw(mainLightActive);
}

void PowerServer::PowerUsbIsr(UsbConnectionStatusEnum usbStatus)
{
    OnUsbConnectionChangedRaw(usbStatus);
}

/////////////////////////////////////////////////////////////////
// §5 主逻辑
/////////////////////////////////////////////////////////////////

// ----- §5.1 Settle / 会话计时（与硬件控制隔离）-----

/**
 * @brief 通用 Settle 倒计时递减
 * @note 独立于 GPIO，供电池电压/充电芯片防抖共用
 */
void PowerServer::TickSettleCountdownRaw(uint16_t& counterMs, uint16_t elapsedMs)
{
    if (counterMs == 0U)
    {
        return;
    }

    if (elapsedMs >= counterMs)
    {
        counterMs = 0U;
        return;
    }

    counterMs = static_cast<uint16_t>(counterMs - elapsedMs);
}

/** @brief 重置 USB 充电会话计时与超时标志 */
void PowerServer::ResetChargingSessionRaw()
{
    m_chargeSessionElapsedMs = 0U;
    m_chargeSessionTimeout   = false;
}

/**
 * @brief 累计 USB 充电会话时长
 * @note 满 kChargeSessionLimitMs(8h) 置超时；芯片 DONE/FAULT 时重置
 */
void PowerServer::TickChargingSessionRaw(uint16_t elapsedMs)
{
    if (m_supplyMode != SupplyMode::UsbPowered)
    {
        ResetChargingSessionRaw();
        return;
    }

    const PowerMonitorSnapshot& snapshot = GetPowerSnapshotRaw();

    if (m_chargeSessionTimeout)
    {
        return; // 充满不需要再增加
    }
    if (!snapshot.chargeEnabled || !IsChargeChipReadableNowRaw(snapshot))
    {
        // 充电异常 需要清除计时数据
        m_chargeSessionElapsedMs = 0U;
        return;
    }

    if (snapshot.chipStatus == ChargeChipStatusEnum::CHARGE_DONE)
    {
        ResetChargingSessionRaw();
        return;
    }

    if (snapshot.chipStatus == ChargeChipStatusEnum::CHARGE_FAULT)
    {
        return;
    }

    m_chargeSessionElapsedMs += static_cast<uint32_t>(elapsedMs);
    if (m_chargeSessionElapsedMs >= kChargeSessionLimitMs)
    {
        m_chargeSessionTimeout = true;
    }
    LOG_BAT("Charge session %u s", m_chargeSessionElapsedMs / 1000);
}

// ----- §5.2 状态映射（纯函数，只读快照，无副作用）-----

/** @brief BSP 电压分级 → Service 层电量等级 */
BatteryVoltLevel PowerServer::MapVoltLevelRaw(BatteryVoltStatusEnum status)
{
    switch (status)
    {
    case BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY:
        return BatteryVoltLevel::CriticalEmpty;
    case BatteryVoltStatusEnum::VOLT_LOW_WARNING:
        return BatteryVoltLevel::LowWarning;
    case BatteryVoltStatusEnum::VOLT_NORMAL:
    default:
        return BatteryVoltLevel::Normal;
    }
}

/**
 * @brief 按优先级表仲裁充电综合状态（EvaluateChargeRaw 内部使用）
 */
BatteryChargeStatus PowerServer::ResolveChargeStatusFromSnapshotRaw(const PowerMonitorSnapshot& snapshot,
                                                                    bool chipValid, bool chargeFaultLatched,
                                                                    bool chargeSessionTimeout)
{
    if (snapshot.tempStatus == BatteryTempStatusEnum::TEMP_BATTERY_REMOVED)
    {
        return BatteryChargeStatus::Nobat;
    }
    if (chargeFaultLatched)
    {
        return BatteryChargeStatus::ChargeFault;
    }
    if (snapshot.tempStatus == BatteryTempStatusEnum::TEMP_TOO_HIGH)
    {
        return BatteryChargeStatus::TempFault;
    }
    if (chipValid && (snapshot.chipStatus == ChargeChipStatusEnum::CHARGE_FAULT))
    {
        return BatteryChargeStatus::ChargeFault;
    }
    if (chipValid && (snapshot.voltStatus == BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY))
    {
        return BatteryChargeStatus::CriticalEmpty;
    }
    if (chipValid && (snapshot.chipStatus == ChargeChipStatusEnum::CHARGING))
    {
        return BatteryChargeStatus::Charging;
    }
    if (chipValid && (snapshot.chipStatus == ChargeChipStatusEnum::CHARGE_DONE))
    {
        return BatteryChargeStatus::ChargeDone;
    }
    if (chargeSessionTimeout)
    {
        return BatteryChargeStatus::ChargeDone;
    }
    if (snapshot.chargeEnabled)
    {
        return BatteryChargeStatus::Charging;
    }
    return BatteryChargeStatus::Idle;
}

/**
 * @brief 由展示状态推导是否允许开充
 * @note 仅 Charging / LowWarning / Idle 需要保持或尝试开启充电
 */
bool PowerServer::DeriveAllowChargeFromStatusRaw(BatteryChargeStatus status)
{
    switch (status)
    {
    case BatteryChargeStatus::Charging:
    case BatteryChargeStatus::Idle:
        return true;
    default:
        return false;
    }
}

/**
 * @brief 充电综合评估：状态 + 是否开充，一次算出
 * @note 指示灯仲裁由 IndicatorServer 负责。
 */
PowerServer::ChargeEvaluation PowerServer::EvaluateChargeRaw(const PowerMonitorSnapshot& snapshot) const
{
    ChargeEvaluation eval{};
    eval.fastChargeFlag = m_mainLightActive ? 0U : 1U;

    /* 根据当前快充状态和主灯状态决定快速充电标志 */
    const bool chipValid = IsChargeChipReadableNowRaw(snapshot);
    /* 根据芯片有效性、故障状态和会话超时状态决定充电状态 */
    eval.status = ResolveChargeStatusFromSnapshotRaw(snapshot, chipValid, m_chargeFaultLatched, m_chargeSessionTimeout);
    eval.allowCharge = DeriveAllowChargeFromStatusRaw(eval.status);

    LOG_BAT("EvaluateCharge: status=%u, allowCharge=%d, fastChargeFlag=%u", static_cast<uint8_t>(eval.status),
            eval.allowCharge, eval.fastChargeFlag);
    return eval;
}

/** @brief 将 ChargeEvaluation 写入 m_currentChargeSnapshot */
void PowerServer::ApplyChargeSnapshotFromEvalRaw(const ChargeEvaluation& eval, const PowerMonitorSnapshot& snapshot,
                                                 bool chipValid)
{
    m_currentChargeSnapshot.status        = eval.status;
    m_currentChargeSnapshot.useFastCharge = (eval.fastChargeFlag != 0U);
    m_currentChargeSnapshot.chipValid     = chipValid;
    m_currentChargeSnapshot.chipCharging  = chipValid && (snapshot.chipStatus == ChargeChipStatusEnum::CHARGING);
}

// ----- §5.3 状态机转换（集中入口）-----

/** @brief 切换运行状态并同步轮询定时器 */
void PowerServer::SetRunStateRaw(PowerRunState nextState)
{
    if (m_runState == nextState)
    {
        return;
    }

    LOG_BAT("RunState %u -> %u", static_cast<uint8_t>(m_runState), static_cast<uint8_t>(nextState));
    m_runState = nextState;
    SyncPollTimerFromRunStateRaw();
}

/** @brief 根据供电来源与主灯状态推导运行状态 */
void PowerServer::RefreshRunStateRaw()
{
    if (m_supplyMode == SupplyMode::UsbPowered)
    {
        SetRunStateRaw(PowerRunState::UsbChargeManage);
        return;
    }

    if (m_mainLightActive)
    {
        SetRunStateRaw(PowerRunState::BatteryDischarging);
    }
    else
    {
        SetRunStateRaw(PowerRunState::BatteryIdle);
    }
}

/** @brief 标记 ISR 后需在 Poll 中刷新硬件策略 */
void PowerServer::RequestSupplyApplyRaw()
{
    m_pendingSupplyApply = true;
}

// ----- §5.4 电池放电控制 -----

/** @brief 开启电池放电通路，启动电压 settle 防抖 */
void PowerServer::EnableBatteryDischargeRaw()
{
    if (m_batteryOutEnabled)
    {
        return;
    }

    SetBatteryOutEnableRaw(true);
    m_batteryOutEnabled = true;
    m_batterySettleMs   = kBatterySettleMs;
    LOG_BAT("Battery discharge enabled");
}

/** @brief 关闭电池放电通路 */
void PowerServer::DisableBatteryDischargeRaw()
{
    m_batterySettleMs = 0U;
    if (!m_batteryOutEnabled)
    {
        return;
    }

    SetBatteryOutEnableRaw(false);
    m_batteryOutEnabled = false;
    LOG_BAT("Battery discharge disabled");
}

/** @brief 按主灯状态同步放电硬件与电压轮询许可 */
void PowerServer::ApplyBatteryDischargeRaw()
{
    m_batteryVoltPollEnabled = m_mainLightActive;

    if (m_mainLightActive)
    {
        EnableBatteryDischargeRaw();
    }
    else
    {
        DisableBatteryDischargeRaw();
    }
}

// ----- §5.5 充电控制（一次评估 → 写硬件 + 更新指示灯快照）-----

/** @brief 判断是否应锁存充电故障（需芯片可读） */
bool PowerServer::ShouldLatchChargeFaultRaw(const PowerMonitorSnapshot& snapshot) const
{
    return IsChargeChipReadableNowRaw(snapshot) && (snapshot.chipStatus == ChargeChipStatusEnum::CHARGE_FAULT);
}

/** @brief 根据充电使能变化更新 settle 倒计时 */
void PowerServer::UpdateChargeSettleAfterDecisionRaw(bool allowCharge, bool wasChargeEnabled)
{
    if (allowCharge && !wasChargeEnabled)
    {
        m_chargeSettleMs = kChargeSettleMs;
        return;
    }

    if (!allowCharge)
    {
        m_chargeSettleMs = 0U;
    }
}

/** @brief 基于 EvaluateChargeRaw 刷新 m_currentChargeSnapshot */
void PowerServer::RefreshChargeSnapshotRaw()
{
    const PowerMonitorSnapshot& snapshot  = GetPowerSnapshotRaw();
    const bool                  chipValid = IsChargeChipReadableNowRaw(snapshot);
    ApplyChargeSnapshotFromEvalRaw(EvaluateChargeRaw(snapshot), snapshot, chipValid);
}

/**
 * @brief 充电控制主流程：锁存故障 → 一次评估 → 写硬件 + 更新快照
 */
void PowerServer::UpdateChargeControlRaw()
{
    const PowerMonitorSnapshot& snapshot = GetPowerSnapshotRaw();

    if (ShouldLatchChargeFaultRaw(snapshot))
    {
        //  m_chargeFaultLatched = true;
        LOG_BAT("Charge fault latched");
    }

    const bool             wasChargeEnabled = snapshot.chargeEnabled;
    const bool             chipValid        = IsChargeChipReadableNowRaw(snapshot);
    const ChargeEvaluation eval             = EvaluateChargeRaw(snapshot);

    SetBatteryChargeEnableRaw(eval.allowCharge, eval.fastChargeFlag);
    UpdateChargeSettleAfterDecisionRaw(eval.allowCharge, wasChargeEnabled);
    ApplyChargeSnapshotFromEvalRaw(eval, snapshot, chipValid);
}

/** @brief 充电仲裁并上报指示灯变化 */
void PowerServer::UpdateChargeControlAndNotifyRaw()
{
    UpdateChargeControlRaw();
    EvaluateChargeStatusRaw();
}

// ----- §5.6 指示灯与电量上报 -----

/** @brief 通知 entry 供电通路就绪，可刷新主灯 PWM */
void PowerServer::NotifyPowerPathReadyRaw()
{
    if (m_powerPathReadyHandler == nullptr)
    {
        return;
    }

    const bool usbUnplugFadeIn   = m_pendingUsbUnplugFadeIn;
    m_pendingUsbUnplugFadeIn     = false;
    m_powerPathReadyHandler(usbUnplugFadeIn);
}

/**
 * @brief 电池模式电量轮询与上报（只读快照）
 * @note CriticalEmpty 时硬切断放电并切至 BatteryIdle
 */
void PowerServer::PollBatteryVoltRaw()
{
    if (m_supplyMode != SupplyMode::Battery)
    {
        return;
    }

    const PowerMonitorSnapshot& snapshot = GetPowerSnapshotRaw();
    const BatteryVoltLevel      level    = MapVoltLevelRaw(snapshot.voltStatus);

    if (level == m_lastReportedVoltLevel)
    {
        return;
    }

    m_lastReportedVoltLevel = level;
    LOG_BAT("Battery level %u", static_cast<uint8_t>(level));

    if (m_batteryVoltHandler != nullptr)
    {
        m_batteryVoltHandler(level);
    }

    if (level != BatteryVoltLevel::CriticalEmpty)
    {
        return;
    }

    DisableBatteryDischargeRaw();
    m_batteryVoltPollEnabled = false;
    SetRunStateRaw(PowerRunState::BatteryIdle);
}

/** @brief USB 拔出或 DeInit 时熄灭充电指示灯并清快照/计时 */
void PowerServer::ClearChargeIndicatorRaw()
{
    if (m_chargeSnapshotValid && (m_chargeStatusHandler != nullptr))
    {
        BatteryChargeSnapshot off{};
        off.status       = BatteryChargeStatus::Idle;
        off.chipValid    = false;
        off.chipCharging = false;
        m_chargeStatusHandler(off);
    }

    m_lastReportedChargeSnapshot = {};
    m_chargeSnapshotValid        = false;
    m_currentChargeSnapshot      = {};
    m_chargeFaultLatched         = false;
    m_chargeSettleMs             = 0U;
    m_powerSnapshotValid         = false;
    ResetChargingSessionRaw();
}

/** @brief 比较充电快照变化，有变化时回调 entry 更新指示灯 */
void PowerServer::EvaluateChargeStatusRaw()
{
    if (m_supplyMode != SupplyMode::UsbPowered)
    {
        return;
    }

    const BatteryChargeSnapshot& snapshot = m_currentChargeSnapshot;
    const bool indicatorInputChanged      = !m_chargeSnapshotValid ||
                                            (snapshot.status != m_lastReportedChargeSnapshot.status) ||
                                            (snapshot.chipValid != m_lastReportedChargeSnapshot.chipValid) ||
                                            (snapshot.chipCharging != m_lastReportedChargeSnapshot.chipCharging);
    const bool snapshotChanged =
        indicatorInputChanged || (snapshot.useFastCharge != m_lastReportedChargeSnapshot.useFastCharge);

    if (!snapshotChanged)
    {
        return;
    }

    if (indicatorInputChanged && (m_chargeStatusHandler != nullptr))
    {
        m_chargeStatusHandler(snapshot);
    }

    m_lastReportedChargeSnapshot = snapshot;
    m_chargeSnapshotValid        = true;
}

// ----- §5.7 供电模式硬件落地 -----

/**
 * @brief 按当前供电来源执行硬件策略
 * @note 调用前须已 Fetch；USB：关放电+充电仲裁；电池：按灯状态放电
 */
void PowerServer::ApplySupplyModeHardwareRaw()
{
    if (m_supplyMode == SupplyMode::UsbPowered)
    {
        DisableBatteryDischargeRaw();
        m_batteryVoltPollEnabled = false;
        UpdateChargeControlAndNotifyRaw();
        NotifyPowerPathReadyRaw();
        return;
    }

    const bool usbUnplugRestore = m_pendingUsbUnplugFadeIn;

    if (usbUnplugRestore)
    {
        m_usbUnplugTransitionActive = true;
        if (m_usbUnplugPrepareHandler != nullptr)
        {
            m_usbUnplugPrepareHandler();
        }
    }

    if (usbUnplugRestore || m_mainLightActive)
    {
        EnableBatteryDischargeRaw();
        m_batteryVoltPollEnabled = usbUnplugRestore || m_mainLightActive;
    }
    else
    {
        DisableBatteryDischargeRaw();
        m_batteryVoltPollEnabled = false;
    }

    if (usbUnplugRestore || m_mainLightActive)
    {
        NotifyPowerPathReadyRaw();
    }

    m_usbUnplugTransitionActive = false;
}

// ----- §5.8 事件响应 -----

/**
 * @brief 主灯输出变化事件处理
 * @note 先 Fetch 再决策；电池模式刷新放电，USB 模式刷新快慢充
 */
void PowerServer::OnMainLightChangedRaw(bool mainLightActive)
{
    if (m_mainLightActive == mainLightActive)
    {
        return;
    }

    m_mainLightActive = mainLightActive;
    LOG_BAT("Main light active %u", static_cast<uint8_t>(mainLightActive));

    if (m_usbUnplugTransitionActive)
    {
        return;
    }

    FetchPowerMonitorSnapshotRaw();
    RefreshRunStateRaw();

    if (m_supplyMode == SupplyMode::Battery)
    {
        ApplyBatteryDischargeRaw();
        if (mainLightActive)
        {
            NotifyPowerPathReadyRaw();
        }
        return;
    }

    UpdateChargeControlAndNotifyRaw();
}

/**
 * @brief USB 拔插 ISR 处理
 * @note ISR 不读硬件、不打日志、不碰 SPI；置 pending，完整策略在 Poll 落地。
 */
void PowerServer::OnUsbConnectionChangedRaw(UsbConnectionStatusEnum usbStatus)
{
    if (usbStatus == UsbConnectionStatusEnum::UsbConnected)
    {
        m_supplyMode         = SupplyMode::UsbPowered;
        m_chargeFaultLatched = false;
        m_chargeSettleMs     = kChargeSettleMs;
        m_powerSnapshotValid = false;
        DisableBatteryDischargeRaw();
        LowPowerCoordinator::Instance().OnUsbPowerActivity(true, false);
    }
    else
    {
        m_supplyMode         = SupplyMode::Battery;
        m_chargeFaultLatched = false;
        m_chargeSettleMs     = 0U;
        m_powerSnapshotValid = false;
        SetBatteryChargeEnableRaw(false, 0U);
        ClearChargeIndicatorRaw();
        /* 注解10：灯亮时拔 USB → 切电池供电后 200ms 淡入 */
        m_pendingUsbUnplugFadeIn = m_mainLightActive;
        LowPowerCoordinator::Instance().OnUsbPowerActivity(false, false);
    }

    RefreshRunStateRaw();
    RequestSupplyApplyRaw();
}

// ----- §5.9 生命周期 -----

/** @brief 电源服务初始化 */
void PowerServer::Init()
{
    BspPowerMonitor::Instance().RegisterUsbNotifyCallback(UsbNotifyBridgeImpl);
    s_pollTimer.Init(PollTimerBridgeImpl, kPollIntervalMs, nullptr);

    FetchPowerMonitorSnapshotRaw();
    /* Init 阶段 Poll 早于回调注册；此处按当前 USB 状态补一次策略同步 */
    OnUsbConnectionChangedRaw(BspPowerMonitor::Instance().GetUsbStatus());
    m_mainLightActive = LightEffectEngine::Instance().IsAnyChannelActive();
    RefreshRunStateRaw();
    ApplySupplyModeHardwareRaw();
    SyncPollTimerFromRunStateRaw();
}

/** @brief 电源服务去初始化，确保最安全断电状态 */
void PowerServer::DeInit()
{
    StartPollTimerRaw(false);
    m_pollTimerEnabled = false;
    ClearChargeIndicatorRaw();
    SetBatteryChargeEnableRaw(false, 0U);
    DisableBatteryDischargeRaw();
    SetRunStateRaw(PowerRunState::BatteryIdle);
}

// ----- §5.10 各状态 Tick + Poll 入口 -----

/** @brief BatteryIdle 态 Tick：无额外操作（放电策略由事件/Apply 驱动） */
void PowerServer::TickBatteryIdleRaw(uint16_t elapsedMs)
{
    (void)elapsedMs;
}

/** @brief BatteryDischarging 态 Tick：轮询电压 */
void PowerServer::TickBatteryDischargingRaw(uint16_t elapsedMs)
{
    (void)elapsedMs;

    if (m_batteryVoltPollEnabled && (m_batterySettleMs == 0U))
    {
        PollBatteryVoltRaw();
    }
}

/** @brief UsbChargeManage 态 Tick：会话计时 → 充电仲裁 → 上报 */
void PowerServer::TickUsbChargeManageRaw(uint16_t elapsedMs)
{
    TickChargingSessionRaw(elapsedMs);
    UpdateChargeControlAndNotifyRaw();
}

/**
 * @brief 电源服务唯一周期驱动入口
 * @param elapsedMs 本轮流逝时间(ms)
 * @note ① settle 倒计时 ② Fetch 按需读硬件 ③ 落地 ISR 策略 ④ switch 分发
 */
void PowerServer::PowerPoll(uint16_t elapsedMs)
{
    // settle 先于 Fetch，保证电压/充电芯片可读性判断与采样同一轮一致
    if (m_runState == PowerRunState::BatteryDischarging)
    {
        TickSettleCountdownRaw(m_batterySettleMs, elapsedMs);
    }
    else if (m_runState == PowerRunState::UsbChargeManage)
    {
        TickSettleCountdownRaw(m_chargeSettleMs, elapsedMs);
    }

    FetchPowerMonitorSnapshotRaw();

    /* USB EXTI 后：在 Poll 上下文更新低功耗保持位（避免 ISR 内 Resume SPI） */
    LowPowerCoordinator::Instance().OnUsbPowerActivity(m_supplyMode == SupplyMode::UsbPowered);

    // ISR 切换供电来源后，统一刷新硬件策略
    if (m_pendingSupplyApply)
    {
        m_pendingSupplyApply = false;
        ApplySupplyModeHardwareRaw();
    }

    switch (m_runState)
    {
    case PowerRunState::BatteryIdle:
        TickBatteryIdleRaw(elapsedMs);
        break;
    case PowerRunState::BatteryDischarging:
        TickBatteryDischargingRaw(elapsedMs);
        break;
    case PowerRunState::UsbChargeManage:
        TickUsbChargeManageRaw(elapsedMs);
        break;
    default:
        assert(false);
        break;
    }
}
