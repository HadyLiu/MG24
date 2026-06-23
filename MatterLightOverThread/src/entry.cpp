/**
 * @file entry.cpp
 * @brief 系统入口：App 层统一接线（各 Service/Middleware 仅通过回调互联）
 * @author hady
 * @date 2026-06-17
 * @layer App
 * @note
 * 接线总览：
 *   ButtonInput ──MailPoster──► ButtonService ──KeyEvent──► LightDecisionCenter
 *   MatterBridgeServer ──Downlink/Event──► (entry) ──► LightDecisionCenter
 *   LightDecisionCenter ──MatterReport/NetControl──► (entry) ──► MatterBridge
 *   BspPowerMonitor ──1s定时器──► entry_PowerPoll ──► LightDecisionCenter
 *   LightDecisionCenter ──► LightSequenceScheduler ──► LightEffectEngine ──►
 * BSP
 */
#include "BspLedIndicatorRed.h"
#include "BspLedIndicatorWhite.h"
#include "BspLedWrgb.h"
#include "BspPowerMonitor.h"
#include "BspTimer.h"

#include "ButtonInput.h"
#include "ButtonService.h"
#include "ColorConverter.h"
#include "DebugLog.h"
#include "LightDecisionCenter.h"
#include "LightDecisionTypes.h"
#include "LightEffectEngine.h"
#include "LightEffectProcessor.h"
#include "LightNvmStorage.h"
#include "LightSequenceScheduler.h"
#include "LightTypes.h"
#include "LightingManager.h"
#include "MatterBridge.h"
#include "MatterBridgeServer.h"

static BspTimer s_buttonScanTimer;  /**< 按键扫描 10ms */
static BspTimer s_lightRenderTimer; /**< 灯效渲染 10ms */
static BspTimer s_powerPollTimer; /**< 电量轮询 1000ms（独立，勿与按键共用） */
static LightNvmStorage s_lightNvmStorage;
static bool s_lastBatteryLow = false;

void button_Init(void);
void Indicator_Init(void);
void power_Init(void);
void led_Init(void);

static bool EntryMatterBypassQuery()
{
  return MatterBridgeServer::Instance().IsReportBypassEnabled();
}

static bool EntryMatterUnprovisionedQuery()
{
  return MatterBridgeServer::Instance().IsUnprovisioned();
}

///**
// * @brief 注册按键链路回调
// * @return 无
// */
// static void WireButtonChain(void)
//{
//  ButtonService::Instance().RegisterKeyEventHandler([](KeyEventType event) {
//    LightDecisionCenter::Instance().ProcessKeyEvent(event);
//  });
//
//  ButtonInput::Instance().RegisterTimerControlCallback(
//      [](bool start) { s_buttonScanTimer.Control(start); });
//}
//
///**
// * @brief LDC 状态变更时上报 Matter（开关/亮度/HSV）
// * @param on         是否点亮
// * @param brightness 亮度 0~255
// * @param pWrgb      逻辑 WRGB 四通道
// * @return 无
// */
// static void EntryReportLightToMatter(bool on, uint8_t brightness,
//                                     const uint16_t* pWrgb)
//{
//  MatterBridge::Instance().UploadOnOff(on);
//  const uint8_t pct =
//      static_cast<uint8_t>((static_cast<uint16_t>(brightness) * 100U) / 255U);
//  MatterBridge::Instance().UploadBrightnessPercent(pct);
//  if (pWrgb != nullptr)
//  {
//    const LightTypes::WrgbColor color{pWrgb[0], pWrgb[1], pWrgb[2], pWrgb[3]};
//    const LightTypes::HsvColor hsv = ColorConverter::ToHsv(color);
//    MatterBridge::Instance().UploadColorHsv(hsv.h, hsv.s);
//  }
//}
//
///**
// * @brief Matter 开关/亮度下行 → LightDecisionCenter
// * @param action  LightingManager 动作码
// * @param level   Matter CurrentLevel (0~254)
// * @param lightOn 开关辅助标志
// * @return 无
// */
// static void EntryOnMatterDownlinkAction(int action, uint8_t level, bool
// lightOn)
//{
//  (void)lightOn;
//  uint16_t wrgb[4] = {0U, 0U, 0U, 0U};
//  LightDecisionCenter::Instance().GetCurrentWrgb(wrgb, 4U);
//
//  if (action == LightingManager::ON_ACTION)
//  {
//    uint8_t brightness =
//    LightDecisionCenter::Instance().GetCurrentBrightness(); if (brightness ==
//    0U)
//    {
//      brightness = 128U;
//    }
//    LightDecisionCenter::Instance().ProcessMatterCommand(
//        wrgb, brightness, LightEffectOpId::Bezier40FadeIn);
//    return;
//  }
//
//  if (action == LightingManager::OFF_ACTION)
//  {
//    LightDecisionCenter::Instance().ProcessMatterCommand(
//        wrgb, 0U, LightEffectOpId::Bezier40FadeOut);
//    return;
//  }
//
//  if (action == LightingManager::LEVEL_ACTION)
//  {
//    const uint8_t brightness =
//        static_cast<uint8_t>((static_cast<uint16_t>(level) * 255U) / 254U);
//    LightDecisionCenter::Instance().ProcessMatterCommand(
//        wrgb, brightness, LightEffectOpId::LinearLerp);
//  }
//}
//
///**
// * @brief Matter 颜色下行 → LightDecisionCenter
// * @param wrgb 已转换的逻辑 WRGB 四通道
// * @return 无
// */
// static void EntryOnMatterDownlinkColor(const uint16_t wrgb[4])
//{
//  const uint8_t brightness =
//      LightDecisionCenter::Instance().GetCurrentBrightness();
//  LightDecisionCenter::Instance().ProcessMatterCommand(
//      wrgb, brightness, LightEffectOpId::LinearLerp);
//}
//
///**
// * @brief LDC 配网控制 → MatterBridge
// * @param action 清除并开配网窗 / 关闭配网窗
// * @return 无
// */
// static void EntryOnNetControl(NetControlAction action)
//{
//  if (action == NetControlAction::ClearAndOpen)
//  {
//    MatterBridge::Instance().TriggerSoftNetworkReset();
//    MatterBridge::Instance().OpenCommissioningWindow();
//  }
//  else if (action == NetControlAction::Close)
//  {
//    MatterBridge::Instance().CloseCommissioningWindow();
//  }
//}
//
///**
// * @brief 注册 Matter ↔ 灯光决策回调
// * @return 无
// */
// static void WireMatterToLight(void)
//{
//  MatterBridgeServer::Instance().Init();
//  MatterBridge::Instance().SetMailPoster(MatterBridgeServer::MailPoster);
//  MatterBridge::Instance().SetReportBypassQuery(EntryMatterBypassQuery);
//  MatterBridge::Instance().SetUnprovisionedQuery(EntryMatterUnprovisionedQuery);
//  MatterBridge::Instance().RegisterDeviceEvents();
//
//  MatterBridgeServer::Instance().RegisterDownlinkActionHandler(
//      EntryOnMatterDownlinkAction);
//  MatterBridgeServer::Instance().RegisterDownlinkColorHandler(
//      EntryOnMatterDownlinkColor);
//  MatterBridgeServer::Instance().RegisterCommissioningCompleteHandler([]() {
//    LightDecisionCenter::Instance().ProcessMatterCommissioningComplete();
//  });
//  MatterBridgeServer::Instance().RegisterIdentifyHandler([](bool active) {
//    LightDecisionCenter::Instance().ProcessMatterIdentify(active);
//  });
//  MatterBridgeServer::Instance().RegisterNetworkConnectedHandler(
//      []() { LightDecisionCenter::Instance().ReportStateToMatter(); });
//}
//
///**
// * @brief 初始化 LightDecisionCenter 并注册上报/配网回调
// * @return 无
// */
// static void WireLightDecision(void)
//{
//  LightDecisionCenter::Instance().Init(&LightSequenceScheduler::Instance(),
//                                       &s_lightNvmStorage);
//  LightDecisionCenter::Instance().RegisterMatterReporter(
//      EntryReportLightToMatter);
//  LightDecisionCenter::Instance().RegisterNetControlCallback(EntryOnNetControl);
//}
//
///**
// * @brief 电量轮询（1s 独立定时器调用，内含 ADC 读取勿放入按键 tick）
// * @return 无
// */
// static void entry_PowerPoll(void)
//{
//  const BatteryVoltStatusEnum voltStatus =
//      BspPowerMonitor::Instance().GetBatteryVoltStatus();
//  const bool isLow = (voltStatus ==
//  BatteryVoltStatusEnum::VOLT_CRITICAL_EMPTY); if (isLow != s_lastBatteryLow)
//  {
//    s_lastBatteryLow = isLow;
//    LightDecisionCenter::Instance().ProcessBatteryEvent(isLow);
//  }
//}

/**
 * @brief 系统总初始化入口
 * @return 无
 * @note 先 BSP/引擎，再 WireLightDecision，最后 Matter 接线。
 */
void entry_Init(void)
{
  button_Init();
  Indicator_Init();
  power_Init();
  led_Init();
  // WireLightDecision();
  // WireMatterToLight();
}

/**
 * @brief 按键链初始化（ButtonInput + ButtonService + 10ms 扫描定时器）
 * @return 无
 */
void button_Init(void)
{
  ButtonInput::Instance().Init();
  ButtonService::Instance().Init();
  // WireButtonChain();

  s_buttonScanTimer.Init(
      [](uint32_t elapsedMs) {
        ButtonInput::Instance().UpdateTicks(elapsedMs);
      },
      10);
  ButtonInput::Instance().RegisterMailPosterCallback(
      [](const ButtonMailMsg& msg) {
        ButtonService::Instance().DispatchMail(msg);
      });
}

/**
 * @brief 指示灯 BSP 初始化
 * @return 无
 */
void Indicator_Init(void)
{
  BspLedIndicatorRed::Instance().Init();
  BspLedIndicatorWhite::Instance().Init();
}

/**
 * @brief 电源 BSP 初始化 + 1s 电量轮询定时器
 * @return 无
 */
void power_Init(void)
{
  BspPowerMonitor::Instance().Init();

  s_powerPollTimer.Init(
      [](uint32_t elapsedMs) {
        (void)elapsedMs;
        // entry_PowerPoll();
      },
      1000U);
  s_powerPollTimer.Control(true);
}

/**
 * @brief 主灯引擎初始化（LightEffectEngine + 10ms 渲染定时器）
 * @return 无
 */
void TestLightSequence();
void led_Init(void)
{
  BspLedWrgb::Instance().Init();
  LightEffectEngine::Instance().Init(
      [](const uint16_t* channelDuties) {
        LOG_ENTRY("W=%u, R=%u, G=%u, B=%u", channelDuties[0], channelDuties[1],
                  channelDuties[2], channelDuties[3]);
        BspLedWrgb::Instance().LedWrgbSetDuty(
            channelDuties[0], channelDuties[1], channelDuties[2],
            channelDuties[3]);
      },
      BspLedWrgb::GetLedMaxNum(), BspLedWrgb::GetMaxPwmBits(),
      LightEffectProcessor::GetMaxFactorBits());

  s_lightRenderTimer.Init(
      [](uint32_t elapsedMs) {
        LightEffectEngine::Instance().UpdateTicks(elapsedMs);
      },
      10);
  LightSequenceScheduler::Instance().Init();
  LightEffectEngine::Instance().RegisterTimerControlCallback(
      [](bool start) { s_lightRenderTimer.Control(start); });

  TestLightSequence();
}

void TestLightSequence()
{
  static const LightSequenceScheduler::SequenceStep pairingNetworkShow[] = {
      // 红色亮起 3200ms -> 绿色亮起 3200ms
      {LightEffectProcessor::CalcBreath80BytesFactor,
       {0U, 1023U, 0U, 0U},
       255U,
       3200U,
       0U},
      {LightEffectProcessor::CalcBreath80BytesFactor,
       {0U, 0U, 1023U, 0U},
       255U,
       3200U,
       0U}};

  // 核心调用：传入剧本、步数、且开启 LoopForever 标志位
  LightSequenceScheduler::Instance().StartSequence(pairingNetworkShow, 2, true);
}