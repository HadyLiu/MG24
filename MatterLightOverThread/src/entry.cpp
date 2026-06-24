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

#include "AppConfig.h"
#include "AppEvent.h"
#include "AppTask.h"

static BspTimer s_buttonScanTimer;  /**< 按键扫描 10ms */
static BspTimer s_lightRenderTimer; /**< 灯效渲染 10ms */
static BspTimer s_powerPollTimer; /**< 电量轮询 1000ms（独立，勿与按键共用） */
static LightNvmStorage s_lightNvmStorage;

void button_Init(void);
void Indicator_Init(void);
void power_Init(void);
void led_Init(void);
void LightDecisionCenter();
void Matter_Init(void);

/**
 * @brief LDC 配网控制 → MatterBridge
 * @param action 清除并开配网窗 / 关闭配网窗
 * @return 无
 */
static void EntryOnNetControl(NetControlAction action)
{
  // if (action == NetControlAction::ClearAndOpen)
  // {
  //   MatterBridge::Instance().TriggerSoftNetworkReset();
  //   MatterBridge::Instance().OpenCommissioningWindow();
  // }
  // else if (action == NetControlAction::Close)
  // {
  //   MatterBridge::Instance().CloseCommissioningWindow();
  // }
}

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
 * @note 先 BSP/引擎，再初始化 LDC，最后接线 Matter（须在 LDC.Init 之后）。
 */
void entry_Init(void)
{
  button_Init();
  Indicator_Init();
  power_Init();
  led_Init();

  // LED 中心
  LightDecisionCenter();

  // Matter ↔ LDC 接线：唯一上报 + 唯一下行
  Matter_Init();
}

/**
 * @brief 工厂复位执行体（AppTask 任务上下文分发，非中断）
 * @param aEvent 占位，未使用
 * @return 无
 * @note 按键事件链跑在 sleeptimer 中断里，直接调用 InitiateFactoryReset 会
 *       因 ScheduleWork 无法从中断投递 CHIP 队列而失败。须转任务上下文执行。
 */
static void EntryClearNetWork(AppEvent* aEvent)
{
  (void)aEvent;
  LOG_LIGHT_DC("InitiateFactoryReset (task ctx)");
  chip::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
}

/**
 * @brief 按键链初始化（ButtonInput + ButtonService + 10ms 扫描定时器）
 * @return 无
 */
void button_Init(void)
{
  ButtonInput::Instance().Init();
  ButtonService::Instance().Init();

  s_buttonScanTimer.Init(
      [](uint32_t elapsedMs) {
        ButtonInput::Instance().UpdateTicks(elapsedMs);
      },
      10);
  ButtonInput::Instance().RegisterTimerControlCallback(
      [](bool start) { s_buttonScanTimer.Control(start); });

  // 注册按键语义事件投递至 ButtonService
  ButtonInput::Instance().RegisterMailPosterCallback(
      [](const ButtonMailMsg& msg) {
        ButtonService::Instance().DispatchMail(msg);
      });

  // 注册按键语义事件回调至 LDC
  ButtonService::Instance().RegisterKeyEventHandler([](KeyEventType event) {
    if (event == KeyEventType::LongPressClearNet)
    {
      // 清理配网信息：转任务上下文执行，避免在中断里调用 Matter 栈失败
      LOG_LIGHT_DC("KeyEvent: LongPressClearNet");
      AppEvent ev{};
      ev.Type    = AppEvent::kEventType_Button;
      ev.Handler = EntryClearNetWork;
      AppTask::GetAppTask().PostEvent(&ev);
    }
    LightDecisionCenter::Instance().ProcessKeyEvent(event);
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

  // TestLightSequence();
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

void LightDecisionCenter()
{
  LightDecisionCenter::Instance().Init(&LightSequenceScheduler::Instance(),
                                       &s_lightNvmStorage);
}

/**
 * @brief Matter ↔ 灯光决策接线（在 entry_Init 中调用）
 * @return 无
 * @note 仅两个出入口：
 *   - 唯一上报：LDC 灯光状态 → MatterBridgeServer::UploadState
 *   - 唯一下行：MatterBridgeServer::DownlinkHandler 按 kind 派发到 LDC
 */
void Matter_Init(void)
{

  MatterBridge::Instance().Init();
  MatterBridge::Instance().RegisterMatterDownlinkLocal(
      [](const MatterDownlinkUploadPayload& mdc) {
        MatterBridgeServer::Instance().OnMatterDataReceived(mdc);
      });

  // 唯一下行回调：Matter 数据(开关/亮度/WRGB/识别/配网) → LDC
  MatterBridgeServer::Instance().RegisterDownlinkHandler(
      [](const MatterBridgeServer::DownlinkData& d) {
        switch (d.kind)
        {
        case MatterBridgeServer::DownlinkKind::kLightControl:
          LOG_LIGHT_DC(
              "DownlinkKind: brightness=%u, W=%u, R=%u, G=%u, B=%u",
              d.brightness, d.wrgb[0], d.wrgb[1], d.wrgb[2], d.wrgb[3]);
          LightDecisionCenter::Instance().ProcessMatterCommand(
              d.wrgb, d.brightness,
              d.brightness > 0U ? LightEffectOpId::LinearLerp
                                : LightEffectOpId::Bezier40FadeOut);
          break;
        case MatterBridgeServer::DownlinkKind::kIdentify:
          LOG_LIGHT_DC("Identify: on=%u", d.on);
          LightDecisionCenter::Instance().ProcessMatterIdentify(d.on);
          break;
        case MatterBridgeServer::DownlinkKind::kCommissioningDone:
          LOG_LIGHT_DC("CommissioningDone");
          LightDecisionCenter::Instance().ProcessMatterCommissioningComplete();
          break;
        case MatterBridgeServer::DownlinkKind::kNetworkConnected:
          LOG_LIGHT_DC("NetworkConnected: request full report");
          LightDecisionCenter::Instance().ReportStateToMatter();
          break;
        }
      });

  // 初始化内部已完成 MatterBridge 投递/查询/设备事件接线
  //  MatterBridgeServer::Instance().Init();

  //// 唯一上报接口：LDC 灯光状态(开关/亮度/WRGB) → Matter
  // LightDecisionCenter::Instance().RegisterMatterReporter(
  //     [](bool on, uint8_t brightness, const uint16_t* pWrgb) {
  //       LOG_LIGHT_DC(
  //           "UploadState: on=%u, brightness=%u, W=%u, R=%u, G=%u, B=%u", on,
  //           brightness, pWrgb[0], pWrgb[1], pWrgb[2], pWrgb[3]);
  //       MatterBridgeServer::Instance().UploadState(on, brightness, pWrgb);
  //     });
  // LightDecisionCenter::Instance().RegisterNetControlCallback(EntryOnNetControl);
  //
  //// 唯一下行回调：Matter 数据(开关/亮度/WRGB/识别) → LDC
  // MatterBridgeServer::Instance().RegisterDownlinkHandler(
  //     [](const MatterBridgeServer::DownlinkData& d) {
  //       switch (d.kind)
  //       {
  //       case MatterBridgeServer::DownlinkKind::kLightControl:
  //         LightDecisionCenter::Instance().ProcessMatterCommand(
  //             d.wrgb, d.brightness,
  //             d.brightness > 0U ? LightEffectOpId::LinearLerp
  //                               : LightEffectOpId::Bezier40FadeOut);
  //         LOG_LIGHT_DC("DownlinkKind: brightness=%u, W=%u, R=%u, G=%u, B=%u",
  //                      d.brightness, d.wrgb[0], d.wrgb[1], d.wrgb[2],
  //                      d.wrgb[3]);
  //         break;
  //       case MatterBridgeServer::DownlinkKind::kIdentify:
  //         LOG_LIGHT_DC("Identify: on=%u", d.on);
  //         LightDecisionCenter::Instance().ProcessMatterIdentify(d.on);
  //         break;
  //       case MatterBridgeServer::DownlinkKind::kCommissioningDone:
  //         LOG_LIGHT_DC("CommissioningDone");
  //         LightDecisionCenter::Instance().ProcessMatterCommissioningComplete();
  //         break;
  //       case MatterBridgeServer::DownlinkKind::kNetworkConnected:
  //         LOG_LIGHT_DC("NetworkConnected: request full report");
  //         LightDecisionCenter::Instance().ReportStateToMatter();
  //         break;
  //       }
  //     });
}
