/**
 * @file PowerServer.cpp
 * @brief
 * 负责系统的电源状态切换（USB/电池）、充电安全保护控制、电池电量轮询监测等核心功能。
 * 与 BSP 紧密协作，提供稳定的电源管理支持。
 * @author hady
 * @date 2026-06-26
 */
#include "PowerServer.h"
#include "DebugLog.h"

/**
 * @brief 静态硬件定时器，用于周期性触发电源轮询 (PowerPoll)
 */
static BspTimer s_powerTimer;

/**
 * @brief 注册电池电量等级变化的回调函数
 * @param handler 外部业务层注册的回调函数指针
 */
void PowerServer::RegisterBatteryVoltHandler(BatteryVoltHandler handler)
{
  m_batteryVoltHandler = handler;
}

/**
 * @brief 注册充电综合状态变化的回调函数
 * @param handler 外部业务层注册的回调函数指针（指示灯扩展预留）
 */
void PowerServer::RegisterChargeStatusHandler(ChargeStatusHandler handler)
{
  m_chargeStatusHandler = handler;
  if (handler == nullptr)
  {
    return;
  }

  // Init 阶段 Evaluate 可能早于 entry 接线，此处补发当前充电指示灯状态
  if (m_powerMode == PowerMode::UsbPowered)
  {
    UpdateChargeControlRaw();
    handler(m_currentChargeSnapshot);
    m_lastChargeSnapshot  = m_currentChargeSnapshot;
    m_chargeSnapshotValid = true;
  }
}

void PowerServer::RegisterLightPowerPathReadyHandler(
    LightPowerPathReadyHandler handler)
{
  m_lightPowerPathReadyHandler = handler;
}

/**
 * @brief 启动或停止电源轮询定时器
 * @param start true: 启动定时器, false: 停止定时器
 */
void PowerServer::TimerStartStop(bool start)
{
  if (start)
  {
    s_powerTimer.TurnOnOff(true);
  }
  else if (s_powerTimer.IsRunning())
  {
    s_powerTimer.TurnOnOff(false);
  }
}

/**
 * @brief 将底层硬件的电池电压状态映射为业务层的电压电量等级
 * @param status 底层驱动上报的原始电压状态
 * @return BatteryVoltLevel 映射后的电量等级（危急、低电预警、正常）
 */
BatteryVoltLevel PowerServer::MapVoltStatusRaw(BatteryVoltStatusEnum status)
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
 * @brief 充电综合状态 → 建议指示灯灯效
 * @note 充电结束：白光熄灭；充电中：白光呼吸；
 *       充电错误：红灯闪烁（白光由 IndicatorEffectEngine 切换时自动停）；
 *       故障消失：ChargeDone/Idle→全灭，恢复 Charging→白呼吸。
 */
ChargeIndicatorEffect
PowerServer::MapIndicatorEffectRaw(BatteryChargeStatus status)
{
  switch (status)
  {
  case BatteryChargeStatus::ChargeFault:
  case BatteryChargeStatus::TempFault:
  case BatteryChargeStatus::CriticalEmpty:
    return ChargeIndicatorEffect::RedBlink;
  case BatteryChargeStatus::Charging:
    return ChargeIndicatorEffect::WhiteBreath;
  case BatteryChargeStatus::ChargeDone:
  case BatteryChargeStatus::LowWarning:
  case BatteryChargeStatus::Idle:
  default:
    return ChargeIndicatorEffect::Off;
  }
}

/**
 * @brief 按优先级表仲裁充电综合电池状态
 * @return BatteryChargeSnapshot 状态 + 灯效 + 快慢充标志
 * @note 优先级（高→低）：ChargeFault > TempFault > CriticalEmpty >
 *       Charging > ChargeDone > LowWarning > Idle
 * @note 连续充电满 8h 视为 ChargeDone（见 kChargeFullTimeoutMs）。
 * @note 芯片 CHARGING/DONE/FAULT 仅在充电使能且 settle 结束后有效。
 */
BatteryChargeSnapshot
PowerServer::BuildChargeSnapshotRaw(ChargeChipStatusEnum chipStatus,
                                    bool chipStatusValid) const
{
  const BatteryTempStatusEnum tempStatus =
      BspPowerMonitor::Instance().GetBatteryTempStatus();
  const BatteryVoltStatusEnum voltStatus =
      BspPowerMonitor::Instance().GetBatteryVoltStatus();

  BatteryChargeSnapshot snapshot{};
  // 主灯关闭 -> 快充；主灯开启 -> 慢充
  snapshot.useFastCharge = !m_lightHasOutput;

  if (m_chargeFaultLatched)
  {
    snapshot.status = BatteryChargeStatus::ChargeFault;
  }
  else if (tempStatus != BatteryTempStatusEnum::TEMP_NORMAL)
  {
    snapshot.status = BatteryChargeStatus::TempFault;
  }
  else if (chipStatusValid &&
           (chipStatus == ChargeChipStatusEnum::CHARGE_FAULT))
  {
    snapshot.status = BatteryChargeStatus::ChargeFault;
  }
  else if (chipStatusValid &&
           (voltStatus == BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY))
  {
    snapshot.status = BatteryChargeStatus::CriticalEmpty;
  }
  else if (chipStatusValid && (chipStatus == ChargeChipStatusEnum::CHARGING))
  {
    snapshot.status = BatteryChargeStatus::Charging;
  }
  else if (chipStatusValid && (chipStatus == ChargeChipStatusEnum::CHARGE_DONE))
  {
    snapshot.status = BatteryChargeStatus::ChargeDone;
  }
  else if (chipStatusValid &&
           (voltStatus == BatteryVoltStatusEnum::VOLT_LOW_WARNING))
  {
    snapshot.status = BatteryChargeStatus::LowWarning;
  }
  else if (m_chargeTimeoutReached)
  {
    snapshot.status = BatteryChargeStatus::ChargeDone;
  }
  else if (BspPowerMonitor::Instance().IsChargeEnabled())
  {
    snapshot.status = BatteryChargeStatus::Charging;
  }
  else
  {
    snapshot.status = BatteryChargeStatus::Idle;
  }

  if (m_chargeTimeoutReached &&
      (snapshot.status == BatteryChargeStatus::Charging))
  {
    snapshot.status = BatteryChargeStatus::ChargeDone;
  }

  snapshot.indicator = MapIndicatorEffectRaw(snapshot.status);
  return snapshot;
}

ChargeChipStatusEnum PowerServer::ReadChargeChipStatusRaw() const
{
  if (!IsChargeChipStatusReadableRaw())
  {
    return ChargeChipStatusEnum::CHARGE_INIT;
  }

  return BspPowerMonitor::Instance().GetChargeStatus();
}

bool PowerServer::IsChargeChipStatusReadableRaw() const
{
  return BspPowerMonitor::Instance().IsChargeEnabled() &&
         (m_chargeStatusSettleMs == 0U);
}

/**
 * @brief 电源管理系统初始化
 * @details
 * 1. 初始化并启动心跳定时器
 * 2. 注册 USB 拔插检测的中断/事件回调
 * 3. 首次检测当前供电模式并应用对应硬件配置
 */
void PowerServer::Init()
{

  // 1. 初始化定时器，设置周期为 kPollIntervalMs，回调绑定到 PowerPoll
  s_powerTimer.Init(
      [](uint16_t elapsedMs) { PowerServer::Instance().PowerPoll(elapsedMs); },
      kPollIntervalMs, nullptr);

  // 2. 注册硬件底层的 USB 状态变化通知回调（ISR 触发）
  BspPowerMonitor::Instance().RegisterUsbNotifyCallback(
      [](UsbConnectionStatusEnum usbStatus) {
        PowerServer::Instance().PowerUsbISR(usbStatus);
      },
      nullptr);

  // 3. 读取当前硬件状态，判断是 USB 供电还是电池供电
  m_powerMode = (BspPowerMonitor::Instance().GetUsbStatus() ==
                 UsbConnectionStatusEnum::UsbConnected)
                    ? PowerMode::UsbPowered
                    : PowerMode::Battery;

  // 4. 应用对应的硬件控制通路
  if (m_powerMode == PowerMode::UsbPowered)
  {
    ApplyUsbModeRaw();
  }
  else
  {
    ApplyBatteryModeRaw();
  }

  // 5. 开启电源服务常驻心跳轮询
  TimerStartStop(true);
}

/**
 * @brief 电源管理系统去初始化（通常用于关机或复位前）
 * @details
 * 关闭定时器，关闭电池对外放电，关闭充电使能，确保设备处于最安全的断电状态
 */
void PowerServer::DeInit()
{
  TimerStartStop(false);
  ClearChargeIndicatorRaw();
  BspPowerMonitor::Instance().SetBatteryOutEnable(false);
  BspPowerMonitor::Instance().SetBatteryChargeEnable(false);
  m_batteryOutEnabled = false;
}

/**
 * @brief 外部负载（如主灯/强光输出）状态改变时的通知函数
 * @param isActive 负载是否处于工作状态（true: 亮起/输出, false: 熄灭/空闲）
 * @details
 * 在电池供电模式下，为了极端省电，只有当灯亮起时，才开启电池对外输出及电量检测。
 */
void PowerServer::OnLightOutputChanged(bool isActive)
{
  if (m_lightHasOutput == isActive)
  {
    return;
  }
  // 仅在电池模式下需要动态根据灯的亮灭去开启/关闭放电输出
  m_lightHasOutput = isActive;
  LOG_BAT("Light output changed: %u", static_cast<uint8_t>(isActive));

  if (m_powerMode == PowerMode::Battery)
  {
    ApplyBatteryModeRaw();
  }
  else if (m_powerMode == PowerMode::UsbPowered)
  {
    // 主灯亮灭变化时同步快慢充档位
    UpdateChargeControlRaw();
    EvaluateChargeStatusRaw();
  }
}

/**
 * @brief USB 状态变化的中断服务回调函数（由底层驱动触发）
 * @param usbStatus 硬件检测到的当前 USB 连接状态
 * @details 快速响应拔插事件，切换供电模式，并标记 m_modeDirty 待 Poll
 * 线程中同步硬件。
 */
void PowerServer::PowerUsbISR(UsbConnectionStatusEnum usbStatus)
{
  if (usbStatus == UsbConnectionStatusEnum::UsbConnected)
  {
    LOG_BAT("USB connected -> USB power mode");
    m_powerMode            = PowerMode::UsbPowered;
    m_chargeFaultLatched   = false;
    m_chargeStatusSettleMs = kChargeStatusSettleMs;
    BspPowerMonitor::Instance().SetBatteryOutEnable(false);
    m_batteryOutEnabled = false;
    ApplyUsbModeRaw();
  }
  else
  {
    LOG_BAT("USB disconnected -> battery mode");
    m_powerMode            = PowerMode::Battery;
    m_chargeFaultLatched   = false;
    m_chargeStatusSettleMs = 0U;
    // USB 拔出，立即关闭充电使能，熄灭充电指示灯
    BspPowerMonitor::Instance().SetBatteryChargeEnable(false);
    ClearChargeIndicatorRaw();
    ApplyBatteryModeRaw();
  }

  // 标记电源模式变脏，需要由 PowerPoll 尽快调用 ApplyXxxMode 刷新硬件寄存器
  m_modeDirty = true;
}

void PowerServer::NotifyLightPowerPathReadyRaw()
{
  if (m_lightPowerPathReadyHandler != nullptr)
  {
    m_lightPowerPathReadyHandler();
  }
}

/**
 * @brief 应用电池供电模式下的硬件策略
 * @details
 * 1. 只有灯亮起时才允许放电通道开启，并允许检测电量。
 * 2. 灯灭时关闭放电，避免不必要的电池自放电与电量检测开销。
 */
void PowerServer::ApplyBatteryModeRaw()
{
  m_batteryVoltPollEnabled = m_lightHasOutput;

  if (m_lightHasOutput)
  {
    if (!m_batteryOutEnabled)
    {
      BspPowerMonitor::Instance().SetBatteryOutEnable(true);
      m_batteryOutEnabled   = true;
      m_batteryPollSettleMs = kBatteryPollSettleMs;
      LOG_BAT("Battery out enabled (light active)");
      NotifyLightPowerPathReadyRaw();
    }
  }
  else
  {
    m_batteryPollSettleMs = 0U;
    if (m_batteryOutEnabled)
    {
      BspPowerMonitor::Instance().SetBatteryOutEnable(false);
      m_batteryOutEnabled = false;
      LOG_BAT("Battery out disabled (light idle)");
    }
  }
}

/**
 * @brief 应用 USB 供电模式下的硬件策略
 * @details
 * 关闭电池放电输出，仅做充电控制；不向 LDC 推送电量事件，避免主灯亮灭被误改。
 */
void PowerServer::ApplyUsbModeRaw()
{
  BspPowerMonitor::Instance().SetBatteryOutEnable(false);
  m_batteryOutEnabled   = false;
  m_batteryPollSettleMs = 0U;
  // USB 模式仅做充电控制，不向 LDC 推送电量事件，避免主灯亮灭被误改
  m_batteryVoltPollEnabled = false;
  UpdateChargeControlRaw();
  EvaluateChargeStatusRaw();
  NotifyLightPowerPathReadyRaw();
}

/**
 * @brief 充电控制安全防护机制（核心安全逻辑）
 * @details USB 上电先开启充电；GetChargeStatus 仅在充电使能且 settle
 *          结束后读取；检测到 CHARGE_FAULT 则关充并锁存故障。
 */
void PowerServer::UpdateChargeControlRaw()
{
  const BatteryTempStatusEnum tempStatus =
      BspPowerMonitor::Instance().GetBatteryTempStatus();
  const bool tempOk = (tempStatus == BatteryTempStatusEnum::TEMP_NORMAL);

  const bool chipReadable               = IsChargeChipStatusReadableRaw();
  const ChargeChipStatusEnum chipStatus = ReadChargeChipStatusRaw();

  if (chipReadable && (chipStatus == ChargeChipStatusEnum::CHARGE_FAULT))
  {
    m_chargeFaultLatched = true;
    LOG_BAT("Charge fault latched -> disable charge");
  }

  bool allowCharge = tempOk && !m_chargeFaultLatched && !m_chargeTimeoutReached;

  if (allowCharge && chipReadable)
  {
    if (chipStatus == ChargeChipStatusEnum::CHARGE_DONE)
    {
      allowCharge = false;
    }
    else if (chipStatus == ChargeChipStatusEnum::CHARGE_FAULT)
    {
      allowCharge = false;
    }
    else
    {
      const BatteryVoltStatusEnum voltStatus =
          BspPowerMonitor::Instance().GetBatteryVoltStatus();
      if (voltStatus == BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY)
      {
        allowCharge = false;
      }
    }
  }

  const bool wasChargeEnabled   = BspPowerMonitor::Instance().IsChargeEnabled();
  const uint8_t chargeSpeedFlag = m_lightHasOutput ? 0U : 1U;
  BspPowerMonitor::Instance().SetBatteryChargeEnable(allowCharge,
                                                     chargeSpeedFlag);

  if (allowCharge && !wasChargeEnabled)
  {
    m_chargeStatusSettleMs = kChargeStatusSettleMs;
  }
  else if (!allowCharge)
  {
    m_chargeStatusSettleMs = 0U;
  }

  const bool snapshotChipValid = IsChargeChipStatusReadableRaw();
  const ChargeChipStatusEnum snapshotChipStatus = ReadChargeChipStatusRaw();
  m_currentChargeSnapshot =
      BuildChargeSnapshotRaw(snapshotChipStatus, snapshotChipValid);

  LOG_BAT("Charge_ctrl: status=%u, allow=%u, fast=%u, chipValid=%u, "
          "chip=%u, chargeTime=%us",
          static_cast<uint8_t>(m_currentChargeSnapshot.status),
          static_cast<uint8_t>(allowCharge),
          static_cast<uint8_t>(chargeSpeedFlag),
          static_cast<uint8_t>(snapshotChipValid),
          static_cast<uint8_t>(snapshotChipStatus),
          static_cast<uint16_t>(m_chargingElapsedMs / 1000U));
}

/**
 * @brief 电池电量状态轮询（仅在电池模式且被允许时生效）
 * @details
 * 检测电量等级变化。一旦发现电量等级变化，立即回调通知外部（如主控或UI显示）；
 *          若检测到极度欠压
 * (CriticalEmpty)，则实行硬切断放电保护，强制关断对外供电。
 */
void PowerServer::PollBatteryStatusRaw()
{
  if (m_powerMode != PowerMode::Battery)
  {
    return;
  }

  // 回调通知外部（UI 电量槽变化，闪烁报警等）
  const BatteryVoltLevel level =
      MapVoltStatusRaw(BspPowerMonitor::Instance().GetBatteryVoltStatus());

  // 【低电保护】极度缺电情况下，强制切断输出，防止电池过放报废
  if (level != m_lastVoltLevel)
  {
    m_lastVoltLevel = level;
    LOG_BAT("Battery volt level: %u", static_cast<uint8_t>(level));

    if (m_batteryVoltHandler != nullptr)
    {
      m_batteryVoltHandler(level);
    }

    if (level == BatteryVoltLevel::CriticalEmpty)
    {
      BspPowerMonitor::Instance().SetBatteryOutEnable(false);
      m_batteryOutEnabled      = false;
      m_batteryVoltPollEnabled = false;
    }
  }
}

/**
 * @brief 重置充电计时（USB 拔出或芯片上报已充满）
 */
void PowerServer::ResetChargingTimerRaw()
{
  m_chargingElapsedMs    = 0U;
  m_chargeTimeoutReached = false;
}

/**
 * @brief 累计 USB 充电时长，满 8h 视为充满
 * @note 充电使能期间即累计；CHARGE_INIT 尚未读到芯片态时也计时。
 */
void PowerServer::UpdateChargingTimerRaw(uint16_t elapsedMs)
{
  if (m_powerMode != PowerMode::UsbPowered)
  {
    ResetChargingTimerRaw();
    return;
  }

  if (m_chargeTimeoutReached)
  {
    return;
  }

  if (!BspPowerMonitor::Instance().IsChargeEnabled())
  {
    return;
  }

  const ChargeChipStatusEnum chargeStatus =
      IsChargeChipStatusReadableRaw() ? ReadChargeChipStatusRaw()
                                      : ChargeChipStatusEnum::CHARGE_INIT;

  if (chargeStatus == ChargeChipStatusEnum::CHARGE_DONE)
  {
    ResetChargingTimerRaw();
    return;
  }

  if (chargeStatus == ChargeChipStatusEnum::CHARGE_FAULT)
  {
    return;
  }

  m_chargingElapsedMs += static_cast<uint32_t>(elapsedMs);
  if (m_chargingElapsedMs >= kChargeFullTimeoutMs)
  {
    m_chargeTimeoutReached = true;
    LOG_BAT("Charge full by 8h timeout (elapsed=%u ms)", m_chargingElapsedMs);
  }
}

/**
 * @brief 退出 USB 充电场景时熄灭指示灯并清快照
 */
void PowerServer::ClearChargeIndicatorRaw()
{
  if (m_chargeSnapshotValid && (m_chargeStatusHandler != nullptr))
  {
    BatteryChargeSnapshot off{};
    off.status    = BatteryChargeStatus::Idle;
    off.indicator = ChargeIndicatorEffect::Off;
    m_chargeStatusHandler(off);
  }

  m_lastChargeSnapshot    = {};
  m_chargeSnapshotValid   = false;
  m_currentChargeSnapshot = {};
  m_chargeFaultLatched    = false;
  m_chargeStatusSettleMs  = 0U;
  ResetChargingTimerRaw();
}

/**
 * @brief USB 充电综合状态评估与上报
 * @details 按优先级表生成 BatteryChargeSnapshot，状态变化时回调 entry。
 */
void PowerServer::EvaluateChargeStatusRaw()
{
  if (m_powerMode != PowerMode::UsbPowered)
  {
    return;
  }

  const BatteryChargeSnapshot& snapshot = m_currentChargeSnapshot;

  const bool indicatorChanged =
      !m_chargeSnapshotValid ||
      (snapshot.status != m_lastChargeSnapshot.status) ||
      (snapshot.indicator != m_lastChargeSnapshot.indicator);

  const bool snapshotChanged =
      indicatorChanged ||
      (snapshot.useFastCharge != m_lastChargeSnapshot.useFastCharge);

  if (!snapshotChanged)
  {
    return;
  }

  LOG_BAT("Charge snapshot: status=%u, indicator=%u, fast=%u",
          static_cast<uint8_t>(snapshot.status),
          static_cast<uint8_t>(snapshot.indicator),
          static_cast<uint8_t>(snapshot.useFastCharge));

  if (indicatorChanged)
  {
    if (m_chargeStatusHandler != nullptr)
    {
      m_chargeStatusHandler(snapshot);
      m_lastChargeSnapshot  = snapshot;
      m_chargeSnapshotValid = true;
    }
  }
  else
  {
    m_lastChargeSnapshot  = snapshot;
    m_chargeSnapshotValid = true;
  }
}

/**
 * @brief 电源服务的定时心跳轮询主函数
 * @param elapsedMs 定时器距离上一次被调用流逝的时间（单位毫秒）
 * @details
 * 该函数在定时器中断或线程中被高频周期性调用，驱动整个电源状态机的运转。
 */
void PowerServer::PowerPoll(uint16_t elapsedMs)
{
  BspPowerMonitor::Instance().PollUsbStatusRaw();

  if (m_modeDirty)
  {
    m_modeDirty = false;
    if (m_powerMode == PowerMode::UsbPowered)
    {
      ApplyUsbModeRaw();
    }
    else
    {
      ApplyBatteryModeRaw();
    }
  }

  if (m_powerMode == PowerMode::Battery)
  {
    if (m_batteryPollSettleMs > 0U)
    {
      if (elapsedMs >= m_batteryPollSettleMs)
      {
        m_batteryPollSettleMs = 0U;
      }
      else
      {
        m_batteryPollSettleMs =
            static_cast<uint16_t>(m_batteryPollSettleMs - elapsedMs);
      }
    }

    if (m_batteryVoltPollEnabled && (m_batteryPollSettleMs == 0U))
    {
      PollBatteryStatusRaw();
    }

    ApplyBatteryModeRaw();
  }

  if (m_powerMode == PowerMode::UsbPowered)
  {
    if (m_chargeStatusSettleMs > 0U)
    {
      if (elapsedMs >= m_chargeStatusSettleMs)
      {
        m_chargeStatusSettleMs = 0U;
      }
      else
      {
        m_chargeStatusSettleMs =
            static_cast<uint16_t>(m_chargeStatusSettleMs - elapsedMs);
      }
    }

    UpdateChargingTimerRaw(elapsedMs);
    UpdateChargeControlRaw();
    EvaluateChargeStatusRaw();
  }
}
