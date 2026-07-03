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

/* 充电芯片状态枚举 */
enum class ChargeChipStatusEnum : uint8_t
{
  CHARGE_INIT = 0, // 充电状态未定（初始状态，尚未判断）
  CHARGING,        // 正在充电 (持续低电平)
  CHARGE_DONE,     // 充满电 (持续高电平)
  CHARGE_FAULT     // 充电异常 (1.2Hz 闪烁脉冲)
};

/* 电池放电状态枚举 */
enum class BatteryVoltStatusEnum : uint8_t
{
  VOLT_NORMAL = 0,    // 电压正常
  VOLT_LOW_WARNING,   // 低电量提示（建议用户充电，系统仍可运行）
  VOLT_CRITICAL_EMPTY // 电池电压过低（触发死区保护，必须切断物理输出防止过放）
};

/* USB 连接状态枚举 */
enum class UsbConnectionStatusEnum : uint8_t
{
  UsbNotConnected = 0,
  UsbConnected
};

/* 温度状态枚举 */
enum class BatteryTempStatusEnum : uint8_t
{
  TEMP_NORMAL = 0,     // 温度正常，允许充电
  TEMP_TOO_HIGH,       // 温度过高，必须停充保护
  TEMP_TOO_LOW,        // 温度过低，必须停充保护
  TEMP_BATTERY_REMOVED // 电池被移除（NTC 开路）
};

/* 遵循 L1 BSP 层级规范，类名采用标准大驼峰 */
class BspPowerMonitor
{
public:
  static BspPowerMonitor& Instance()
  {
    static BspPowerMonitor bspPowerMonitor;
    return bspPowerMonitor;
  }

  /* 公开 API 统一采用标准大驼峰 */
  void Init();
  void DeInit();
  void SetBatteryOutEnable(bool enable);
  void SetBatteryChargeEnable(bool enable, uint8_t fast = 0);

  /* 对外回调注册接口 */
  typedef void (*PfUsbCallback)(UsbConnectionStatusEnum usbStatus);
  void RegisterUsbNotifyCallback(PfUsbCallback callback);

  /* 供应用层轮询获取状态的接口 */
  ChargeChipStatusEnum GetChargeStatus();
  bool IsChargeEnabled() const
  {
    return chargeState_;
  }
  BatteryTempStatusEnum GetBatteryTempStatus();
  BatteryVoltStatusEnum GetBatteryVoltStatus();

  /* 内联函数严格遵循 Allman 大括号风格 */
  UsbConnectionStatusEnum GetUsbStatus() const
  {
    return usbStatus_;
  }

  HalStateEnum GetBatteryVoltage(uint16_t* batMv = nullptr);
  HalStateEnum GetBatteryNtcVoltage(uint16_t* ntcMv = nullptr);

  /** @brief 轮询 PA08(USB_AD) ADC 检测 USB 插拔，状态变化时触发已注册回调 */
  void PollUsbStatusRaw();

private:
  /* 硬件抽象与驱动对象 */
  HalIadc batIadc_;
  HalIadc ntcIadc_;
  HalIadc usbIadc_;

  HalGpio batEnIo_;
  HalGpio chargeEnIo_;
  HalGpio chargeSpeedIo_;

  HalExti chargeStatExti_;

  /* 状态与计数私有变量 */
  bool chargeState_;
  UsbConnectionStatusEnum usbStatus_;
  ChargeChipStatusEnum chargeStatus_;
  BatteryTempStatusEnum batteryTempStatus_;
  uint32_t lastInterruptMs_;
  uint32_t pulseCounter_;
  bool isPulsing_;

  /* 回调函数指针 */
  PfUsbCallback appCallback_;

  /* 构造函数 */
  BspPowerMonitor();

  /* 🌟 私有辅助/中断中转函数：严格采用大驼峰 + Impl 后缀，视觉与语法双重隔离 */
  static void ChargeStatIsrBridgeCallbackImpl(uint8_t pin, bool pin_state,
                                              void* ctx);

  HalStateEnum GetUsbInputVoltageRaw(uint16_t* usbMv);
};
