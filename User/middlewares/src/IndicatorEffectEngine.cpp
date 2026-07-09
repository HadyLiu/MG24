/**
 * @file IndicatorEffectEngine.cpp
 * @brief 指示灯灯效引擎实现
 * @author hady
 * @date 2026-06-26
 * @layer Middlewares
 * @note 物理 PWM 插值模型：
 *       峰值占空比 = 亮度 × kIndicatorPwmMax / 255；
 *       算子返回值须 >>m_operatorBits；红灯闪烁以 duty>0 映射为开关。
 *       白/红双通道独立渲染，可叠加（如充电白呼吸 + 故障红闪）。
 */
#include "IndicatorEffectEngine.h"
#include "BspLedIndicatorRed.h"
#include "BspLedIndicatorWhite.h"
#include "BspTimer.h"
#include "LightEffectProcessor.h"

static BspTimer s_indicatorRenderTimer; /**< 指示灯渲染 10ms tick */

/**
 * @brief 启停渲染定时器
 * @param start true=拉起，false=挂起（仅当正在运行时才关闭）
 */
void IndicatorEffectEngine::TimerStartStop(bool start)
{
    if (start)
    {
        s_indicatorRenderTimer.TurnOnOff(true);
    }
    else if (s_indicatorRenderTimer.IsRunning())
    {
        s_indicatorRenderTimer.TurnOnOff(false);
    }
}

/**
 * @brief 初始化引擎：复位双通道状态、读取算子位宽、注册 10ms 回调
 */
void IndicatorEffectEngine::Init()
{
    m_whiteMode      = WhiteEffectMode::None;
    m_redMode        = RedEffectMode::None;
    m_whiteDynamic   = DynamicChannelRuntime{};
    m_redDynamic     = DynamicChannelRuntime{};
    m_whiteSteadyPwm = 0U;
    m_operatorBits   = LightEffectProcessor::GetMaxFactorBits();

    s_indicatorRenderTimer.Init([](uint16_t elapsedMs) { IndicatorEffectEngine::Instance().UpdateTicks(elapsedMs); },
                                10U, nullptr);
}

/**
 * @brief 聚合两路通道状态
 * @return 当前 EngineState
 */
IndicatorEffectEngine::EngineState IndicatorEffectEngine::DeriveEngineStateRaw() const
{
    const bool whiteDynamic = (m_whiteMode == WhiteEffectMode::Breath) || (m_whiteMode == WhiteEffectMode::Blink);
    const bool redDynamic   = (m_redMode == RedEffectMode::Blink);

    if (whiteDynamic || redDynamic)
    {
        return EngineState::Running;
    }

    const bool whiteActive = (m_whiteMode == WhiteEffectMode::On);
    const bool redActive   = (m_redMode == RedEffectMode::On);
    if (whiteActive || redActive)
    {
        return EngineState::Steady;
    }

    return EngineState::Idle;
}

/**
 * @brief 查询引擎聚合运行状态
 * @return 当前 EngineState
 */
IndicatorEffectEngine::EngineState IndicatorEffectEngine::GetEngineState() const
{
    return DeriveEngineStateRaw();
}

/**
 * @brief 查询白灯通道模式
 * @return 当前 WhiteEffectMode
 */
IndicatorEffectEngine::WhiteEffectMode IndicatorEffectEngine::GetWhiteEffectMode() const
{
    return m_whiteMode;
}

/**
 * @brief 查询红灯通道模式
 * @return 当前 RedEffectMode
 */
IndicatorEffectEngine::RedEffectMode IndicatorEffectEngine::GetRedEffectMode() const
{
    return m_redMode;
}

/**
 * @brief 算子 Q12 混合值归一化为物理 PWM
 * @param rawMixed 算子返回值（定点）
 * @return 限幅后的占空比 0~kIndicatorPwmMax
 */
uint16_t IndicatorEffectEngine::ClampDutyRaw(uint32_t rawMixed) const
{
    const uint32_t duty = rawMixed >> m_operatorBits;
    if (duty > kIndicatorPwmMax)
    {
        return kIndicatorPwmMax;
    }
    return static_cast<uint16_t>(duty);
}

/**
 * @brief 周期参数限幅，防止过短/过长导致视觉异常或 tick 溢出
 * @param cycleMs 请求周期 (ms)
 * @return 限幅后的周期
 */
uint16_t IndicatorEffectEngine::NormalizeBlinkCycleMsRaw(uint16_t cycleMs) const
{
    if (cycleMs < kMinBlinkCycleMs)
    {
        return kMinBlinkCycleMs;
    }
    if (cycleMs > kMaxBlinkCycleMs)
    {
        return kMaxBlinkCycleMs;
    }
    return cycleMs;
}

/**
 * @brief 逻辑亮度映射为物理 PWM 峰值
 * @param brightness 0~255
 * @return 物理占空比 0~kIndicatorPwmMax
 */
uint16_t IndicatorEffectEngine::BrightnessToPwmRaw(uint8_t brightness) const
{
    return static_cast<uint16_t>((static_cast<uint32_t>(kIndicatorPwmMax) * brightness) / 255U);
}

/**
 * @brief 仅输出白灯 PWM，不影响红灯
 * @param duty 物理占空比
 */
void IndicatorEffectEngine::ApplyWhiteDutyRaw(uint16_t duty)
{
    BspLedIndicatorWhite::Instance().IndicatorWhiteSetDuty(duty);
}

/**
 * @brief 仅输出红灯开关，不影响白灯
 * @param on true=亮，false=灭
 */
void IndicatorEffectEngine::ApplyRedOnRaw(bool on)
{
    BspLedIndicatorRed::Instance().SetRedIndicator(on);
}

/**
 * @brief 根据两路动态状态决定定时器启停
 */
void IndicatorEffectEngine::SyncRenderTimerRaw()
{
    const bool needTimer = (DeriveEngineStateRaw() == EngineState::Running);
    TimerStartStop(needTimer);
}

/**
 * @brief 启动白灯动态灯效
 */
void IndicatorEffectEngine::StartWhiteDynamicRaw(WhiteEffectMode mode, EffectRenderAction action, uint32_t peakPwm,
                                                 uint16_t cycleMs)
{
    if ((m_whiteMode == mode) && (m_whiteDynamic.pAction == action) && (m_whiteDynamic.peakPwm == peakPwm) &&
        (m_whiteDynamic.cycleMs == cycleMs))
    {
        return;
    }

    m_whiteMode              = mode;
    m_whiteDynamic.pAction   = action;
    m_whiteDynamic.peakPwm   = peakPwm;
    m_whiteDynamic.cycleMs   = cycleMs;
    m_whiteDynamic.elapsedMs = 0U;

    RenderWhiteFrameRaw();
    SyncRenderTimerRaw();
}

/**
 * @brief 启动红灯动态灯效
 */
void IndicatorEffectEngine::StartRedDynamicRaw(RedEffectMode mode, EffectRenderAction action, uint32_t peakPwm,
                                               uint16_t cycleMs)
{
    if ((m_redMode == mode) && (m_redDynamic.pAction == action) && (m_redDynamic.peakPwm == peakPwm) &&
        (m_redDynamic.cycleMs == cycleMs))
    {
        return;
    }

    m_redMode              = mode;
    m_redDynamic.pAction   = action;
    m_redDynamic.peakPwm   = peakPwm;
    m_redDynamic.cycleMs   = cycleMs;
    m_redDynamic.elapsedMs = 0U;

    RenderRedFrameRaw();
    SyncRenderTimerRaw();
}

/**
 * @brief 渲染白灯通道当前帧
 */
void IndicatorEffectEngine::RenderWhiteFrameRaw()
{
    switch (m_whiteMode)
    {
    case WhiteEffectMode::On:
        ApplyWhiteDutyRaw(static_cast<uint16_t>(m_whiteSteadyPwm));
        break;

    case WhiteEffectMode::Breath:
    case WhiteEffectMode::Blink: {
        if (m_whiteDynamic.pAction == nullptr)
        {
            ApplyWhiteDutyRaw(0U);
            break;
        }

        const uint16_t phaseMs =
            static_cast<uint16_t>(m_whiteDynamic.elapsedMs % static_cast<uint32_t>(m_whiteDynamic.cycleMs));
        const uint32_t rawMixed = m_whiteDynamic.pAction(0U, m_whiteDynamic.peakPwm, phaseMs, m_whiteDynamic.cycleMs);
        ApplyWhiteDutyRaw(ClampDutyRaw(rawMixed));
        break;
    }

    case WhiteEffectMode::None:
    default:
        ApplyWhiteDutyRaw(0U);
        break;
    }
}

/**
 * @brief 渲染红灯通道当前帧
 */
void IndicatorEffectEngine::RenderRedFrameRaw()
{
    switch (m_redMode)
    {
    case RedEffectMode::On:
        ApplyRedOnRaw(true);
        break;

    case RedEffectMode::Blink: {
        if (m_redDynamic.pAction == nullptr)
        {
            ApplyRedOnRaw(false);
            break;
        }

        const uint16_t phaseMs =
            static_cast<uint16_t>(m_redDynamic.elapsedMs % static_cast<uint32_t>(m_redDynamic.cycleMs));
        const uint32_t rawMixed = m_redDynamic.pAction(0U, m_redDynamic.peakPwm, phaseMs, m_redDynamic.cycleMs);
        ApplyRedOnRaw(ClampDutyRaw(rawMixed) > 0U);
        break;
    }

    case RedEffectMode::None:
    default:
        ApplyRedOnRaw(false);
        break;
    }
}

/**
 * @brief 渲染白/红两路当前帧
 */
void IndicatorEffectEngine::RenderAllFramesRaw()
{
    RenderWhiteFrameRaw();
    RenderRedFrameRaw();
}

/**
 * @brief 白灯常亮（Steady，不参与动态 tick）
 * @param brightness 逻辑亮度 0~255
 */
void IndicatorEffectEngine::StartWhiteOn(uint8_t brightness)
{
    const uint32_t peakPwm = BrightnessToPwmRaw(brightness);
    if ((m_whiteMode == WhiteEffectMode::On) && (m_whiteSteadyPwm == peakPwm))
    {
        return;
    }

    m_whiteMode      = WhiteEffectMode::On;
    m_whiteSteadyPwm = peakPwm;
    m_whiteDynamic   = DynamicChannelRuntime{};

    RenderWhiteFrameRaw();
    SyncRenderTimerRaw();
}

/**
 * @brief 红灯常亮（Steady，不参与动态 tick）
 */
void IndicatorEffectEngine::StartRedOn()
{
    if (m_redMode == RedEffectMode::On)
    {
        ApplyRedOnRaw(true);
        return;
    }

    m_redMode    = RedEffectMode::On;
    m_redDynamic = DynamicChannelRuntime{};

    RenderRedFrameRaw();
    SyncRenderTimerRaw();
}

/**
 * @brief 白光呼吸：CalcBreath80BytesFactor，周期默认 3200ms
 */
void IndicatorEffectEngine::StartWhiteBreath(uint8_t brightness, uint16_t cycleMs)
{
    const uint16_t normalizedCycleMs = NormalizeBlinkCycleMsRaw(cycleMs);
    StartWhiteDynamicRaw(WhiteEffectMode::Breath, LightEffectProcessor::CalcBreath80BytesFactor,
                         BrightnessToPwmRaw(brightness), normalizedCycleMs);
}

/**
 * @brief 白光闪烁：GetBlink 算子，PWM 调光
 */
void IndicatorEffectEngine::StartWhiteBlink(uint8_t brightness, uint16_t cycleMs)
{
    const uint16_t normalizedCycleMs = NormalizeBlinkCycleMsRaw(cycleMs);
    StartWhiteDynamicRaw(WhiteEffectMode::Blink, LightEffectProcessor::GetBlink, BrightnessToPwmRaw(brightness),
                         normalizedCycleMs);
}

/**
 * @brief 红光闪烁：GetBlink 算子，输出映射为开关
 */
void IndicatorEffectEngine::StartRedBlink(uint16_t cycleMs)
{
    const uint16_t normalizedCycleMs = NormalizeBlinkCycleMsRaw(cycleMs);
    StartRedDynamicRaw(RedEffectMode::Blink, LightEffectProcessor::GetBlink, kIndicatorPwmMax, normalizedCycleMs);
}

/**
 * @brief 停止白灯通道
 */
void IndicatorEffectEngine::StopWhite()
{
    if (m_whiteMode == WhiteEffectMode::None)
    {
        return;
    }

    m_whiteMode      = WhiteEffectMode::None;
    m_whiteDynamic   = DynamicChannelRuntime{};
    m_whiteSteadyPwm = 0U;

    ApplyWhiteDutyRaw(0U);
    SyncRenderTimerRaw();
}

/**
 * @brief 停止红灯通道
 */
void IndicatorEffectEngine::StopRed()
{
    if (m_redMode == RedEffectMode::None)
    {
        return;
    }

    m_redMode    = RedEffectMode::None;
    m_redDynamic = DynamicChannelRuntime{};

    ApplyRedOnRaw(false);
    SyncRenderTimerRaw();
}

/**
 * @brief 停止两路灯效并熄灭白/红指示灯
 */
void IndicatorEffectEngine::Stop()
{
    if ((m_whiteMode == WhiteEffectMode::None) && (m_redMode == RedEffectMode::None))
    {
        return;
    }

    m_whiteMode      = WhiteEffectMode::None;
    m_redMode        = RedEffectMode::None;
    m_whiteDynamic   = DynamicChannelRuntime{};
    m_redDynamic     = DynamicChannelRuntime{};
    m_whiteSteadyPwm = 0U;

    TimerStartStop(false);
    ApplyWhiteDutyRaw(0U);
    ApplyRedOnRaw(false);
}

/**
 * @brief 10ms tick 入口：累加动态通道时间并刷新两路帧
 * @param elapsedMs 距上次 tick 的毫秒增量（固定 10）
 */
void IndicatorEffectEngine::UpdateTicks(uint16_t elapsedMs)
{
    if (DeriveEngineStateRaw() != EngineState::Running)
    {
        return;
    }

    if ((m_whiteMode == WhiteEffectMode::Breath) || (m_whiteMode == WhiteEffectMode::Blink))
    {
        m_whiteDynamic.elapsedMs += elapsedMs;
    }

    if (m_redMode == RedEffectMode::Blink)
    {
        m_redDynamic.elapsedMs += elapsedMs;
    }

    RenderAllFramesRaw();
}
