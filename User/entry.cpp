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
 *   MatterBridge kCommissioningComplete ──► MatterBridgeServer ──► LDC 配对成功灯效
 *   PowerServer ──BatteryVolt/ChargeState──► (entry) ──► LightDecisionCenter / IndicatorServer
 *   LightEffectEngine ──OutputActivity──► (entry) ──► PowerServer
 *   LightDecisionCenter ──► LightSequenceScheduler ──► LightEffectEngine ──►
 * BSP
 */
#include "BspLedWrgb.h"
#include "BspPowerMonitor.h"
#include "BspTimer.h"

#include "ButtonInput.h"
#include "ButtonService.h"
#include "ColorConverter.h"
#include "DebugLog.h"
#include "IndicatorServer.h"
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

#include "LowPowerCoordinator.h"
#include "PowerServer.h"

static LightNvmStorage s_lightNvmStorage;

void power_Init(void);
void power_Wire(void);
void led_Init(void);
void Indicator_Init(void);
void LightDecisionCenter_Init(void);
void button_Init(void);
void Matter_Init(void);

/**
 * @brief 系统总初始化入口
 * @return 无
 * @note 先 BSP/引擎，再初始化 LDC，最后接线 Matter（须在 LDC.Init 之后）。
 */
void entry_Init(void)
{
    led_Init();

    /* 指示灯初始化 */
    Indicator_Init();

    /* 主灯控制中心 */
    LightDecisionCenter_Init();

    /* 电源服务接线（须在 LDC / 灯效引擎初始化之后） */
    power_Init();

    power_Wire();

    button_Init();

    Matter_Init();

    /* 低功耗协调：架构兼容，默认不进入真休眠（见 LowPowerConfig） */
    LowPowerCoordinator::Instance().Init();
    LowPowerCoordinator::Instance().OnMainLightActivity(
        LightEffectEngine::Instance().IsAnyChannelActive());
    LowPowerCoordinator::Instance().OnUsbPowerActivity(
        BspPowerMonitor::Instance().GetUsbStatus() == UsbConnectionStatusEnum::UsbConnected);
}

/**
 * @brief 电源 BSP 初始化 + 1s 电量轮询定时器
 * @return 无
 */
void power_Init(void)
{
    BspPowerMonitor::Instance().Init();
    PowerServer::Instance().Init();
}

/**
 * @brief 电源服务 ↔ 灯效 / LDC 接线
 * @return 无
 */
void power_Wire(void)
{
    // 主灯变化输出回调
    LightEffectEngine::Instance().RegisterOutputActivityCallback([](bool isActive) {
        PowerServer::Instance().OnLightOutputChanged(isActive);
        LowPowerCoordinator::Instance().OnMainLightActivity(isActive);
    });

    // 电池电压等级变化回调
    PowerServer::Instance().RegisterBatteryVoltHandler(
        [](BatteryVoltLevel level) { LightDecisionCenter::Instance().ProcessBatteryVoltLevel(level); });

    // 充电状态变化 → IndicatorServer 仲裁指示灯 + 低功耗 USB 保持位
    PowerServer::Instance().RegisterChargeStatusHandler([](const BatteryChargeSnapshot& snapshot) {
        IndicatorServer::Instance().OnChargeSnapshot(snapshot);
        const bool usbPresent =
            (BspPowerMonitor::Instance().GetUsbStatus() == UsbConnectionStatusEnum::UsbConnected);
        LowPowerCoordinator::Instance().OnUsbPowerActivity(usbPresent);
        const bool indicActive =
            (IndicatorServer::Instance().GetAppliedEffects() != ChargeIndicatorEffect::Off);
        LowPowerCoordinator::Instance().OnIndicatorActivity(indicActive);
    });

    // 注解10：拔 USB 切电池 — 先灭主灯再开 BAT_EN，随后 400ms 淡入
    PowerServer::Instance().RegisterUsbUnplugLightPrepareHandler([]() {
        LightDecisionCenter::Instance().PrepareUsbUnplugLightOffRaw();
    });

    PowerServer::Instance().RegisterLightPowerPathReadyHandler([](bool usbUnplugFadeIn) {
        const bool wantLight = (LightDecisionCenter::Instance().GetCurrentBrightness() > 0U) &&
                               !LightDecisionCenter::Instance().IsBatteryLowLocked();
        if (!wantLight && !usbUnplugFadeIn && !LightEffectEngine::Instance().IsAnyChannelActive())
        {
            return;
        }

        if (usbUnplugFadeIn)
        {
            LightDecisionCenter::Instance().RefreshOutputIfAllowed(true);
            return;
        }

        LightEffectEngine::Instance().RefreshHardwareOutput();
        LightDecisionCenter::Instance().RefreshOutputIfAllowed(false);
    });

    // 电池低电量警告 → IndicatorServer
    LightDecisionCenter::Instance().RegisterBatteryWarnIndicatorCallback(
        []() { IndicatorServer::Instance().OnBatteryLowWarn(); });

    // 读取主灯物理输出
    PowerServer::Instance().OnLightOutputChanged(LightEffectEngine::Instance().IsAnyChannelActive());
}

/**
 * @brief 主灯引擎初始化（LightEffectEngine + 10ms 渲染定时器）
 * @return 无
 */
void led_Init(void)
{
    BspLedWrgb::Instance().Init();
    LightEffectEngine::Instance().Init();
    LightSequenceScheduler::Instance().Init();

    // TestLightSequence();
}

/**
 * @brief 指示灯服务初始化
 * @return 无
 */
void Indicator_Init(void)
{
    IndicatorServer::Instance().Init();
}

/**
 * @brief 灯光决策中心初始化
 * @return 无
 */
void LightDecisionCenter_Init(void)
{
    LightDecisionCenter::Instance().Init(&LightSequenceScheduler::Instance(), &s_lightNvmStorage);
}

/**
 * @brief 按键链初始化（ButtonInput + ButtonService + 10ms 扫描定时器）
 * @return 无
 */
void button_Init(void)
{
    ButtonInput::Instance().Init();
    ButtonService::Instance().Init();

    // 注册按键语义事件投递至 ButtonService
    ButtonInput::Instance().RegisterMailPosterCallback(
        [](const ButtonMailMsg& msg) { ButtonService::Instance().DispatchMail(msg); });

    // 注册按键语义事件回调至 LDC / IndicatorServer
    ButtonService::Instance().RegisterKeyEventHandler([](KeyEventType event) {
        MatterBridge::Instance().NotifyUserInteraction();
        LowPowerCoordinator::Instance().RequestUserWake();

        if (event == KeyEventType::LongPressClearNetLighting)
        {
            // 约 8s：红灯与主灯预警时序；≥13s 武装后，时序完结才工厂复位
            IndicatorServer::Instance().OnNetConfigIndicatorStart();
        }
        else if (event == KeyEventType::LongPressStopNet)
        {
            // 未满 13s 松开：停红灯；主灯由 LDC 恢复
            IndicatorServer::Instance().OnNetConfigIndicatorStop();
        }

        LightDecisionCenter::Instance().ProcessKeyEvent(event);
    });

    // LDC 配网控制：工厂复位 / 打开配网窗
    LightDecisionCenter::Instance().RegisterNetControlCallback([](NetControlAction action) {
        if (action == NetControlAction::FactoryReset)
        {
            MatterBridge::Instance().MatterExecuteCmd(MatterExecuteElement::kClearNetwork);
        }
        else if (action == NetControlAction::OpenCommissioning)
        {
            // 手动/短按/灯开自动：强制刷新 15 分钟倒计时
            MatterBridge::Instance().RequestOpenCommissioningWindow(true);
        }
    });
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
    // 须在 MatterBridge::Init 开窗任务之前注册查询/回调
    MatterBridge::Instance().RegisterLightOnQuery(
        []() { return LightDecisionCenter::Instance().GetCurrentBrightness() > 0U; });

    MatterBridge::Instance().SetFirstCommissionPending(
        !LightDecisionCenter::Instance().HasCompletedFirstCommission());

    MatterBridge::Instance().RegisterCommissioningUiCallback(
        [](bool windowOpenedForFirstPair, bool commissioningDone) {
            if (commissioningDone)
            {
                IndicatorServer::Instance().OnFirstCommissionBreathStop();
                MatterBridge::Instance().SetFirstCommissionPending(false);
                return;
            }

            if (windowOpenedForFirstPair)
            {
                IndicatorServer::Instance().OnFirstCommissionBreathStart();
            }
        });

    MatterBridge::Instance().Init();
    MatterBridge::Instance().MatterDownlinkLocalRegister(
        [](const MatterDownlinkUploadPayload& mdc) { MatterBridgeServer::Instance().OnMatterDataReceived(mdc); });

    // 唯一上报：LDC 本地状态 → 同步下行缓存 + 写回 Matter 属性（防手机 Toggle 反了）
    LightDecisionCenter::Instance().RegisterMatterReporter(
        [](bool on, uint8_t brightness, const uint16_t* pWrgb) {
            MatterBridgeServer::Instance().SyncLocalLightState(on, brightness, pWrgb);

            MatterDownlinkUploadPayload onPayload{};
            onPayload.element = MatterDataElement::kOn;
            onPayload.on      = on;
            MatterBridge::Instance().MatterUploadLocalReport(onPayload);

            // MatterUploadBrightness 入参为 0~100%
            uint8_t percent = static_cast<uint8_t>((static_cast<uint16_t>(brightness) * 100U) / 255U);
            if ((brightness > 0U) && (percent == 0U))
            {
                percent = 1U;
            }
            MatterDownlinkUploadPayload levelPayload{};
            levelPayload.element    = MatterDataElement::kBrightness;
            levelPayload.brightness = percent;
            MatterBridge::Instance().MatterUploadLocalReport(levelPayload);

            if (pWrgb == nullptr)
            {
                return;
            }

            // 颜色上报：有 W → CT；无 W → HSV + XY
            const LightTypes::WrgbColor wrgb = {pWrgb[0], pWrgb[1], pWrgb[2], pWrgb[3]};

            if (pWrgb[0] > 0U)
            {
                uint32_t kelvin = ColorConverter::ToColorTemperature(wrgb);
                if (kelvin < 1U)
                {
                    kelvin = 2700U;
                }
                if (kelvin < 2200U)
                {
                    kelvin = 2200U;
                }
                if (kelvin > 6500U)
                {
                    kelvin = 6500U;
                }
                const uint16_t mireds = static_cast<uint16_t>(1000000U / kelvin);
                MatterDownlinkUploadPayload ctPayload{};
                ctPayload.element                   = MatterDataElement::kCt;
                ctPayload.color.ct.colorTemperature = mireds;
                LOG_MATTER("Color uplink CT: K=%lu mireds=%u WRGB=%u,%u,%u,%u",
                           static_cast<unsigned long>(kelvin), mireds, pWrgb[0], pWrgb[1], pWrgb[2],
                           pWrgb[3]);
                MatterBridge::Instance().MatterUploadLocalReport(ctPayload);
            }
            else
            {
                const LightTypes::HsvColor hsv = ColorConverter::ToHsv(wrgb);
                MatterDownlinkUploadPayload hsvPayload{};
                hsvPayload.element              = MatterDataElement::kHsv;
                hsvPayload.color.hsv.hue        = hsv.h;
                hsvPayload.color.hsv.saturation = hsv.s;
                LOG_MATTER("Color uplink HSV: H=%u S=%u WRGB=%u,%u,%u,%u", hsv.h, hsv.s, pWrgb[0],
                           pWrgb[1], pWrgb[2], pWrgb[3]);
                MatterBridge::Instance().MatterUploadLocalReport(hsvPayload);

                const LightTypes::XyColor xy = ColorConverter::ToXy(wrgb);
                MatterDownlinkUploadPayload xyPayload{};
                xyPayload.element    = MatterDataElement::kXy;
                xyPayload.color.xy.x = xy.x;
                xyPayload.color.xy.y = xy.y;
                LOG_MATTER("Color uplink XY: X=%u Y=%u", xy.x, xy.y);
                MatterBridge::Instance().MatterUploadLocalReport(xyPayload);
            }
        });

    // 唯一下行回调：Matter 数据(开关/亮度/WRGB/识别/配网) → LDC
    MatterBridgeServer::Instance().RegisterDownlinkHandler([](const MatterBridgeServer::DownlinkData& d) {
        switch (d.kind)
        {
        case MatterBridgeServer::DownlinkKind::kLightControl:
            LOG_LIGHT_DC("DownlinkKind: brightness=%u, W=%u, R=%u, G=%u, B=%u", d.brightness, d.wrgb[0], d.wrgb[1],
                         d.wrgb[2], d.wrgb[3]);
            LightDecisionCenter::Instance().ProcessMatterCommand(
                d.wrgb,
                d.brightness,
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
}

/**
 * ############################
 * 测试程序
 * ############################
 **/

/**
 * @brief 灯效调度器测试函数（演示链式时序）
 * @return 无
 */
void TestLightSequence()
{
    static const LightSequenceScheduler::SequenceStep pairingNetworkShow[] = {
        // 红色亮起 3200ms -> 绿色亮起 3200ms
        {LightEffectProcessor::CalcBreath80BytesFactor, {0U, 1023U, 0U, 0U}, 255U, 3200U, 0U},
        {LightEffectProcessor::CalcBreath80BytesFactor, {0U, 0U, 1023U, 0U}, 255U, 3200U, 0U}};

    // 核心调用：传入剧本、步数、且开启 LoopForever 标志位
    LightSequenceScheduler::Instance().StartSequence(pairingNetworkShow, 2, true);
}
