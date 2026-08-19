#pragma once

// 底层层级 (L0)
#include "cmsis_os2.h"

// HAL 层级 (L0)
#include "HalDef.h"
#include "HalExti.h"
#include "HalGpio.h"
#include "HalIadc.h"

// BSP 层级 (L1)
#include "BspIoConfig.h"
#include "BspSleepTimer.h"
#include "sl_sleeptimer.h"

/* 充电芯片状态枚举 */
enum class ChargeChipStatusEnum : uint8_t { CHARGE_INIT = 0, CHARGING, CHARGE_DONE, CHARGE_FAULT };

/* 电池放电状态枚举 */
enum class BatteryVoltStatusEnum : uint8_t { VOLT_NORMAL = 0, VOLT_LOW_WARNING, VOLT_CRITICAL_EMPTY };

/* USB 连接状态枚举 */
enum class UsbConnectionStatusEnum : uint8_t { UsbNotConnected = 0, UsbConnected };

/* 温度状态枚举 */
enum class BatteryTempStatusEnum : uint8_t { TEMP_NORMAL = 0, TEMP_TOO_HIGH, TEMP_TOO_LOW, TEMP_BATTERY_REMOVED };

class BspPowerMonitor {
 public:
  static BspPowerMonitor& Instance() {
    static BspPowerMonitor bspPowerMonitor;
    return bspPowerMonitor;
  }

  void Init();
  void DeInit();
  void SetBatteryOutEnable(bool enable);
  void SetBatteryChargeEnable(bool enable, uint8_t fast = 0);

  typedef void (*PfUsbCallback)(UsbConnectionStatusEnum usbStatus);
  void RegisterUsbNotifyCallback(PfUsbCallback callback);

  ChargeChipStatusEnum GetChargeStatus();
  bool IsChargeEnabled() const {
    return chargeState_;
  }
  BatteryTempStatusEnum GetBatteryTempStatus();
  BatteryVoltStatusEnum GetBatteryVoltStatus();

  UsbConnectionStatusEnum GetUsbStatus() const {
    return usbStatus_;
  }

  HalStateEnum GetBatteryVoltage(uint16_t* batMv = nullptr);
  HalStateEnum GetBatteryNtcVoltage(uint16_t* ntcMv = nullptr);

  /**
   * @brief 同步确认 USB 状态（ADC 阈值，与历史行为一致）
   * @note 常态靠 EXTI 唤醒后防抖再 ADC；Fetch/Init 也可直接调用本接口。
   */
  void PollUsbStatusRaw();

 private:
  HalIadc batIadc_;
  HalIadc ntcIadc_;
  HalIadc usbIadc_; /**< 仅用于插拔确认采样，不常开 */

  HalGpio batEnIo_;
  HalGpio chargeEnIo_;
  HalGpio chargeSpeedIo_;

  HalExti chargeStatExti_;
  HalExti usbDetectExti_; /**< PA8 双边沿唤醒（无上下拉） */

  sl_sleeptimer_timer_handle_t usbDebounceTimer_;
  bool usbDebounceArmed_;
  bool usbConfirmBusy_;

  bool chargeState_;
  UsbConnectionStatusEnum usbStatus_;
  ChargeChipStatusEnum chargeStatus_;
  BatteryTempStatusEnum batteryTempStatus_;
  uint32_t lastInterruptMs_;
  uint32_t pulseCounter_;
  bool isPulsing_;

  PfUsbCallback appCallback_;

  uint32_t filteredBatMv_;
  bool batFilterSeeded_;
  uint32_t filteredNtcMv_;
  bool ntcFilterSeeded_;

  uint8_t ntcRemovedHighCount_;
  uint8_t ntcPresentLowCount_;

  BspPowerMonitor();

  static void ChargeStatIsrBridgeCallbackImpl(uint8_t pin, bool pin_state, void* ctx);
  static void UsbDetectIsrBridgeCallbackImpl(uint8_t pin, bool pin_state, void* ctx);
  static void UsbDebounceTimerBridgeImpl(sl_sleeptimer_timer_handle_t* handle, void* data);

  void ApplyUsbConnectedRaw(bool connected, bool invokeCallback);
  void ScheduleUsbAdcConfirmRaw();
  void ConfirmUsbByAdcRaw();
  void ArmUsbExtiRaw();
  void DisarmUsbExtiRaw();

  HalStateEnum FetchNtcFromHardwareRaw(uint16_t* pRawMv, uint16_t* pFilteredMv);
  void ResetAdcAccumulatorsRaw();

  /** @brief 唯一静态电平采样：低=充电中，高=充满 */
  void ApplyChargeStatLevelFromGpioRaw();
};
