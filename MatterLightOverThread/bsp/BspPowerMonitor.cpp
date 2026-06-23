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

/** @name 放电与输入电压阈值（mV / 计数）
 *  @{ */
#define BatDisChargeLowWarnVal (2300)      ///< 低电告警电压(mV)
#define BatDisChargeLowVal (2000)          ///< 低电保护截止(mV)
#define PowerInLimitVol (3000)             ///< 外部输入过压阈值(mV)
#define PowerInOvervoltagesLimitCount (10) ///< 过压连续判定次数
/** @} */

/** @name 充电电流、NTC 与超时（硬件标定常量）
 *  @{ */
#define BatLimitI (6700)             ///< 充电电流上限(µA 或标定单位)
#define NTCDetectBat (3092)          ///< NTC 采样基准
#define NTCLowValTemp (2026)         ///< 低温保护 ADC 阈值
#define NTCOverValTemp (591)         ///< 高温保护 ADC 阈值
#define NTCRecoverValTemp (660)      ///< 高温恢复 ADC 阈值
#define NTCBatUsedOverValTemp (1349) ///< 使用中过温 ADC 阈值

/**
 * @brief 构造函数：完美对齐头文件声明顺序进行初始化
 */
BspPowerMonitor::BspPowerMonitor()
    : batIadc_(BAT_VOLTAGE_PORT, BAT_VOLTAGE_PIN),
      ntcIadc_(BAT_NTC_PORT, BAT_NTC_PIN), batEnIo_(BAT_EN_PORT, BAT_EN_PIN),
      chargeEnIo_(CHARGE_EN_PORT, CHARGE_EN_PIN),
      chargeSpeedIo_(CHARGE_SPEED_PORT, CHARGE_SPEED_PIN),
      usbExti_(USB_IN_PORT, USB_IN_PIN, HalExti::EdgeTrigger::BOTH),
      chargeStatExti_(LAMP_STATUS_PORT, LAMP_STATUS_PIN,
                      HalExti::EdgeTrigger::BOTH)
{
  // 成员变量初始化
  chargeState_     = false;
  usbStatus_       = UsbConnectionStatusEnum::UsbNotConnected;
  chargeStatus_    = ChargeChipStatusEnum::CHARGE_INIT;
  lastInterruptMs_ = 0;
  pulseCounter_    = 0;
  isPulsing_       = false;
  appCallback_     = nullptr;
  appContext_      = nullptr;
}

/** * @brief 初始化 ADC 通道与外部中断（上电或唤醒后）
 */
void BspPowerMonitor::Init()
{
  // 1. 初始化 ADC 组件
  batIadc_.Init();
  ntcIadc_.Init();

  // 2. 初始化普通 GPIO 输出管脚，默认状态为关闭
  batEnIo_.Init(SL_GPIO_MODE_PUSH_PULL,
                HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  chargeEnIo_.Init(SL_GPIO_MODE_PUSH_PULL,
                   HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  chargeSpeedIo_.Init(SL_GPIO_MODE_WIRED_AND,
                      HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);

  // 3. 注册并拉起底层 EXTI 外部中断（指向 Impl 后缀的私有静态中断桥梁）
  usbExti_.RegisterCallback(BspPowerMonitor::UsbIsrBridgeCallbackImpl, this);
  usbExti_.Init();

  chargeStatExti_.RegisterCallback(
      BspPowerMonitor::ChargeStatIsrBridgeCallbackImpl, this);
  chargeStatExti_.Init();

  // 4. 同步读取初始现场状态
  HalGpio::GpioPinStateEnum usb_pin_state = usbExti_.GetGpioPinState();
  usbStatus_ = (usb_pin_state == HalGpio::GpioPinStateEnum::GPIO_PIN_SET)
                   ? UsbConnectionStatusEnum::UsbConnected
                   : UsbConnectionStatusEnum::UsbNotConnected;

  // 如果开机时 USB 就在位，在确保安全的前提下直接联动开启充电
  // if (usbStatus_ == UsbConnectionStatusEnum::UsbConnected &&
  //    GetBatteryTempStatus() == BatteryTempStatusEnum::TEMP_NORMAL &&
  //    GetBatteryVoltStatus() != BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY)
  //{
  //  chargeState_ = true;
  //  chargeEnIo_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_SET);
  //}
}

/**
 * @brief 反初始化 ADC 通道（进入深睡眠前释放资源）
 */
void BspPowerMonitor::DeInit()
{
  batIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  ntcIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
}

/**
 * @brief 读取电池电压
 */
HalStateEnum BspPowerMonitor::GetBatteryVoltage(uint16_t* bat_mv)
{
  HalStateEnum state    = HalStateEnum::HAL_ERROR;
  uint32_t batVoltageMv = 0;

  batIadc_.Init();
  osDelay(10); // 稍微延时确保 ADC 稳定
  batVoltageMv = batIadc_.ReadVoltageMilliVolts();

  if (batVoltageMv != 0)
  {
    batVoltageMv = batVoltageMv * 3;
    if (bat_mv != nullptr)
    {
      *bat_mv = static_cast<uint16_t>(batVoltageMv);
    }
    state = HalStateEnum::HAL_OK;
  }
  batIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  return state;
}

/**
 * @brief 读取 NTC 电压
 */
HalStateEnum BspPowerMonitor::GetBatteryNtcVoltage(uint16_t* ntc_mv)
{
  HalStateEnum state    = HalStateEnum::HAL_ERROR;
  uint32_t ntcVoltageMv = 0;

  if (!chargeState_)
  {
    chargeEnIo_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_SET);
  }

  ntcIadc_.Init();
  osDelay(10); // 稍微延时确保 ADC 稳定
  ntcVoltageMv = ntcIadc_.ReadVoltageMilliVolts();

  if (ntcVoltageMv != 0)
  {
    ntcVoltageMv = ntcVoltageMv * 3;
    if (ntc_mv != nullptr)
    {
      *ntc_mv = static_cast<uint16_t>(ntcVoltageMv);
    }
    state = HalStateEnum::HAL_OK;
  }

  ntcIadc_.DeInit(gpioModePushPull, HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);

  if (!chargeState_)
  {
    chargeEnIo_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  }
  return state;
}

/**
 * @brief 使能电池放电输出
 */
void BspPowerMonitor::SetBatteryOutEnable(bool enable)
{
  HalGpio::GpioPinStateEnum pin_state =
      enable ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET
             : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
  batEnIo_.SetGpioPinState(pin_state);
}

/**
 * @brief 控制充电芯片使能与速率选择
 */
void BspPowerMonitor::SetBatteryChargeEnable(bool enable, uint8_t fast)
{
  chargeState_ = enable;

  HalGpio::GpioPinStateEnum en_pin_state =
      enable ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET
             : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
  chargeEnIo_.SetGpioPinState(en_pin_state);

  if (!enable)
  {
    chargeSpeedIo_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  }
  else
  {
    HalGpio::GpioPinStateEnum speedPinState =
        (fast == 0) ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET
                    : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
    chargeSpeedIo_.SetGpioPinState(speedPinState);
  }
}

/**
 * @brief 充电芯片状态中断回调（双边沿捕获，视觉与语法隔离 Impl 后缀）
 */
void BspPowerMonitor::ChargeStatIsrBridgeCallbackImpl(uint8_t pin,
                                                      bool pin_state, void* ctx)
{
  (void)pin;
  if (ctx != nullptr)
  {
    BspPowerMonitor* self = static_cast<BspPowerMonitor*>(ctx);

    // 利用 BspSleepTimer 获取绝对硬件低功耗毫秒时间戳
    uint32_t currentMs = BspSleepTimer::BspGetLowFrequencyMs();
    uint32_t deltaMs   = currentMs - self->lastInterruptMs_;

    // 更新时间戳现场
    self->lastInterruptMs_ = currentMs;

    // 1.6Hz 方波半周期大约为 416ms。放宽硬件误差区间到 300ms ~ 550ms
    if (deltaMs >= 300 && deltaMs <= 550)
    {
      self->pulseCounter_++;
      if (self->pulseCounter_ >= 3)
      {
        // 连续检测到 3 次标准的交替脉冲，判定进入闪烁异常状态
        self->isPulsing_    = true;
        self->chargeStatus_ = ChargeChipStatusEnum::CHARGE_FAULT;
      }
    }
    else
    {
      self->pulseCounter_ = 0;
      self->isPulsing_    = false;

      // 既然没有处于闪烁状态，当前的实时物理电平就是最准确的静态状态
      if (!pin_state)
      {
        // 持续低电平 = 正在充电
        self->chargeStatus_ = ChargeChipStatusEnum::CHARGING;
      }
      else
      {
        // 持续高电平 = 充满
        self->chargeStatus_ = ChargeChipStatusEnum::CHARGE_DONE;
      }
    }
  }
}

/**
 * @brief 供应用层轮询获取状态的接口（带脉冲超时打破逻辑）
 */
ChargeChipStatusEnum BspPowerMonitor::GetChargeStatus()
{
  // 如果当前处于 m_is_pulsing 状态，但是距离上一次中断发生已经过去了超过
  // 600ms， 说明方波已经停止（波形变稳），需要打破脉冲状态，更新为静态电平。
  if (isPulsing_)
  {
    uint32_t current_ms = BspSleepTimer::BspGetLowFrequencyMs();
    uint32_t elapsed_ms = current_ms - lastInterruptMs_;

    if (elapsed_ms > 600)
    {
      isPulsing_    = false;
      pulseCounter_ = 0;

      // 重新读取引脚当前的真实物理电平
      HalGpio::GpioPinStateEnum pinState = chargeStatExti_.GetGpioPinState();
      bool current_level =
          (pinState == HalGpio::GpioPinStateEnum::GPIO_PIN_SET);

      chargeStatus_ = (!current_level) ? ChargeChipStatusEnum::CHARGING
                                       : ChargeChipStatusEnum::CHARGE_DONE;
    }
  }

  return chargeStatus_;
}

BatteryVoltStatusEnum BspPowerMonitor::GetBatteryVoltStatus()
{
  uint16_t batMv = 0U;
  if (GetBatteryVoltage(&batMv) != HalStateEnum::HAL_OK)
  {
    return BatteryVoltStatusEnum::VOLT_NORMAL;
  }

  if (batMv <= static_cast<uint16_t>(BatDisChargeLowVal))
  {
    return BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY;
  }

  if (batMv <= static_cast<uint16_t>(BatDisChargeLowWarnVal))
  {
    return BatteryVoltStatusEnum::VOLT_LOW_WARNING;
  }

  return BatteryVoltStatusEnum::VOLT_NORMAL;
}

BatteryTempStatusEnum BspPowerMonitor::GetBatteryTempStatus()
{
  return BatteryTempStatusEnum::TEMP_NORMAL;
}

/**
 * @brief USB 插入/拔出检测中断中转桥梁（Impl 后缀）
 */
void BspPowerMonitor::UsbIsrBridgeCallbackImpl(uint8_t pin, bool pin_state,
                                               void* ctx)
{
  (void)pin;
  if (ctx != nullptr)
  {
    BspPowerMonitor* self = static_cast<BspPowerMonitor*>(ctx);
    if (pin_state)
    {
      self->usbStatus_ = UsbConnectionStatusEnum::UsbConnected;
    }
    else
    {
      self->usbStatus_ = UsbConnectionStatusEnum::UsbNotConnected;
    }
  }
}