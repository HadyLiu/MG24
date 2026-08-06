/**
 * @file IndicatorServer.cpp
 * @brief 指示灯管理服务实现
 * @author hady
 * @date 2026-07-09
 * @layer Service
 * @note 白/红独立仲裁：
 *       白呼吸 = 充电；
 *       红闪背景 = 无电池/故障/过温/临界电量；
 *       红灯覆盖 = 配网重置/低电量警告时序（不抢占白通道）。
 */
#include "IndicatorServer.h"
#include "BlinkTimingSpec.h"
#include "BspLedIndicatorRed.h"
#include "BspLedIndicatorWhite.h"
#include "DebugLog.h"
#include "IndicatorEffectEngine.h"
#include "LightEffectProcessor.h"

namespace {

/** @brief 低电量警告：快闪×2（PRD §6 + 注） */
static constexpr IndicatorEffectEngine::BlinkSequenceStep kBatteryLowWarnSteps[] = {
    {nullptr, 0U, BlinkTimingSpec::kFastBlinkCycleMs, 1U},
};

/**
 * @brief 重置网络红灯（与主灯同步，PRD「注」）
 * @note 熄灭 400ms → 正常闪×3 → 慢闪×1 → 熄灭（慢闪结束即灭，与主灯同步）
 *       peakPwm=0 + GetKeep 表示保持熄灭（见 BeginRedBlinkSequenceStepRaw）
 */
static constexpr IndicatorEffectEngine::BlinkSequenceStep kNetConfigIndicatorSteps[] = {
    {LightEffectProcessor::GetKeep, 0U, BlinkTimingSpec::kResetOffLeadMs, 0U},
    {nullptr, 0U, BlinkTimingSpec::kNormalBlinkCycleMs, BlinkTimingSpec::kResetNormalBlinkExtraRepeats},
    {nullptr, 0U, BlinkTimingSpec::kSlowBlinkCycleMs, 0U},
};

/** @brief 红灯覆盖时序自然播完 → 恢复红通道背景 */
void OnRedOverrideSequenceFinishedBridge()
{
    IndicatorServer::Instance().OnRedOverrideSequenceFinishedRaw();
}

} // namespace

/**
 * @brief 初始化 BSP 与灯效引擎，注册红灯时序结束回调
 */
void IndicatorServer::Init()
{
    m_chargeInput             = BatteryChargeSnapshot{};
    m_chargeInputValid        = false;
    m_chargeEffects           = ChargeIndicatorEffect::Off;
    m_redOverrideActive                = false;
    m_redOverrideLoopForever           = false;
    m_firstCommissionBreathActive      = false;
    m_firstCommissionBreathSuppressed  = false;

    BspLedIndicatorRed::Instance().Init();
    BspLedIndicatorWhite::Instance().Init();
    IndicatorEffectEngine::Instance().Init();
    IndicatorEffectEngine::Instance().RegisterRedBlinkSequenceFinishedCallback(OnRedOverrideSequenceFinishedBridge);

    LOG_BAT("[IndicatorServer] init");
}

/**
 * @brief 查询当前充电侧背景灯效
 */
ChargeIndicatorEffect IndicatorServer::GetAppliedEffects() const
{
    return m_chargeEffects;
}

/**
 * @brief 由充电快照推导背景灯效
 */
ChargeIndicatorEffect IndicatorServer::ArbitrateChargeEffectsRaw(const BatteryChargeSnapshot& snapshot) const
{
    uint8_t effects = 0U;

    switch (snapshot.status)
    {
    case BatteryChargeStatus::Nobat:
    case BatteryChargeStatus::ChargeFault:
    case BatteryChargeStatus::TempFault:
        // §6.3：充电异常持续红快闪；临界电量不占背景红闪（仅开灯尝试时×2，见 §6.2）
        effects |= static_cast<uint8_t>(ChargeIndicatorEffect::RedBlink);
        break;
    case BatteryChargeStatus::ChargeDone:
        // §6.3：充满电系统 LED 熄灭
        break;
    default:
        break;
    }

    if (snapshot.status == BatteryChargeStatus::ChargeDone)
    {
        if (effects == 0U)
        {
            return ChargeIndicatorEffect::Off;
        }
        return static_cast<ChargeIndicatorEffect>(effects);
    }

    if (snapshot.chipValid && snapshot.chipCharging)
    {
        effects |= static_cast<uint8_t>(ChargeIndicatorEffect::WhiteBreath);
    }
    else if (snapshot.status == BatteryChargeStatus::Charging)
    {
        effects |= static_cast<uint8_t>(ChargeIndicatorEffect::WhiteBreath);
    }

    if (effects == 0U)
    {
        return ChargeIndicatorEffect::Off;
    }

    return static_cast<ChargeIndicatorEffect>(effects);
}

/**
 * @brief 聚合背景灯效（当前仅充电侧）
 */
ChargeIndicatorEffect IndicatorServer::ArbitrateBackgroundEffectsRaw() const
{
    if (!m_chargeInputValid)
    {
        return ChargeIndicatorEffect::Off;
    }

    return m_chargeEffects;
}

/**
 * @brief 下发白通道：充电呼吸或首次配网呼吸（任一需要即开）
 */
void IndicatorServer::ApplyWhiteChannelRaw()
{
    const uint8_t bgBits = static_cast<uint8_t>(ArbitrateBackgroundEffectsRaw());
    const bool chargeBreath =
        (bgBits & static_cast<uint8_t>(ChargeIndicatorEffect::WhiteBreath)) != 0U;
    const bool enableBreath = chargeBreath || m_firstCommissionBreathActive;

    auto& engine = IndicatorEffectEngine::Instance();
    if (enableBreath)
    {
        engine.StartWhiteBreath(kDefaultWhiteBreathBrightness);
    }
    else
    {
        engine.StopWhite();
    }
}

/**
 * @brief 下发红通道背景（红灯未被覆盖时）
 */
void IndicatorServer::ApplyRedBackgroundRaw(bool enableBlink)
{
    if (m_redOverrideActive)
    {
        return;
    }

    auto& engine = IndicatorEffectEngine::Instance();
    if (enableBlink)
    {
        engine.StartRedBlink();
    }
    else
    {
        engine.StopRed();
    }
}

/**
 * @brief 启动红灯覆盖时序，白通道按充电/首次配网刷新
 */
void IndicatorServer::StartRedOverrideSequenceRaw(const IndicatorEffectEngine::BlinkSequenceStep* steps, uint8_t count,
                                                  bool loopForever)
{
    if ((steps == nullptr) || (count == 0U))
    {
        return;
    }

    m_redOverrideActive      = true;
    m_redOverrideLoopForever = loopForever;

    auto& engine = IndicatorEffectEngine::Instance();
    engine.StopRed();
    engine.StartRedBlinkSequence(steps, count, loopForever);

    ApplyWhiteChannelRaw();

    LOG_BAT("IndicatorServer red override start, steps=%u loop=%u", count, loopForever ? 1U : 0U);
}

/**
 * @brief 红灯覆盖结束，恢复充电侧红闪背景
 */
void IndicatorServer::ResumeRedBackgroundRaw()
{
    m_redOverrideActive      = false;
    m_redOverrideLoopForever = false;

    const uint8_t bgBits = static_cast<uint8_t>(ArbitrateBackgroundEffectsRaw());
    ApplyRedBackgroundRaw((bgBits & static_cast<uint8_t>(ChargeIndicatorEffect::RedBlink)) != 0U);

    LOG_BAT("IndicatorServer red override end, resume red bg=0x%02X", bgBits);
}

/**
 * @brief 红灯一次性覆盖时序自然播完
 */
void IndicatorServer::OnRedOverrideSequenceFinishedRaw()
{
    if (!m_redOverrideActive || m_redOverrideLoopForever)
    {
        return;
    }

    ResumeRedBackgroundRaw();
}

/**
 * @brief 充电输入变化：白通道始终刷新，红通道仅在无覆盖时刷新
 */
void IndicatorServer::RefreshFromChargeInputRaw()
{
    m_chargeEffects = ArbitrateChargeEffectsRaw(m_chargeInput);
    const uint8_t bgBits = static_cast<uint8_t>(m_chargeEffects);

    ApplyWhiteChannelRaw();
    ApplyRedBackgroundRaw((bgBits & static_cast<uint8_t>(ChargeIndicatorEffect::RedBlink)) != 0U);
}

/**
 * @brief 消费 PowerServer 充电快照
 */
void IndicatorServer::OnChargeSnapshot(const BatteryChargeSnapshot& snapshot)
{
    m_chargeInput      = snapshot;
    m_chargeInputValid = true;
    RefreshFromChargeInputRaw();

    LOG_BAT("IndicatorServer charge status=%u effects=0x%02X", static_cast<uint8_t>(snapshot.status),
            static_cast<uint8_t>(m_chargeEffects));
}

/**
 * @brief 低电量警告：仅开灯边沿时红灯快闪×2（PRD §6.1）
 */
void IndicatorServer::OnBatteryLowWarn()
{
    LOG_BAT("IndicatorServer battery low warn");
    StartRedOverrideSequenceRaw(kBatteryLowWarnSteps,
                                static_cast<uint8_t>(sizeof(kBatteryLowWarnSteps) / sizeof(kBatteryLowWarnSteps[0])),
                                false);
}

/**
 * @brief 配网重置：红灯一次性时序
 */
void IndicatorServer::OnNetConfigIndicatorStart()
{
    LOG_BAT("IndicatorServer net-config indicator start");
    StartRedOverrideSequenceRaw(
        kNetConfigIndicatorSteps,
        static_cast<uint8_t>(sizeof(kNetConfigIndicatorSteps) / sizeof(kNetConfigIndicatorSteps[0])), false);
}

/**
 * @brief 配网结束：仅停止红灯覆盖
 */
void IndicatorServer::OnNetConfigIndicatorStop()
{
    if (!m_redOverrideActive)
    {
        return;
    }

    LOG_BAT("IndicatorServer net-config indicator stop");
    IndicatorEffectEngine::Instance().StopRed();
    ResumeRedBackgroundRaw();
}

/**
 * @brief 首次出厂配网：系统 LED 白呼吸（§3.2）
 */
void IndicatorServer::OnFirstCommissionBreathStart()
{
    if (m_firstCommissionBreathSuppressed || m_firstCommissionBreathActive)
    {
        return;
    }

    m_firstCommissionBreathActive = true;
    ApplyWhiteChannelRaw();
    LOG_BAT("IndicatorServer first-commission white breath start");
}

/**
 * @brief 配网成功或任意按键后：停止白呼吸（§3.2）
 */
void IndicatorServer::OnFirstCommissionBreathStop()
{
    m_firstCommissionBreathSuppressed = true;

    if (!m_firstCommissionBreathActive)
    {
        return;
    }

    m_firstCommissionBreathActive = false;
    ApplyWhiteChannelRaw();
    LOG_BAT("IndicatorServer first-commission white breath stop");
}

/**
 * @brief 熄灭全部指示灯
 */
void IndicatorServer::StopAll()
{
    m_chargeInputValid                 = false;
    m_chargeEffects                    = ChargeIndicatorEffect::Off;
    m_chargeInput                      = BatteryChargeSnapshot{};
    m_redOverrideActive                = false;
    m_redOverrideLoopForever           = false;
    m_firstCommissionBreathActive      = false;
    m_firstCommissionBreathSuppressed  = false;

    IndicatorEffectEngine::Instance().Stop();
    LOG_BAT("IndicatorServer stop all");
}
