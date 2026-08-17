/**
 * @file bsp_power_monitor.cpp
 * @author hady
 * @brief - 电源状态获取相关的 BSP 实现
 * - 实现 NTC 温度读取 (通过电压值映射温度等级)
 * - 实现充电状态读取 (充电中低电平/充满高电平/异常 1.6hz 脉冲电平)
 * 中断实现配合定时器
 * - 实现 USB 连接状态读取 (通过检测 USB_IN 引脚电平) 中断实现
 * - 实现电池输出使能功能 (通过 GPIO 控制)
 * - 实现充电控制接口 (通过 GPIO 控制充电芯片的使能和充电速率)
 * @version 0.1
 * @date 2026-06-14
 */

#include "BspPowerMonitor.h"
#include "CheckConsecutive.h"
#include "DebugLog.h"
#include "FirstOrderFilter.h"

/** @name ADC 检测规格（注解）
 *  @{ */
/** @brief 电池分压比 1/3：ADC 中点电压 × 本系数还原电池端电压 */
static constexpr uint32_t kBatteryAdcDividerRestore = 3U;
/** @brief 电池电压 ADC 固定偏差调节 (mV)，实测偏高则取负、偏低则取正 */
static constexpr int16_t kBatteryAdcOffsetMv = 200;
/** @brief NTC ADC 固定偏差调节 (mV) */
static constexpr int16_t kNtcAdcOffsetMv = 12;
/** @brief 电池电压滤波系数 A（注解建议 5~15，取中值） */
static constexpr uint8_t kBatteryVoltFilterAlpha = 10U;
/** @brief NTC/温度滤波系数 A（注解建议 2~8，取中值） */
static constexpr uint8_t kNtcVoltFilterAlpha = 5U;
/** @} */

/** @name 放电电压阈值（mV，还原后的电池端电压）
 *  @{ */
static constexpr uint16_t kBatDischargeLowWarnMv = 7000U;   ///< §6.1 低电量：低于 7.0V
static constexpr uint16_t kBatDischargeCriticalMv = 6200U;  ///< §6.2 临界：低于 6.2V
/** @brief USB 分压还原后阈值(mV)：与历史 ADC 判定一致（引脚约 ≥1.5V） */
static constexpr uint16_t kUsbDetectLimitMv = 3000U;
/** @brief EXTI 后 ADC 确认防抖时间 */
static constexpr uint32_t kUsbDebounceMs = 50U;
/** @} */

/**
 * @brief 对还原后的 ADC 值施加固定偏差调节（注解）
 * @param rawMv  还原后的毫伏值
 * @param offset 有符号固定偏差
 * @return 调节后且钳位到非负的毫伏值
 */
static uint32_t ApplyAdcOffsetRaw(uint32_t rawMv, int16_t offset) {
  const int32_t adjusted = static_cast<int32_t>(rawMv) + static_cast<int32_t>(offset);
  if (adjusted <= 0) {
    return 0U;
  }

  return static_cast<uint32_t>(adjusted);
}

/**
 * @brief 首次采样直接种子，之后一阶低通（注解：防单次误触发保护）
 */
static uint32_t FilterAdcSampleRaw(uint32_t sampleMv, uint8_t alpha, uint32_t* pFiltered, bool* pSeeded) {
  if ((pFiltered == nullptr) || (pSeeded == nullptr)) {
    return sampleMv;
  }

  if (!(*pSeeded)) {
    *pFiltered = sampleMv;
    *pSeeded = true;
    return sampleMv;
  }

  bsp::FirstOrderFilter::Apply(sampleMv, alpha, pFiltered);
  return *pFiltered;
}

/** @name NTC 温度阈值（ADC mV；硬件上拉 20K，注解）
 *  @{ */
/** @brief 电池断开判定阈值：连续 3 次 NTC >= 本值 → 无电池 */
static constexpr uint16_t kNtcBatteryRemovedMv = 3150U;
static constexpr uint8_t kNtcConsecutiveMax = 3U;
#define NTCLowValTemp (2026)     ///< 低温保护 ADC 阈值
#define NTCOverValTemp (591)     ///< 高温保护 ADC 阈值
#define NTCRecoverValTemp (660)  ///< 高温恢复 ADC 阈值
/** @} */

/**
 * @brief 构造函数：完美对齐头文件声明顺序进行初始化
 */
BspPowerMonitor::BspPowerMonitor()
    : batIadc_(BAT_VOLTAGE_PORT, BAT_VOLTAGE_PIN),
      ntcIadc_(BAT_NTC_PORT, BAT_NTC_PIN),
      usbIadc_(USB_IN_PORT, USB_IN_PIN),
      batEnIo_(BAT_EN_PORT, BAT_EN_PIN),
      chargeEnIo_(CHARGE_EN_PORT, CHARGE_EN_PIN),
      chargeSpeedIo_(CHARGE_SPEED_PORT, CHARGE_SPEED_PIN),
      chargeStatExti_(LAMP_STATUS_PORT, LAMP_STATUS_PIN, HalExti::EdgeTrigger::BOTH),
      usbDetectExti_(USB_IN_PORT, USB_IN_PIN, HalExti::EdgeTrigger::BOTH),
      usbDebounceTimer_{},
      usbDebounceArmed_(false),
      usbConfirmBusy_(false) {
  chargeState_ = false;
  usbStatus_ = UsbConnectionStatusEnum::UsbNotConnected;
  chargeStatus_ = ChargeChipStatusEnum::CHARGE_INIT;
  batteryTempStatus_ = BatteryTempStatusEnum::TEMP_NORMAL;
  lastInterruptMs_ = 0;
  pulseCounter_ = 0;
  isPulsing_ = false;
  appCallback_ = nullptr;
  filteredBatMv_ = 0U;
  batFilterSeeded_ = false;
  filteredNtcMv_ = 0U;
  ntcFilterSeeded_ = false;
  ntcRemovedHighCount_ = 0U;
  ntcPresentLowCount_ = 0U;
}

/** * @brief 初始化 ADC 通道与外部中断（上电或唤醒后）
 */
void BspPowerMonitor::Init() {
  batIadc_.Init();
  ntcIadc_.Init();

  batEnIo_.Init(SL_GPIO_MODE_PUSH_PULL, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  chargeEnIo_.Init(SL_GPIO_MODE_PUSH_PULL, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  chargeSpeedIo_.Init(SL_GPIO_MODE_WIRED_AND, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);

  chargeStatExti_.RegisterCallback(BspPowerMonitor::ChargeStatIsrBridgeCallbackImpl, this);
  chargeStatExti_.Init(HalExti::PullMode::Down);
  chargeStatExti_.Enable(true);

  /* USB：EXTI 无上下拉（分压脚）；判定仍用 ADC 阈值 */
  usbDetectExti_.RegisterCallback(BspPowerMonitor::UsbDetectIsrBridgeCallbackImpl, this);
  ArmUsbExtiRaw();

  PollUsbStatusRaw();
}

/**
 * @brief 反初始化 ADC 通道与 USB EXTI
 */
void BspPowerMonitor::DeInit() {
  if (usbDebounceArmed_) {
    (void)sl_sleeptimer_stop_timer(&usbDebounceTimer_);
    usbDebounceArmed_ = false;
  }

  batIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  ntcIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  DisarmUsbExtiRaw();
}

/**
 * @brief 读取电池电压（分压还原 → 固定偏差 → 一阶低通）
 * @note 无电池（NTC 已判断开）时不更新滤波，避免开路采样污染累计值。
 */
HalStateEnum BspPowerMonitor::GetBatteryVoltage(uint16_t* bat_mv) {
  HalStateEnum state = HalStateEnum::HAL_ERROR;
  uint32_t batVoltageMv = 0;

  batIadc_.Init();
  batVoltageMv = batIadc_.ReadVoltageMilliVolts();

  if (batVoltageMv != 0) {
    batVoltageMv = batVoltageMv * kBatteryAdcDividerRestore;
    batVoltageMv = ApplyAdcOffsetRaw(batVoltageMv, kBatteryAdcOffsetMv);

    // 无电池期间跳过滤波更新；有电池时正常低通
    if (batteryTempStatus_ != BatteryTempStatusEnum::TEMP_BATTERY_REMOVED) {
      batVoltageMv = FilterAdcSampleRaw(batVoltageMv, kBatteryVoltFilterAlpha, &filteredBatMv_, &batFilterSeeded_);
    }

    if (bat_mv != nullptr) {
      *bat_mv = static_cast<uint16_t>(batVoltageMv);
    }
    state = HalStateEnum::HAL_OK;
  }
  batIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);

  LOG_POWER("Battery voltage: %u mV", batVoltageMv);
  return state;
}

/**
 * @brief NTC 唯一硬件读取入口：一次 ADC，输出原始与滤波值
 * @note 注解：断电池连续判定必须用原始数据，不得用滤波结果。
 */
HalStateEnum BspPowerMonitor::FetchNtcFromHardwareRaw(uint16_t* pRawMv, uint16_t* pFilteredMv) {
  HalStateEnum state = HalStateEnum::HAL_ERROR;
  uint32_t rawMv = 0U;
  uint32_t filtered = 0U;

  if (!chargeState_) {
    chargeEnIo_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_SET);
  }

  ntcIadc_.Init();
  rawMv = ntcIadc_.ReadVoltageMilliVolts();
  ntcIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);

  if (!chargeState_) {
    chargeEnIo_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  }

  if (rawMv == 0U) {
    LOG_POWER("NTC raw read failed");
    return HalStateEnum::HAL_ERROR;
  }

  // 无电池开路高压会污染低通；断开态只回原始值，插入后再种子化
  if (batteryTempStatus_ != BatteryTempStatusEnum::TEMP_BATTERY_REMOVED) {
    filtered = ApplyAdcOffsetRaw(rawMv, kNtcAdcOffsetMv);
    filtered = FilterAdcSampleRaw(filtered, kNtcVoltFilterAlpha, &filteredNtcMv_, &ntcFilterSeeded_);
  } else if (ntcFilterSeeded_) {
    filtered = filteredNtcMv_;
  } else {
    filtered = ApplyAdcOffsetRaw(rawMv, kNtcAdcOffsetMv);
  }

  if (pRawMv != nullptr) {
    *pRawMv = static_cast<uint16_t>(rawMv);
  }
  if (pFilteredMv != nullptr) {
    *pFilteredMv = static_cast<uint16_t>(filtered);
  }

  state = HalStateEnum::HAL_OK;
  LOG_POWER("NTC raw=%u filtered=%u mV", static_cast<unsigned>(rawMv), static_cast<unsigned>(filtered));
  return state;
}

/**
 * @brief 读取 NTC 电压（对外：偏差 + 一阶低通后的值）
 */
HalStateEnum BspPowerMonitor::GetBatteryNtcVoltage(uint16_t* ntc_mv) {
  uint16_t rawMv = 0U;
  uint16_t filteredMv = 0U;
  const HalStateEnum state = FetchNtcFromHardwareRaw(&rawMv, &filteredMv);
  (void)rawMv;
  if ((state == HalStateEnum::HAL_OK) && (ntc_mv != nullptr)) {
    *ntc_mv = filteredMv;
  }
  return state;
}

/**
 * @brief 使能电池放电输出
 */
void BspPowerMonitor::SetBatteryOutEnable(bool enable) {
  HalGpio::GpioPinStateEnum pin_state =
      enable ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
  batEnIo_.SetGpioPinState(pin_state);
}

/**
 * @brief 控制充电芯片使能与速率选择
 */
void BspPowerMonitor::SetBatteryChargeEnable(bool enable, uint8_t fast) {
  chargeState_ = enable;

  HalGpio::GpioPinStateEnum en_pin_state =
      enable ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
  chargeEnIo_.SetGpioPinState(en_pin_state);

  if (!enable) {
    chargeSpeedIo_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  } else {
    HalGpio::GpioPinStateEnum speedPinState =
        (fast == 0) ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
    chargeSpeedIo_.SetGpioPinState(speedPinState);
  }
}

/**
 * @brief 注册 USB 状态变化回调函数
 * @param callback 用户自定义的回调函数指针
 * @note 该函数允许应用层注册一个回调函数，当 USB 状态发生变化时被调用
 */
void BspPowerMonitor::RegisterUsbNotifyCallback(PfUsbCallback callback) {
  appCallback_ = callback;
}

/**
 * @brief 充电芯片状态中断回调（双边沿捕获，视觉与语法隔离 Impl 后缀）
 */
void BspPowerMonitor::ChargeStatIsrBridgeCallbackImpl(uint8_t pin, bool pin_state, void* ctx) {
  (void)pin;
  if (ctx != nullptr) {
    BspPowerMonitor* self = static_cast<BspPowerMonitor*>(ctx);

    // 利用 BspSleepTimer 获取绝对硬件低功耗毫秒时间戳
    uint32_t currentMs = BspSleepTimer::BspGetLowFrequencyMs();
    uint32_t deltaMs = currentMs - self->lastInterruptMs_;

    // 更新时间戳现场
    self->lastInterruptMs_ = currentMs;

    // 1.6Hz 方波半周期大约为 416ms。放宽硬件误差区间到 300ms ~ 600ms
    if (deltaMs >= 300 && deltaMs <= 600) {
      self->pulseCounter_++;
      if (self->pulseCounter_ >= 3) {
        // 连续检测到 3 次标准的交替脉冲，判定进入闪烁异常状态
        self->isPulsing_ = true;
        self->chargeStatus_ = ChargeChipStatusEnum::CHARGE_FAULT;
      }
    } else {
      self->pulseCounter_ = 0;
      self->isPulsing_ = false;

      // 既然没有处于闪烁状态，当前的实时物理电平就是最准确的静态状态
      if (!pin_state) {
        // 持续低电平 = 正在充电
        self->chargeStatus_ = ChargeChipStatusEnum::CHARGING;
      } else {
        // 持续高电平 = 充满
        self->chargeStatus_ = ChargeChipStatusEnum::CHARGE_DONE;
      }
    }
  }
}

/**
 * @brief 供应用层轮询获取状态的接口（带脉冲超时打破逻辑）
 */
ChargeChipStatusEnum BspPowerMonitor::GetChargeStatus() {
  // 如果当前处于 m_is_pulsing 状态，但是距离上一次中断发生已经过去了超过
  // 600ms， 说明方波已经停止（波形变稳），需要打破脉冲状态，更新为静态电平。
  if (isPulsing_) {
    uint32_t current_ms = BspSleepTimer::BspGetLowFrequencyMs();
    uint32_t elapsed_ms = current_ms - lastInterruptMs_;

    if (elapsed_ms > 600) {
      isPulsing_ = false;
      pulseCounter_ = 0;

      // 重新读取引脚当前的真实物理电平
      HalGpio::GpioPinStateEnum pinState = chargeStatExti_.GetGpioPinState();
      bool current_level = (pinState == HalGpio::GpioPinStateEnum::GPIO_PIN_SET);

      chargeStatus_ = (!current_level) ? ChargeChipStatusEnum::CHARGING : ChargeChipStatusEnum::CHARGE_DONE;
    }
  }

  return chargeStatus_;
}

BatteryVoltStatusEnum BspPowerMonitor::GetBatteryVoltStatus() {
  // 无电池时电压采样无效，避免滤波虚低触发 CriticalEmpty 拖垮充电白呼吸
  if (batteryTempStatus_ == BatteryTempStatusEnum::TEMP_BATTERY_REMOVED) {
    return BatteryVoltStatusEnum::VOLT_NORMAL;
  }

  uint16_t batMv = 0U;
  if (GetBatteryVoltage(&batMv) != HalStateEnum::HAL_OK) {
    return BatteryVoltStatusEnum::VOLT_NORMAL;
  }

  // §6：低于 6.2V 临界；低于 7.0V 低电警告
  if (batMv < kBatDischargeCriticalMv) {
    return BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY;
  }

  if (batMv < kBatDischargeLowWarnMv) {
    return BatteryVoltStatusEnum::VOLT_LOW_WARNING;
  }

  return BatteryVoltStatusEnum::VOLT_NORMAL;
}

/**
 * @brief USB 插拔：重置 NTC/电池电压滤波累计与连续判定计数（注解）
 */
void BspPowerMonitor::ResetAdcAccumulatorsRaw() {
  filteredBatMv_ = 0U;
  batFilterSeeded_ = false;
  filteredNtcMv_ = 0U;
  ntcFilterSeeded_ = false;
  ntcRemovedHighCount_ = 0U;
  ntcPresentLowCount_ = 0U;
}

/**
 * @brief 更新 USB 连接状态（唯一写入口）
 */
void BspPowerMonitor::ApplyUsbConnectedRaw(bool connected, bool invokeCallback) {
  const UsbConnectionStatusEnum newStatus =
      connected ? UsbConnectionStatusEnum::UsbConnected : UsbConnectionStatusEnum::UsbNotConnected;

  if (newStatus == usbStatus_) {
    return;
  }

  ResetAdcAccumulatorsRaw();
  usbStatus_ = newStatus;

  if (invokeCallback && (appCallback_ != nullptr)) {
    appCallback_(usbStatus_);
  }
}

/**
 * @brief 配置 PA8 为数字 EXTI（无上下拉）
 */
void BspPowerMonitor::ArmUsbExtiRaw() {
  (void)usbDetectExti_.Init(HalExti::PullMode::None);
  usbDetectExti_.Enable(true);
}

/**
 * @brief 关闭 USB EXTI，释放引脚给 ADC
 */
void BspPowerMonitor::DisarmUsbExtiRaw() {
  usbDetectExti_.Enable(false);
  usbDetectExti_.Deinit();
}

/**
 * @brief EXTI 后启动 50ms 单次防抖，再 ADC 确认
 */
void BspPowerMonitor::ScheduleUsbAdcConfirmRaw() {
  if (usbDebounceArmed_) {
    (void)sl_sleeptimer_stop_timer(&usbDebounceTimer_);
    usbDebounceArmed_ = false;
  }

  const sl_status_t status =
      sl_sleeptimer_start_timer_ms(&usbDebounceTimer_, kUsbDebounceMs, BspPowerMonitor::UsbDebounceTimerBridgeImpl,
                                   this, 0, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  if (status == SL_STATUS_OK) {
    usbDebounceArmed_ = true;
  }
}

/**
 * @brief 防抖定时器桥接
 */
void BspPowerMonitor::UsbDebounceTimerBridgeImpl(sl_sleeptimer_timer_handle_t* handle, void* data) {
  (void)handle;
  if (data == nullptr) {
    return;
  }

  BspPowerMonitor* self = static_cast<BspPowerMonitor*>(data);
  self->usbDebounceArmed_ = false;
  self->ConfirmUsbByAdcRaw();
}

/**
 * @brief ADC 确认 USB（与历史阈值一致），完成后重新 Arm EXTI
 */
void BspPowerMonitor::ConfirmUsbByAdcRaw() {
  if (usbConfirmBusy_) {
    return;
  }
  usbConfirmBusy_ = true;

  DisarmUsbExtiRaw();

  uint32_t usbVoltageMv = 0U;
  usbIadc_.Init();
  usbVoltageMv = usbIadc_.ReadVoltageMilliVolts();
  /* 释放模拟脚为高阻，勿推挽，避免干扰下次 EXTI */
  usbIadc_.DeInit(gpioModeDisabled, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);

  if (usbVoltageMv != 0U) {
    usbVoltageMv = usbVoltageMv * 2U;
  }

  const bool connected = (usbVoltageMv >= kUsbDetectLimitMv);
  ApplyUsbConnectedRaw(connected, true);

  ArmUsbExtiRaw();
  usbConfirmBusy_ = false;
}

/**
 * @brief USB_IN EXTI：只启动防抖 ADC，不在 ISR 内采模拟量
 */
void BspPowerMonitor::UsbDetectIsrBridgeCallbackImpl(uint8_t pin, bool pin_state, void* ctx) {
  (void)pin;
  (void)pin_state;
  if (ctx == nullptr) {
    return;
  }

  BspPowerMonitor* self = static_cast<BspPowerMonitor*>(ctx);
  self->ScheduleUsbAdcConfirmRaw();
}

/**
 * @brief 同步确认 USB（Init/Fetch 漏沿补读）
 */
void BspPowerMonitor::PollUsbStatusRaw() {
  ConfirmUsbByAdcRaw();
}

/**
 * @brief 读取电池温度状态
 * @return 电池温度状态
 * @note 注解：断电池连续判定用 NTC **原始数据**（非滤波）；
 *       连续 3 次原始 >=3150mV → 断开；连续 3 次原始 <3150mV → 清零累计。
 *       高/低温判定仍用滤波后数值，避免抖动。
 */
BatteryTempStatusEnum BspPowerMonitor::GetBatteryTempStatus() {
  uint16_t ntcRawMv = 0U;
  uint16_t ntcFilteredMv = 0U;
  if (FetchNtcFromHardwareRaw(&ntcRawMv, &ntcFilteredMv) != HalStateEnum::HAL_OK) {
    return batteryTempStatus_;
  }

  const bool batteryRemoved =
      bsp::CheckConsecutive::Apply(ntcRawMv, kNtcBatteryRemovedMv, &ntcRemovedHighCount_, kNtcConsecutiveMax);
  const bool presentStable =
      bsp::CheckConsecutive::ApplyBelow(ntcRawMv, kNtcBatteryRemovedMv, &ntcPresentLowCount_, kNtcConsecutiveMax);

  if (presentStable) {
    // 连续 3 次原始低于阈值：重置断开累计，并以当前原始值重新种子化滤波
    ntcRemovedHighCount_ = 0U;
    filteredNtcMv_ = ApplyAdcOffsetRaw(static_cast<uint32_t>(ntcRawMv), kNtcAdcOffsetMv);
    ntcFilterSeeded_ = true;
    ntcFilteredMv = static_cast<uint16_t>(filteredNtcMv_);

    // 电压滤波在无电池期已停更；插入后下次采样重新种子，避免虚低临界态
    batFilterSeeded_ = false;
    filteredBatMv_ = 0U;

    if (batteryTempStatus_ == BatteryTempStatusEnum::TEMP_BATTERY_REMOVED) {
      batteryTempStatus_ = BatteryTempStatusEnum::TEMP_NORMAL;
    }
  }

  if (batteryRemoved) {
    batteryTempStatus_ = BatteryTempStatusEnum::TEMP_BATTERY_REMOVED;
    // 开路采样不进入低通累计
    ntcFilterSeeded_ = false;
    batFilterSeeded_ = false;
    return batteryTempStatus_;
  }

  // 断开累计未满时，若仍处 REMOVED 锁存则保持（等低侧 3 次清除）
  if (batteryTempStatus_ == BatteryTempStatusEnum::TEMP_BATTERY_REMOVED) {
    return batteryTempStatus_;
  }

  do {
    /* 低温（滤波值） */
    if (ntcFilteredMv >= static_cast<uint16_t>(NTCLowValTemp)) {
      batteryTempStatus_ = BatteryTempStatusEnum::TEMP_TOO_LOW;
      break;
    }

    /* 高温恢复 */
    if (batteryTempStatus_ == BatteryTempStatusEnum::TEMP_TOO_HIGH) {
      if (ntcFilteredMv <= static_cast<uint16_t>(NTCRecoverValTemp)) {
        batteryTempStatus_ = BatteryTempStatusEnum::TEMP_NORMAL;
        break;
      }
    }
    /* 高温 */
    if (ntcFilteredMv <= static_cast<uint16_t>(NTCOverValTemp)) {
      batteryTempStatus_ = BatteryTempStatusEnum::TEMP_TOO_HIGH;
      break;
    }
    /* 正常 */
    batteryTempStatus_ = BatteryTempStatusEnum::TEMP_NORMAL;
  } while (0);

  return batteryTempStatus_;
}
