/**
 * @file IndicatorServer.cpp
 * @brief 指示灯管理服务实现
 * @author hady
 * @date 2026-07-09
 * @layer Service
 * @note 仲裁规则：
 *       红闪 = 无电池 / 充电故障 / 过温 / 临界电量；
 *       白呼吸 = 芯片 CHARGING 或综合状态 Charging；
 *       两路可叠加；充满 / 空闲 / 低电提示 → 熄灭。
 *       高优先级一次性红闪时序（如低电量警告）播完前挂起背景灯效，
 *       经 IndicatorEffectEngine 结束回调恢复。
 */
#include "IndicatorServer.h"
#include "BspLedIndicatorRed.h"
#include "BspLedIndicatorWhite.h"
#include "DebugLog.h"
#include "IndicatorEffectEngine.h"

namespace {

/** @brief 低电量警告红闪时序：800ms×3 + 1200ms×1 */
static constexpr IndicatorEffectEngine::BlinkSequenceStep kBatteryLowWarnSteps[] = {
    {nullptr, 0U, 800U, 2U},
    {nullptr, 0U, 1200U, 0U},
};

/** @brief 引擎红闪时序自然播完 → IndicatorServer 恢复背景灯效 */
void OnIndicatorEffectFinishedBridge()
{
    IndicatorServer::Instance().OnHighPriorityEffectFinishedRaw();
}

} // namespace

/**
 * @brief 初始化 BSP 与灯效引擎，注册时序结束回调
 */
void IndicatorServer::Init()
{
    m_chargeInput         = BatteryChargeSnapshot{};
    m_chargeInputValid    = false;
    m_chargeEffects       = ChargeIndicatorEffect::Off;
    m_lastApplied         = ChargeIndicatorEffect::Off;
    m_highPriorityActive  = false;

    BspLedIndicatorRed::Instance().Init();
    BspLedIndicatorWhite::Instance().Init();
    IndicatorEffectEngine::Instance().Init();
    IndicatorEffectEngine::Instance().RegisterRedBlinkSequenceFinishedCallback(OnIndicatorEffectFinishedBridge);

    LOG_BAT("[IndicatorServer] init");
}

/**
 * @brief 查询当前已下发灯效
 */
ChargeIndicatorEffect IndicatorServer::GetAppliedEffects() const
{
    return m_lastApplied;
}

/**
 * @brief 由充电快照推导灯效请求
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

    /* 芯片仍在充电时保留白呼吸，与故障红闪不互斥 */
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
 * @brief 聚合各输入源（当前仅充电侧，预留低电量等扩展）
 */
ChargeIndicatorEffect IndicatorServer::ArbitrateOutputRaw() const
{
    if (!m_chargeInputValid)
    {
        return ChargeIndicatorEffect::Off;
    }

    return m_chargeEffects;
}

/**
 * @brief 将位标志同步至 IndicatorEffectEngine
 */
void IndicatorServer::ApplyOutputRaw(ChargeIndicatorEffect effects, bool forceApply)
{
    if (!forceApply && (effects == m_lastApplied))
    {
        return;
    }

    auto&         engine     = IndicatorEffectEngine::Instance();
    const uint8_t effectBits = static_cast<uint8_t>(effects);

    if ((effectBits & static_cast<uint8_t>(ChargeIndicatorEffect::WhiteBreath)) != 0U)
    {
        engine.StartWhiteBreath(kDefaultWhiteBreathBrightness);
    }
    else
    {
        engine.StopWhite();
    }

    if ((effectBits & static_cast<uint8_t>(ChargeIndicatorEffect::RedBlink)) != 0U)
    {
        engine.StartRedBlink();
    }
    else
    {
        engine.StopRed();
    }

    m_lastApplied = effects;

    LOG_BAT("IndicatorServer apply effects=0x%02X", effectBits);
}

/**
 * @brief 启动高优先级一次性红闪时序，挂起背景灯效直至播完回调
 */
void IndicatorServer::StartHighPriorityRedSequenceRaw(const IndicatorEffectEngine::BlinkSequenceStep* steps,
                                                      uint8_t                                             count)
{
    if ((steps == nullptr) || (count == 0U))
    {
        return;
    }

    m_highPriorityActive = true;
    m_lastApplied        = ChargeIndicatorEffect::Off;

    auto& engine = IndicatorEffectEngine::Instance();
    engine.Stop();
    engine.StartRedBlinkSequence(steps, count, false);

    LOG_BAT("IndicatorServer high-priority red sequence start, steps=%u", count);
}

/**
 * @brief 高优先级时序自然播完：恢复背景仲裁结果
 */
void IndicatorServer::OnHighPriorityEffectFinishedRaw()
{
    if (!m_highPriorityActive)
    {
        return;
    }

    m_highPriorityActive = false;
    m_lastApplied        = ChargeIndicatorEffect::Off;

    const ChargeIndicatorEffect backgroundEffects = ArbitrateOutputRaw();
    ApplyOutputRaw(backgroundEffects, true);

    LOG_BAT("IndicatorServer high-priority finished, resume effects=0x%02X",
            static_cast<uint8_t>(backgroundEffects));
}

/**
 * @brief 充电输入更新后重新仲裁
 */
void IndicatorServer::RefreshFromChargeInputRaw()
{
    m_chargeEffects = ArbitrateChargeEffectsRaw(m_chargeInput);
    if (m_highPriorityActive)
    {
        return;
    }

    ApplyOutputRaw(ArbitrateOutputRaw());
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
 * @brief 低电量警告：高优先级一次性红闪，播完后恢复充电背景灯效
 */
void IndicatorServer::OnBatteryLowWarn()
{
    LOG_BAT("IndicatorServer battery low warn");
    StartHighPriorityRedSequenceRaw(kBatteryLowWarnSteps,
                                    static_cast<uint8_t>(sizeof(kBatteryLowWarnSteps) / sizeof(kBatteryLowWarnSteps[0])));
}

/**
 * @brief 熄灭全部指示灯
 */
void IndicatorServer::StopAll()
{
    m_chargeInputValid   = false;
    m_chargeEffects      = ChargeIndicatorEffect::Off;
    m_chargeInput        = BatteryChargeSnapshot{};
    m_highPriorityActive = false;

    IndicatorEffectEngine::Instance().Stop();
    m_lastApplied = ChargeIndicatorEffect::Off;

    LOG_BAT("IndicatorServer stop all");
}
