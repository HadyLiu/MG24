/**
 * @file MatterBridgeServer.cpp
 * @brief Matter 服务层实现：邮箱消费 + Matter 栈操作
 * @author hady
 * @date 2026-06-16
 * @layer Service
 */
#include "MatterBridgeServer.h"

#include "ColorConverter.h"
#include "DebugLog.h"
#include "LightingManager.h"
#include "LightTypes.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/CodegenIntegration.h>
#include <app/server/Server.h>
#include <cmsis_os2.h>
#include <platform/CHIPDeviceLayer.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <openthread/dataset.h>
#include <platform/OpenThread/GenericThreadStackManagerImpl_OpenThread.h>
#endif

#include <cstring>

namespace
{

static constexpr uint32_t kQueueDepth = 8U;

osMessageQueueId_t s_mailQueue = nullptr;

static void SafeUploadOnOff(intptr_t ctx)
{
  const bool isOn = ctx != 0;
  MatterBridgeServer::Instance().SetReportBypass(true);
  chip::app::Clusters::OnOff::Attributes::OnOff::Set(1, isOn);
  MatterBridgeServer::Instance().SetReportBypass(false);
  LOG_MATTER("[MatterBridgeServer] upload onOff=%u",
             static_cast<unsigned>(isOn));
}

static void SafeUploadBrightness(intptr_t ctx)
{
  uint8_t pct = static_cast<uint8_t>(ctx);
  if (pct > 100U)
  {
    pct = 100U;
  }
  const uint8_t level = static_cast<uint8_t>(
      static_cast<uint16_t>(pct) * 254U / 100U);
  MatterBridgeServer::Instance().SetReportBypass(true);
  chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Set(1, level);
  MatterBridgeServer::Instance().SetReportBypass(false);
  LOG_MATTER("[MatterBridgeServer] upload brightness pct=%u level=%u", pct,
             level);
}

static void SafeUploadColorHsv(intptr_t ctx)
{
  const uint8_t hue = static_cast<uint8_t>(ctx & 0xFFU);
  const uint8_t sat = static_cast<uint8_t>((ctx >> 8) & 0xFFU);
  MatterBridgeServer::Instance().SetReportBypass(true);
  chip::app::Clusters::ColorControl::Attributes::CurrentHue::Set(1, hue);
  chip::app::Clusters::ColorControl::Attributes::CurrentSaturation::Set(1, sat);
  MatterBridgeServer::Instance().SetReportBypass(false);
  LOG_MATTER("[MatterBridgeServer] upload hue=%u sat=%u", hue, sat);
}

static void SafeUploadColorCt(intptr_t ctx)
{
  const uint8_t cycleIndex = static_cast<uint8_t>(ctx);
  const uint16_t mireds =
      static_cast<uint16_t>(250U + static_cast<uint16_t>(cycleIndex) * 10U);
  LOG_MATTER("[MatterBridgeServer] upload color cycleIdx=%u mireds=%u (stub)",
             cycleIndex, mireds);
  MatterBridgeServer::Instance().SetReportBypass(true);
  chip::app::Clusters::ColorControl::Attributes::ColorTemperatureMireds::Set(
      1, mireds);
  MatterBridgeServer::Instance().SetReportBypass(false);
}

static void DoOpenCommissioningWindow(intptr_t)
{
  LOG_MATTER("[MatterBridgeServer] open commissioning window");
  chip::Server::GetInstance()
      .GetCommissioningWindowManager()
      .OpenBasicCommissioningWindow();
}

static void DoCloseCommissioningWindow(intptr_t)
{
  LOG_MATTER("[MatterBridgeServer] close commissioning window");
  auto& mgr = chip::Server::GetInstance().GetCommissioningWindowManager();
  if (mgr.IsCommissioningWindowOpen())
  {
    mgr.CloseCommissioningWindow();
  }
}

static void DoSoftNetworkResetHandler(intptr_t)
{
  LOG_MATTER("[MatterBridgeServer] soft network reset");
  chip::Server::GetInstance()
      .GetCommissioningWindowManager()
      .CloseCommissioningWindow();

  bool hasFabrics = false;
  for (const auto& fabric : chip::Server::GetInstance().GetFabricTable())
  {
    if (fabric.GetFabricIndex() != chip::kUndefinedFabricIndex)
    {
      hasFabrics = true;
    }
  }
  if (hasFabrics)
  {
    chip::Server::GetInstance().GetFabricTable().DeleteAllFabrics();
  }

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
  otInstance* otInst = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
  if (otInst != nullptr)
  {
    otThreadSetEnabled(otInst, false);
    otIp6SetEnabled(otInst, false);
    otOperationalDataset empty{};
    std::memset(&empty, 0, sizeof(empty));
    otDatasetSetActive(otInst, &empty);
  }
#endif

  chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
}

static void OnMatterDeviceEvent(const chip::DeviceLayer::ChipDeviceEvent* event,
                                intptr_t)
{
  static bool animDone = false;
  if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
  {
    animDone = false;
  }

  if (event->Type == chip::DeviceLayer::DeviceEventType::kCommissioningComplete &&
      !animDone)
  {
    animDone = true;
    LOG_MATTER("[MatterBridgeServer] commissioning complete");
    MatterBridgeServer::Instance().NotifyCommissioningComplete();
  }

  if (event->Type ==
      chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged)
  {
    MatterBridgeServer::Instance().NotifyNetworkConnected();
  }
}

static void DoRegisterDeviceEvents(intptr_t)
{
  LOG_MATTER("[MatterBridgeServer] register device events + identify");
  chip::DeviceLayer::PlatformMgr().AddEventHandler(OnMatterDeviceEvent, 0);
  MatterBridgeServer::Instance().InitIdentifyCluster();
}

} // namespace

using namespace chip;
using namespace chip::DeviceLayer;

void MatterBridgeServer::EnsureQueue()
{
  if (s_mailQueue == nullptr)
  {
    s_mailQueue =
        osMessageQueueNew(kQueueDepth, sizeof(MatterMailMsg), nullptr);
  }
}

void MatterBridgeServer::Init()
{
  EnsureQueue();
  m_bypassZclCallback = false;
  m_drainScheduled    = false;
  LOG_MATTER("[MatterBridgeServer] init (queue ready)");
}

void MatterBridgeServer::RegisterDownlinkActionHandler(
    DownlinkActionHandler handler)
{
  m_downlinkAction = handler;
}

void MatterBridgeServer::RegisterDownlinkColorHandler(
    DownlinkColorHandler handler)
{
  m_downlinkColor = handler;
}

void MatterBridgeServer::RegisterCommissioningCompleteHandler(
    CommissioningCompleteHandler handler)
{
  m_commissioningComplete = handler;
}

void MatterBridgeServer::RegisterIdentifyHandler(IdentifyHandler handler)
{
  m_identify = handler;
}

void MatterBridgeServer::RegisterNetworkConnectedHandler(
    NetworkConnectedHandler handler)
{
  m_networkConnected = handler;
}

void MatterBridgeServer::NotifyCommissioningComplete()
{
  if (m_commissioningComplete != nullptr)
  {
    m_commissioningComplete();
  }
}

void MatterBridgeServer::NotifyNetworkConnected()
{
  if (m_networkConnected != nullptr)
  {
    m_networkConnected();
  }
}

void MatterBridgeServer::NotifyIdentify(bool active)
{
  if (m_identify != nullptr)
  {
    m_identify(active);
  }
}

void MatterBridgeServer::MailPoster(const MatterMailMsg& msg)
{
  Instance().Post(msg);
}

void MatterBridgeServer::Post(const MatterMailMsg& msg)
{
  EnsureQueue();
  if (s_mailQueue == nullptr)
  {
    return;
  }

  (void)osMessageQueuePut(s_mailQueue, &msg, 0U, 0U);
  RequestDrain();
}

void MatterBridgeServer::RequestDrain()
{
  if (m_drainScheduled)
  {
    return;
  }

  m_drainScheduled = true;
  PlatformMgr().ScheduleWork(DrainWork, 0);
}

void MatterBridgeServer::DrainWork(intptr_t ctx)
{
  (void)ctx;
  MatterBridgeServer& server = MatterBridgeServer::Instance();
  server.m_drainScheduled    = false;
  server.DrainMailQueue();
}

void MatterBridgeServer::DrainMailQueue()
{
  if (s_mailQueue == nullptr)
  {
    return;
  }

  MatterMailMsg msg{};
  while (osMessageQueueGet(s_mailQueue, &msg, nullptr, 0U) == osOK)
  {
    DispatchMail(msg);
  }
}

void MatterBridgeServer::DispatchMail(const MatterMailMsg& msg)
{
  switch (msg.cmd)
  {
  case MatterMailCmd::kUploadOnOff:
    PlatformMgr().ScheduleWork(SafeUploadOnOff, msg.byte0 ? 1 : 0);
    break;
  case MatterMailCmd::kUploadBrightness:
    PlatformMgr().ScheduleWork(SafeUploadBrightness, msg.byte0);
    break;
  case MatterMailCmd::kUploadColorCt:
    PlatformMgr().ScheduleWork(SafeUploadColorCt, msg.byte0);
    break;
  case MatterMailCmd::kUploadColorHsv:
    PlatformMgr().ScheduleWork(
        SafeUploadColorHsv,
        static_cast<intptr_t>(msg.byte0 | (static_cast<intptr_t>(msg.byte1) << 8)));
    break;
  case MatterMailCmd::kOpenCommissioning:
    PlatformMgr().ScheduleWork(DoOpenCommissioningWindow, 0);
    break;
  case MatterMailCmd::kCloseCommissioning:
    PlatformMgr().ScheduleWork(DoCloseCommissioningWindow, 0);
    break;
  case MatterMailCmd::kSoftNetworkReset:
    PlatformMgr().ScheduleWork(DoSoftNetworkResetHandler, 0);
    break;
  case MatterMailCmd::kRegisterDeviceEvents:
    PlatformMgr().ScheduleWork(DoRegisterDeviceEvents, 0);
    break;
  case MatterMailCmd::kDownlinkAction:
    HandleDownlinkAction(static_cast<int>(msg.byte0),
                         static_cast<uint8_t>(msg.word0 & 0xFFU),
                         msg.byte1 != 0U);
    break;
  case MatterMailCmd::kDownlinkColor:
    HandleDownlinkColor(msg.byte0, msg.byte1,
                        static_cast<uint8_t>(msg.word1 & 0xFFU), msg.word0,
                        msg.word0, msg.word1);
    break;
  default:
    break;
  }
}

void MatterBridgeServer::SetReportBypass(bool enabled)
{
  m_bypassZclCallback = enabled;
}

uint8_t MatterBridgeServer::ClampMatterLevel(uint8_t matterLevel)
{
  if (matterLevel > 0U && matterLevel < 5U)
  {
    return 5U;
  }
  return matterLevel;
}

bool MatterBridgeServer::IsUnprovisioned() const
{
  return chip::Server::GetInstance().GetFabricTable().FabricCount() == 0;
}

void MatterBridgeServer::InitIdentifyCluster()
{
  static Identify sIdentify(
      1,
      [](Identify*) {
        LOG_MATTER("[MatterBridgeServer] identify start");
        MatterBridgeServer::Instance().NotifyIdentify(true);
      },
      [](Identify*) {
        LOG_MATTER("[MatterBridgeServer] identify stop");
        MatterBridgeServer::Instance().NotifyIdentify(false);
      },
      chip::app::Clusters::Identify::IdentifyTypeEnum::kNone);
  (void)sIdentify;
}

void MatterBridgeServer::HandleDownlinkAction(int action, uint8_t level,
                                              bool lightOn)
{
  if (m_downlinkAction != nullptr)
  {
    m_downlinkAction(action, level, lightOn);
    return;
  }

  LOG_MATTER("[MatterBridgeServer] downlink action=%d (no handler)", action);
  (void)level;
  (void)lightOn;
}

void MatterBridgeServer::HandleDownlinkColor(uint8_t action, uint8_t hue,
                                             uint8_t saturation,
                                             uint16_t ctMireds, uint16_t x,
                                             uint16_t y)
{
  LightTypes::WrgbColor wrgb{0U, 0U, 0U, 0U};

  switch (static_cast<LightingManager::Action_t>(action))
  {
  case LightingManager::COLOR_ACTION_HSV: {
    LightTypes::HsvColor hsv{};
    hsv.h = hue;
    hsv.s = saturation;
    hsv.v = 254U;
    wrgb  = ColorConverter::FromHsv(hsv);
    break;
  }
  case LightingManager::COLOR_ACTION_CT: {
    const uint32_t kelvin =
        (ctMireds > 0U) ? (1000000U / ctMireds) : 2700U;
    wrgb = ColorConverter::FromColorTemperature(kelvin);
    break;
  }
  case LightingManager::COLOR_ACTION_XY: {
    LightTypes::XyColor xy{};
    xy.x = x;
    xy.y = y;
    wrgb = ColorConverter::FromXy(xy);
    break;
  }
  default:
    return;
  }

  if (m_downlinkColor != nullptr)
  {
    const uint16_t wrgbChannels[4] = {wrgb.w, wrgb.r, wrgb.g, wrgb.b};
    m_downlinkColor(wrgbChannels);
  }
  else
  {
    LOG_MATTER("[MatterBridgeServer] downlink color action=%u (no handler)",
               action);
  }
}
