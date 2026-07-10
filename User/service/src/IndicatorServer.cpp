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
#include "BspLedIndicatorRed.h"
#include "BspLedIndicatorWhite.h"
#include "DebugLog.h"
#include "IndicatorEffectEngine.h"

namespace {

/** @brief 低电量警告红闪时序：800ms×3 + 2400ms×1 */
static constexpr IndicatorEffectEngine::BlinkSequenceStep kBatteryLowWarnSteps[] = {
    {nullptr, 0U, 800U, 2U},
    {nullptr, 0U, 2400U, 0U},
};

/** @brief 配网重置红灯：800ms 周期闪 3 次 + 1200ms 慢闪 1 次 */
static constexpr IndicatorEffectEngine::BlinkSequenceStep kNetConfigIndicatorSteps[] = {
    {nullptr, 0U, 800U, 2U},
    {nullptr, 0U, 2400U, 0U},
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
    m_redOverrideActive       = false;
    m_redOverrideLoopForever  = false;

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
    case BatteryChargeStatus::CriticalEmpty:
        effects |= static_cast<uint8_t>(ChargeIndicatorEffect::RedBlink);
        break;
    default:
        break;
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
 * @brief 下发白通道
 */
void IndicatorServer::ApplyWhiteChannelRaw(bool enableBreath)
{
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
 * @brief 启动红灯覆盖时序，白通道保持当前充电状态
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

    const uint8_t bgBits = static_cast<uint8_t>(ArbitrateBackgroundEffectsRaw());
    ApplyWhiteChannelRaw((bgBits & static_cast<uint8_t>(ChargeIndicatorEffect::WhiteBreath)) != 0U);

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

    ApplyWhiteChannelRaw((bgBits & static_cast<uint8_t>(ChargeIndicatorEffect::WhiteBreath)) != 0U);
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
 * @brief 低电量警告：红灯一次性时序
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
 * @brief 熄灭全部指示灯
 */
void IndicatorServer::StopAll()
{
    m_chargeInputValid       = false;
    m_chargeEffects          = ChargeIndicatorEffect::Off;
    m_chargeInput            = BatteryChargeSnapshot{};
    m_redOverrideActive      = false;
    m_redOverrideLoopForever = false;

    IndicatorEffectEngine::Instance().Stop();
    LOG_BAT("IndicatorServer stop all");
}
