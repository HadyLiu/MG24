/**
 * @file PowerMonitor.cpp
 * @brief 电源监控服务实现（含原 powerManage 全部逻辑）
 * @author hady
 * @date 2026-06-12
 * @layer Service
 */
#include "PowerMonitor.h"
#include "../hal/IadcHal.h"
#include "../hal/IndicatorHal.h"
#include "../hal/PinHal.h"
#include "AppConfig.h"
#include "app/AppLog.h"
#include "sl_udelay.h"

namespace {

bool s_powerStatus                       = false;
bool s_upstreamPowerStatus               = false;
bool s_triggerDetection                  = false;
bool s_chargingChipTriggerDetection      = false;
TypedefBatEnum s_batStatus               = Bat_DisCharge;
bool s_chargeState                       = false;
unsigned int s_chargeTimeSec             = 0;
unsigned int s_adBatLowVal         = 0;
unsigned int s_adPowerInVal        = 0;
unsigned int s_adTemperature       = 0;
unsigned int s_adBatLowMultipleVal = 0;

typedef enum
{
  Error,
  Incharge,
  ChargeFull,
} FM4258_Enum;

FM4258_Enum s_chargingChipStatus = Incharge;
unsigned char s_fmTimeout        = FMTimeOut;
unsigned char s_fmCount          = 0;

IadcHal s_adcA8(gpioPortA, 8);
IadcHal s_adcC4(gpioPortC, 4);
IadcHal s_adcC5(gpioPortC, 5);

FM4258_Enum fm4258GetStatus()
{
  const uint32_t samplePeriodMs = 10;
  const uint32_t pulseMinMs     = 800;
  const uint32_t pulseMaxMs     = 1200;

  static uint8_t lastLevel    = 0;
  static uint32_t msSinceEdge = 0;
  static uint8_t edgeCount    = 0;

  uint8_t level = PinHal::readChargeStatus();
  msSinceEdge += samplePeriodMs;

  if (level != lastLevel)
  {
    if (msSinceEdge >= pulseMinMs && msSinceEdge <= pulseMaxMs)
      edgeCount++;
    else
      edgeCount = 1;
    msSinceEdge = 0;
    lastLevel   = level;
  }

  if (edgeCount >= 2)
    edgeCount = 2;

  if (msSinceEdge > pulseMaxMs * 2)
    edgeCount = 0;

  if (level == 0)
    return Incharge;
  return ChargeFull;
}

} // namespace

/** @brief 获取 PowerMonitor 单例 */
PowerMonitor& PowerMonitor::instance()
{
  static PowerMonitor s_mon;
  return s_mon;
}

/** @brief 初始化 ADC 通道 */
void PowerMonitor::adcInit()
{
  s_adcC4.Init();
  s_adcC5.Init();
}

/** @brief 反初始化 ADC */
void PowerMonitor::adcDeInit()
{
  s_adcC4.DeInit(gpioModeInput, 0);
  s_adcC5.DeInit(gpioModeInput, 0);
  CMU_ClockEnable(cmuClock_IADC0, false);
}

uint16_t PowerMonitor::getPowerInAdcVol()
{
  uint16_t powerInVol;
  s_adcA8.Init();
  powerInVol = static_cast<uint16_t>(s_adcA8.ReadVoltageMilliVolts());
  s_adcA8.DeInit(gpioModeInput, 0);
  return powerInVol;
}

uint16_t PowerMonitor::getBatAdcVol()
{
  return static_cast<uint16_t>(s_adcC4.ReadVoltageMilliVolts());
}

uint16_t PowerMonitor::getTempAdcVol()
{
  return static_cast<uint16_t>(s_adcC5.ReadVoltageMilliVolts());
}

bool PowerMonitor::detectTemp()
{
  PinHal::setChargeEnable(true);
  sl_udelay_wait(2000);
  s_adTemperature = getTempAdcVol();
  PinHal::setChargeEnable(false);
  return true;
}

/** @brief 电源管理模块内部状态初始化 */
void PowerMonitor::powerManageInit()
{
  s_triggerDetection             = false;
  s_chargeTimeSec                = 0;
  s_adBatLowMultipleVal          = ((BatDisChargeLowWarnVal + 100) << 3);
  s_fmTimeout                    = FMTimeOut;
  s_fmCount                      = 0;
  s_chargingChipStatus           = Incharge;
  s_chargingChipTriggerDetection = false;
  PinHal::setChargeEnable(false);
}

/** @brief 获取放电时电池状态 */
void PowerMonitor::getDisChargeStatus()
{
  do
  {
    s_adBatLowVal = getBatAdcVol();

    if ((s_batStatus == Bat_LowVolProt) || (s_adBatLowVal < BatDisChargeLowVal))
    {
      s_batStatus = Bat_LowVolProt;
      break;
    }
    if ((s_batStatus == Bat_LowVolWarn) ||
        (s_adBatLowVal < BatDisChargeLowWarnVal))
    {
      s_batStatus = Bat_LowVolWarn;
      break;
    }
    s_batStatus = Bat_DisCharge;
    break;
  } while (0);
}

/** @brief 充电时间更新（10ms 调用） */
void PowerMonitor::chargeTimeUpdate()
{
  static unsigned char timeCount = 0;
  if (++timeCount > 100)
  {
    timeCount = 0;
    if (s_chargeTimeSec <= ChargeTimeMax && s_batStatus >= Bat_Nobat)
      s_chargeTimeSec++;
  }
}

bool PowerMonitor::chargeDetect()
{
  bool status                  = false;
  static uint8_t intervalTimer = 0;
  static uint8_t testingSteps  = 0;

  if (++intervalTimer >= 20)
  {
    intervalTimer = 0;
    if (testingSteps >= 2)
      testingSteps = 1;
  }

  if (s_batStatus >= Bat_Nobat)
  {
    if (testingSteps == 0)
    {
      PinHal::setChargeEnable(true);
      sl_udelay_wait(30);
      s_adTemperature = getTempAdcVol();
      testingSteps++;
    }
    else if (testingSteps == 1)
    {
      sl_udelay_wait(30);
      s_adPowerInVal = getPowerInAdcVol();
      testingSteps   = 0;
      status         = true;
    }
  }
  else
  {
    status = true;
  }

  s_chargingChipStatus = fm4258GetStatus();
  return status;
}

/** @brief 充电逻辑状态机 */
void PowerMonitor::chargeLogic(bool receptionStatus)
{
  while (receptionStatus)
  {
    if (s_batStatus <= Bat_InputOverCurt)
      break;
    if (s_adPowerInVal > PowerInLimitVol)
    {
      s_batStatus = Bat_InputOverVol;
      break;
    }

    if (s_adTemperature > NTCDetectBat)
    {
      s_chargeTimeSec = 0;
      s_batStatus     = Bat_Nobat;
      break;
    }

    if (s_chargeTimeSec < 4)
    {
      s_batStatus = Bat_Detect;
      break;
    }

    if (s_batStatus < Bat_ChargeInit)
      break;

    detectTemp();
    if (s_adTemperature > NTCDetectBat)
    {
      s_batStatus = Bat_Nobat;
      break;
    }
    if (s_batStatus == Bat_HighTemp)
    {
      if (s_adTemperature < NTCRecoverValTemp)
        s_batStatus = Bat_InCharge;
      else
        break;
    }
    else if (s_adTemperature < NTCOverValTemp)
    {
      s_batStatus = Bat_HighTemp;
      break;
    }

    if (s_chargingChipStatus == ChargeFull || (s_chargeTimeSec > ChargeTimeMax))
    {
      s_batStatus = Bat_ChargeFull;
      break;
    }
    if (s_chargingChipStatus == Error)
    {
      s_batStatus = Bat_ItError;
      break;
    }
    s_batStatus = Bat_InCharge;
    break;
  }
}

/** @brief 初始化电源监控 */
void PowerMonitor::init()
{
  LOG_BAT("[PowerMonitor] init");
  refreshUpstreamPower();
  powerManageInit();
  if (s_upstreamPowerStatus == false)
    getDisChargeStatus();
  else
    chargeLogic(chargeDetect());
}

void PowerMonitor::refreshUpstreamPower()
{
  s_upstreamPowerStatus = PinHal::getPowerInDetect();
}

void PowerMonitor::powerSwitchAssignment()
{
  if (s_powerStatus != s_upstreamPowerStatus)
  {
    s_powerStatus = s_upstreamPowerStatus;
    if (s_powerStatus)
      s_batStatus = Bat_ChargeInit;
    else
      s_batStatus = Bat_DisCharge;

    setPowerProtect(false);
    powerManageInit();
    IndicatorHal::stopWhiteBreath();
    IndicatorHal::stopRedBlink();
  }
}

void PowerMonitor::batOutEnable()
{
  PinHal::setBatteryEnable(true);
}

void PowerMonitor::batOutDisable()
{
  PinHal::setBatteryEnable(false);
}

void PowerMonitor::chargeCurrentCtrl(bool lightOn)
{
  if (s_batStatus == Bat_Nobat || s_batStatus == Bat_ChargeInit ||
      s_batStatus == Bat_Detect || s_batStatus == Bat_InCharge)
  {
    if (s_chargeState == false)
    {
      s_chargeState = true;
      PinHal::setChargeEnable(true);
    }

    if (!lightOn)
    {
      PinHal::setChargeSpeed(false);
    }
    else
    {
      PinHal::setChargeSpeed(true);
    }
  }
  else
  {
    if (s_chargeState == true)
    {
      s_chargeState = false;
      PinHal::setChargeEnable(false);
    }
  }
}

bool PowerMonitor::powerStatus() const
{
  return s_powerStatus;
}

bool PowerMonitor::upstreamPowerStatus() const
{
  return s_upstreamPowerStatus;
}

uint8_t PowerMonitor::batteryStatus() const
{
  return static_cast<uint8_t>(s_batStatus);
}

TypedefBatEnum PowerMonitor::batteryStatusEnum() const
{
  return s_batStatus;
}

uint32_t PowerMonitor::batteryVoltageMv() const
{
  return s_adBatLowVal * 3;
}

bool PowerMonitor::isPowerProtect() const
{
  return m_powerProtect;
}

void PowerMonitor::setPowerProtect(bool v)
{
  if (m_powerProtect != v)
    LOG_BAT("[PowerMonitor] power protect=%d", v);
  m_powerProtect = v;
}

void PowerMonitor::tickBatteryIfDue(uint32_t tick10ms, uint32_t interval10ms)
{
  static uint32_t last = 0;
  if (tick10ms - last >= interval10ms)
  {
    last = tick10ms;
    getDisChargeStatus();
    if (s_batStatus == Bat_LowVolProt)
      m_powerProtect = true;
  }
}

void PowerMonitor::tickCharge(uint32_t tick10ms)
{
  (void)tick10ms;
  chargeTimeUpdate();
  chargeLogic(chargeDetect());
}
