/**
 * @file PowerServer.h
 * @brief 电源服务：电池放电/充电控制与电压检测
 * @author hady
 * @date 2026-06-25
 * @layer Service
 */
#pragma once

#include "BspPowerMonitor.h"
#include "BspTimer.h"
#include "LightDecisionTypes.h"
#include <cstdint>

class PowerServer
{
public:
  /** @brief 电池电压等级变化（→ entry → LightDecisionCenter） */
  using BatteryVoltHandler = void (*)(BatteryVoltLevel level);

  /**
   * @brief 充电综合状态变化（→ entry → IndicatorEffectEngine）
   * @note 策略：充电中白呼吸 / 充电结束熄灭 / 充电错误红闪。
   */
  using ChargeStatusHandler = void (*)(const BatteryChargeSnapshot& snapshot);

  /** @brief 供电通路切换且主灯应亮时，通知 entry 刷新 PWM */
  using LightPowerPathReadyHandler = void (*)();

  static PowerServer& Instance()
  {
    static PowerServer instance;
    return instance;
  }

  void Init();
  void DeInit();

  void RegisterBatteryVoltHandler(BatteryVoltHandler handler);
  void RegisterChargeStatusHandler(ChargeStatusHandler handler);
  void RegisterLightPowerPathReadyHandler(LightPowerPathReadyHandler handler);

  /** @brief 灯物理输出活跃状态变化（由 entry 从 LightEffectEngine 注入） */
  void OnLightOutputChanged(bool isActive);

private:
  enum class PowerMode : uint8_t
  {
    Battery = 0,
    UsbPowered
  };

  PowerServer()                              = default;
  ~PowerServer()                             = default;
  PowerServer(const PowerServer&)            = delete;
  PowerServer& operator=(const PowerServer&) = delete;

  void TimerStartStop(bool start);
  void PowerPoll(uint16_t elapsedMs);
  void PowerUsbISR(UsbConnectionStatusEnum usbStatus);

  void ApplyBatteryModeRaw();
  void ApplyUsbModeRaw();
  void PollBatteryStatusRaw();
  void EvaluateChargeStatusRaw();
  void UpdateChargeControlRaw();
  void ClearChargeIndicatorRaw();
  void UpdateChargingTimerRaw(uint16_t elapsedMs);
  void ResetChargingTimerRaw();
  void NotifyLightPowerPathReadyRaw();
  ChargeChipStatusEnum ReadChargeChipStatusRaw() const;
  bool IsChargeChipStatusReadableRaw() const;

  BatteryChargeSnapshot BuildChargeSnapshotRaw(ChargeChipStatusEnum chipStatus,
                                               bool chipStatusValid) const;
  static BatteryVoltLevel MapVoltStatusRaw(BatteryVoltStatusEnum status);
  static ChargeIndicatorEffect MapIndicatorEffectRaw(BatteryChargeStatus status);

  BatteryVoltHandler m_batteryVoltHandler{nullptr};
  ChargeStatusHandler m_chargeStatusHandler{nullptr};
  LightPowerPathReadyHandler m_lightPowerPathReadyHandler{nullptr};

  PowerMode m_powerMode{PowerMode::Battery};
  BatteryVoltLevel m_lastVoltLevel{BatteryVoltLevel::Normal};
  BatteryChargeSnapshot m_lastChargeSnapshot{};
  bool m_lightHasOutput{false};
  bool m_batteryOutEnabled{false};
  bool m_batteryVoltPollEnabled{false};
  bool m_modeDirty{false};
  bool m_chargeSnapshotValid{false};
  bool m_chargeTimeoutReached{false};
  bool m_chargeFaultLatched{false};
  uint32_t m_chargingElapsedMs{0U};
  uint16_t m_batteryPollSettleMs{0U};
  uint16_t m_chargeStatusSettleMs{0U};
  BatteryChargeSnapshot m_currentChargeSnapshot{};

  static constexpr uint16_t kPollIntervalMs = 1000U;
  static constexpr uint16_t kBatteryPollSettleMs = 500U;
  static constexpr uint16_t kChargeStatusSettleMs = 300U;
  static constexpr uint32_t kChargeFullTimeoutMs = 8U * 60U * 60U * 1000U;
};
