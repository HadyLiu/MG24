/**
 * @file MatterBridge.cpp
 * @brief Matter 集成层实现：属性同步、Identify、软复位、C 兼容 API
 * @author hady
 * @date 2026-06-12
 * @layer Integration
 * @note 上行属性写入经 PlatformMgr().ScheduleWork 切到 Matter
 * 线程，避免线程冲突。
 */
#include <cstring>

#include "../policy/CommissioningManager.h"
#include "../policy/IdentifyEffect.h"
#include "../service/ColorConverter.h"
#include "../service/ColorLibrary.h"
#include "../service/LightEngine.h"
#include "AppConfig.h"
#include "LightingManager.h"
#include "MatterBridge.h"
#include "RGBLEDWidget.h"
#include "app/WakeControl.h"
#include "app/AppLog.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/CodegenIntegration.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <openthread/dataset.h>
#include <platform/OpenThread/GenericThreadStackManagerImpl_OpenThread.h>
#endif

#include "AppConfig.h"

using namespace chip;
using namespace chip::DeviceLayer;

static bool g_bypass_zcl_callback = false;
uint16_t out_ex = 0, out_ey = 0;

/** @brief 获取 MatterBridge 单例 */
MatterBridge& MatterBridge::instance()
{
  static MatterBridge s_bridge;
  return s_bridge;
}

bool MatterBridge::isReportBypassEnabled() const
{
  return g_bypass_zcl_callback;
}

uint8_t MatterBridge::clampMatterLevel(uint8_t matterLevel)
{
  // 最低约 2%（5 流明），Matter level 0-254
  if (matterLevel > 0 && matterLevel < 5)
    return 5;
  return matterLevel;
}

/** @brief Matter 下行：开关/亮度动作 */
void MatterBridge::onActionInitiated(int aAction, uint8_t* aValue, bool lightOn)
{
  (void)lightOn;
  ConditionalWake_up();

  LightEngine& le = LightEngine::instance();

  if (aAction == LightingManager::ON_ACTION ||
      aAction == LightingManager::OFF_ACTION)
  {
    uint8_t prev = le.brightness();
    if (aAction == LightingManager::ON_ACTION)
    {
      if (le.historyBrightness() <= 1)
        le.setHistoryBrightness(255);
      le.updateNormalState(true, le.historyBrightness(), le.rawColor());
    }
    else
    {
      if (le.brightness() >= 1)
        le.setHistoryBrightness(le.brightness());
      le.updateNormalState(false, 0, le.rawColor());
    }
    (void)prev;
    le.setChangeOrigin(ChangeOriginMatterApp);
    le.saveStateToFlash();
    // 恒定 400ms 淡入/淡出
    le.startFadeToCurrent(LED_FADE_KEY_TOTAL_MS, FadePolicyFixedMs);
    LOG_MATTER("[MatterBridge] on/off action=%d on=%d bright=%u", aAction,
               le.isOn(), le.brightness());
  }

  if (aAction == LightingManager::LEVEL_ACTION && aValue)
  {
    uint8_t level = clampMatterLevel(*aValue);
    le.setChangeOrigin(ChangeOriginMatterApp);
    le.updateNormalState(le.isOn(), level, le.rawColor());
    le.saveStateToFlash();
    // 恒定 400ms 淡入/淡出
    le.startFadeToCurrent(LED_FADE_KEY_TOTAL_MS, FadePolicyFixedMs);
    LOG_MATTER("[MatterBridge] level=%u (clamped from %u)", level, *aValue);
  }
}

void MatterBridge::onColorEvent(uint8_t action, void* valueData, uint16_t x,
                                uint16_t y)
{
  if (!valueData)
    return;
  ConditionalWake_up();

  auto* colorData = reinterpret_cast<RGBLEDWidget::ColorData_t*>(valueData);
  LightEngine& le = LightEngine::instance();
  WrgbColor c     = {0, 0, 0, 0};

  switch (static_cast<LightingManager::Action_t>(action))
  {
  case LightingManager::COLOR_ACTION_HSV:
    c = ColorConverter::fromHsv(colorData->hsv.h, colorData->hsv.s, 254);
    break;
  case LightingManager::COLOR_ACTION_CT: {
    uint16_t mireds = colorData->ct.ctMireds;
    uint32_t kelvin = (mireds > 0) ? (1000000U / mireds) : 2700U;
    c               = ColorConverter::fromColorTemperature(kelvin);
    break;
  }
  case LightingManager::COLOR_ACTION_XY:
    c = ColorConverter::fromXy(out_ex, out_ey);
    break;
  default:
    return;
  }

  le.setColorSource(ColorSourceCustomWrgb);
  le.setChangeOrigin(ChangeOriginMatterApp);
  le.updateNormalState(le.isOn(), le.brightness(), c);
  le.saveStateToFlash();
  le.startFadeToCurrent(LED_FADE_COLOR_SWITCH_MS, FadePolicyFixedMs);
  LOG_MATTER("[MatterBridge] color event action=%u", action);
}

static void SafeUploadOnOff(intptr_t ctx)
{
  const bool isOn       = ctx != 0;
  g_bypass_zcl_callback = true;
  chip::app::Clusters::OnOff::Attributes::OnOff::Set(1, isOn);
  g_bypass_zcl_callback = false;
}

static void SafeUploadBrightness(intptr_t ctx)
{
  uint8_t pct = static_cast<uint8_t>(ctx);
  if (pct > 100)
    pct = 100;
  uint8_t level = static_cast<uint8_t>(static_cast<uint16_t>(pct) * 254 / 100);
  g_bypass_zcl_callback = true;
  chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Set(1, level);
  g_bypass_zcl_callback = false;
}

static void SafeUploadColorCt(intptr_t ctx)
{
  uint16_t mireds       = static_cast<uint16_t>(ctx);
  g_bypass_zcl_callback = true;
  chip::app::Clusters::ColorControl::Attributes::ColorTemperatureMireds::Set(
      1, mireds);
  g_bypass_zcl_callback = false;
}

void MatterBridge::uploadOnOff(bool on)
{
  PlatformMgr().ScheduleWork(SafeUploadOnOff, on ? 1 : 0);
}

void MatterBridge::uploadBrightnessPercent(uint8_t percent)
{
  PlatformMgr().ScheduleWork(SafeUploadBrightness, percent);
}

void MatterBridge::uploadColorFromCycleIndex(uint8_t cycleIndex)
{
  // 取该循环索引对应库 ID 的近似色温(mireds)，使 APP 颜色显示与设备同步
  uint8_t libId   = ColorLibrary::instance().libraryIdAtCycleIndex(cycleIndex);
  uint16_t mireds = ColorLibrary::instance().libraryIdToMireds(libId);
  LOG_MATTER("[MatterBridge] upload color cycleIdx=%u libId=%u mireds=%u",
             cycleIndex, libId, mireds);
  PlatformMgr().ScheduleWork(SafeUploadColorCt, mireds);
}

bool MatterBridge::isUnprovisioned() const
{
  return chip::Server::GetInstance().GetFabricTable().FabricCount() == 0;
}

static void DoOpenCommissioningWindow(intptr_t)
{
  chip::Server::GetInstance()
      .GetCommissioningWindowManager()
      .OpenBasicCommissioningWindow();
}

void MatterBridge::openCommissioningWindow()
{
  LOG_MATTER("[MatterBridge] open commissioning window");
  PlatformMgr().ScheduleWork(DoOpenCommissioningWindow, 0);
}

static void DoCloseCommissioningWindow(intptr_t)
{
  auto& mgr = chip::Server::GetInstance().GetCommissioningWindowManager();
  if (mgr.IsCommissioningWindowOpen())
    mgr.CloseCommissioningWindow();
}

void MatterBridge::closeCommissioningWindow()
{
  LOG_MATTER("[MatterBridge] close commissioning window");
  PlatformMgr().ScheduleWork(DoCloseCommissioningWindow, 0);
}

static void DoSoftNetworkResetHandler(intptr_t)
{
  chip::Server::GetInstance()
      .GetCommissioningWindowManager()
      .CloseCommissioningWindow();

  bool hasFabrics = false;
  for (const auto& f : chip::Server::GetInstance().GetFabricTable())
  {
    if (f.GetFabricIndex() != chip::kUndefinedFabricIndex)
      hasFabrics = true;
  }
  if (hasFabrics)
    chip::Server::GetInstance().GetFabricTable().DeleteAllFabrics();

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
  otInstance* otInst = ThreadStackMgrImpl().OTInstance();
  if (otInst)
  {
    otThreadSetEnabled(otInst, false);
    otIp6SetEnabled(otInst, false);
    otOperationalDataset empty{};
    std::memset(&empty, 0, sizeof(empty));
    otDatasetSetActive(otInst, &empty);
  }
#endif
  ConnectivityMgr().SetBLEAdvertisingEnabled(true);
}

void MatterBridge::triggerSoftNetworkReset()
{
  LOG_MATTER("[MatterBridge] schedule soft network reset");
  PlatformMgr().ScheduleWork(DoSoftNetworkResetHandler, 0);
}

static void OnMatterDeviceEvent(const ChipDeviceEvent* event, intptr_t)
{
  static bool animDone = false;
  if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
    animDone = false;

  if (event->Type == DeviceEventType::kCommissioningComplete && !animDone)
  {
    animDone = true;
    CommissioningManager::instance().onCommissioningComplete();
  }
}

void MatterBridge::registerDeviceEvents()
{
  LOG_MATTER("[MatterBridge] register device events + identify");
  PlatformMgr().AddEventHandler(OnMatterDeviceEvent, 0);
  initIdentifyCluster();
}

void MatterBridge::initIdentifyCluster()
{
  static Identify sIdentify(
      1, [](Identify*) { IdentifyEffect::instance().onStart(); },
      [](Identify*) { IdentifyEffect::instance().onStop(); },
      chip::app::Clusters::Identify::IdentifyTypeEnum::kNone);
  (void)sIdentify;
}

// C 兼容层
bool IsMatterReportBypassEnabled(void)
{
  return MatterBridge::instance().isReportBypassEnabled();
}

void Upload_Matter_OnOff(bool is_on)
{
  MatterBridge::instance().uploadOnOff(is_on);
}

void Upload_Matter_Brightness(uint8_t pct)
{
  MatterBridge::instance().uploadBrightnessPercent(pct);
}

bool IsMatterUnprovisioned(void)
{
  return MatterBridge::instance().isUnprovisioned();
}

void TriggerNetworkResetWithoutReboot(void)
{
  MatterBridge::instance().triggerSoftNetworkReset();
}

void RegisterDeviceEventListener(void)
{
  MatterBridge::instance().registerDeviceEvents();
}

void MyActionInitiatedBridge(int aAction, uint8_t* aValue, bool lightOn)
{
  MatterBridge::instance().onActionInitiated(aAction, aValue, lightOn);
}

void MyColorEventHandlerBridge(uint8_t action, void* valueData, uint16_t X,
                               uint16_t Y)
{
  (void)X;
  (void)Y;
  MatterBridge::instance().onColorEvent(action, valueData, X, Y);
}

void CommissioningFirstBreath_Stop(void)
{
  CommissioningManager::instance().stopFirstBreath();
}
