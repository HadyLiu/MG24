/**
 * @file IndicatorEffectEngine.cpp
 * @brief 指示灯灯效引擎实现
 */
#include "IndicatorEffectEngine.h"
#include "BspLedIndicatorRed.h"
#include "BspLedIndicatorWhite.h"
#include "BspTimer.h"
#include "LightEffectProcessor.h"

static BspTimer s_indicatorRenderTimer;

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

void IndicatorEffectEngine::Init()
{
  m_state        = EngineState::Idle;
  m_mode         = EffectMode::None;
  m_pAction      = nullptr;
  m_peakPwm      = 0U;
  m_elapsedMs    = 0U;
  m_cycleMs      = 0U;
  m_operatorBits = LightEffectProcessor::GetMaxFactorBits();

  s_indicatorRenderTimer.Init(
      [](uint16_t elapsedMs) {
        IndicatorEffectEngine::Instance().UpdateTicks(elapsedMs);
      },
      10U, nullptr);
}

IndicatorEffectEngine::EngineState IndicatorEffectEngine::GetEngineState() const
{
  return m_state;
}

IndicatorEffectEngine::EffectMode IndicatorEffectEngine::GetEffectMode() const
{
  return m_mode;
}

uint16_t IndicatorEffectEngine::ClampDutyRaw(uint32_t rawMixed) const
{
  const uint32_t duty = rawMixed >> m_operatorBits;
  if (duty > kIndicatorPwmMax)
  {
    return kIndicatorPwmMax;
  }
  return static_cast<uint16_t>(duty);
}

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

uint16_t IndicatorEffectEngine::BrightnessToPwmRaw(uint8_t brightness) const
{
  return static_cast<uint16_t>(
      (static_cast<uint32_t>(kIndicatorPwmMax) * brightness) / 255U);
}

void IndicatorEffectEngine::ApplyWhiteDutyRaw(uint16_t duty)
{
  BspLedIndicatorRed::Instance().SetRedIndicator(false);
  BspLedIndicatorWhite::Instance().IndicatorWhiteSetDuty(duty);
}

void IndicatorEffectEngine::ApplyRedOnRaw(bool on)
{
  BspLedIndicatorWhite::Instance().IndicatorWhiteSetDuty(0U);
  BspLedIndicatorRed::Instance().SetRedIndicator(on);
}

void IndicatorEffectEngine::StartDynamicEffectRaw(EffectMode mode,
                                                    EffectRenderAction action,
                                                    uint32_t peakPwm,
                                                    uint16_t cycleMs)
{
  if ((m_state == EngineState::Running) && (m_mode == mode) &&
      (m_pAction == action) && (m_peakPwm == peakPwm) && (m_cycleMs == cycleMs))
  {
    return;
  }

  m_mode      = mode;
  m_pAction   = action;
  m_peakPwm   = peakPwm;
  m_cycleMs   = cycleMs;
  m_elapsedMs = 0U;
  m_state     = EngineState::Running;

  RenderCurrentFrameRaw();
  TimerStartStop(true);
}

void IndicatorEffectEngine::RenderCurrentFrameRaw()
{
  if (m_pAction == nullptr)
  {
    return;
  }

  const uint16_t phaseMs =
      static_cast<uint16_t>(m_elapsedMs % static_cast<uint32_t>(m_cycleMs));
  const uint32_t rawMixed = m_pAction(0U, m_peakPwm, phaseMs, m_cycleMs);
  const uint16_t duty     = ClampDutyRaw(rawMixed);

  if ((m_mode == EffectMode::WhiteBreath) || (m_mode == EffectMode::WhiteBlink))
  {
    ApplyWhiteDutyRaw(duty);
  }
  else if (m_mode == EffectMode::RedBlink)
  {
    ApplyRedOnRaw(duty > 0U);
  }
}

void IndicatorEffectEngine::StartWhiteOn(uint8_t brightness)
{
  const uint32_t peakPwm = BrightnessToPwmRaw(brightness);
  if ((m_state == EngineState::Steady) && (m_mode == EffectMode::WhiteOn) &&
      (m_peakPwm == peakPwm))
  {
    return;
  }

  TimerStartStop(false);
  m_pAction = nullptr;
  m_mode    = EffectMode::WhiteOn;
  m_state   = EngineState::Steady;
  m_peakPwm = peakPwm;

  ApplyWhiteDutyRaw(static_cast<uint16_t>(peakPwm));
}

void IndicatorEffectEngine::StartRedOn()
{
  if ((m_state == EngineState::Steady) && (m_mode == EffectMode::RedOn))
  {
    ApplyRedOnRaw(true);
    return;
  }

  TimerStartStop(false);
  m_pAction = nullptr;
  m_mode    = EffectMode::RedOn;
  m_state   = EngineState::Steady;

  ApplyRedOnRaw(true);
}

void IndicatorEffectEngine::StartWhiteBreath(uint8_t brightness, uint16_t cycleMs)
{
  const uint16_t normalizedCycleMs = NormalizeBlinkCycleMsRaw(cycleMs);
  StartDynamicEffectRaw(EffectMode::WhiteBreath,
                        LightEffectProcessor::CalcBreath80BytesFactor,
                        BrightnessToPwmRaw(brightness), normalizedCycleMs);
}

void IndicatorEffectEngine::StartWhiteBlink(uint8_t brightness, uint16_t cycleMs)
{
  const uint16_t normalizedCycleMs = NormalizeBlinkCycleMsRaw(cycleMs);
  StartDynamicEffectRaw(EffectMode::WhiteBlink, LightEffectProcessor::GetBlink,
                        BrightnessToPwmRaw(brightness), normalizedCycleMs);
}

void IndicatorEffectEngine::StartRedBlink(uint16_t cycleMs)
{
  const uint16_t normalizedCycleMs = NormalizeBlinkCycleMsRaw(cycleMs);
  StartDynamicEffectRaw(EffectMode::RedBlink, LightEffectProcessor::GetBlink,
                        kIndicatorPwmMax, normalizedCycleMs);
}

void IndicatorEffectEngine::Stop()
{
  if ((m_state == EngineState::Idle) && (m_mode == EffectMode::None))
  {
    return;
  }

  m_state   = EngineState::Idle;
  m_mode    = EffectMode::None;
  m_pAction = nullptr;

  TimerStartStop(false);
  ApplyWhiteDutyRaw(0U);
  ApplyRedOnRaw(false);
}

void IndicatorEffectEngine::UpdateTicks(uint16_t elapsedMs)
{
  if ((m_state != EngineState::Running) || (m_pAction == nullptr))
  {
    return;
  }

  m_elapsedMs += elapsedMs;
  RenderCurrentFrameRaw();
}
