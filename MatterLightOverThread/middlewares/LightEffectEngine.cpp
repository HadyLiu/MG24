/**
 * @file LightEffectEngine.cpp
 * @brief 多通道灯效时序渲染引擎实现
 * @author hady
 * @date 2026-06-19
 * @layer Middlewares
 * @note 物理 PWM 插值模型：
 *       物理占空比 = 逻辑 RGBW × 亮度 / 255；
 *       算子输入/输出均为物理 PWM 域，blendingOperator 返回值须 >>12。
 */
#include "LightEffectEngine.h"
#include <cstring>

/**
 * @brief 初始化引擎
 * @param callback           多通道 PWM 输出回调
 * @param channelCount       激活通道数
 * @param outputPwmMaxBits   硬件 PWM 位宽
 * @param operatorPwmMaxBits 算子 Q 格式位宽（通常为 12）
 * @return 无
 */
void LightEffectEngine::Init(BspMultiChannelCallback callback,
                             uint8_t channelCount, uint8_t outputPwmMaxBits,
                             uint8_t operatorPwmMaxBits)
{
  if (channelCount > kMaxChannelsSupported)
  {
    m_activeChannels = kMaxChannelsSupported;
  }
  else
  {
    m_activeChannels = channelCount;
  }

  m_state            = EngineState::Idle;
  m_pCurrentAction   = nullptr;
  m_globalBrightness = 0U;

  m_operatorMaxPwmBits = operatorPwmMaxBits;
  m_outputMaxPwmBits   = outputPwmMaxBits;

  m_totalClockMs        = 0U;
  m_singleEffectRunTime = 0U;
  m_totalDurationMs     = 0U;
  m_pBspCallback        = callback;
  m_pTimerCtrlCallback  = nullptr;
  m_mixedTimingCallback = nullptr;

  memset(m_originTargetColor, 0, sizeof(m_originTargetColor));
  memset(m_startOutColor, 0, sizeof(m_startOutColor));
  memset(m_targetOutColor, 0, sizeof(m_targetOutColor));
  memset(m_currentOutColor, 0, sizeof(m_currentOutColor));
  memset(m_currentOutPhysicalValueColor, 0,
         sizeof(m_currentOutPhysicalValueColor));
}

/**
 * @brief 查询引擎状态
 * @return 当前 EngineState
 */
LightEffectEngine::EngineState LightEffectEngine::GetEngineState() const
{
  return m_state;
}

bool LightEffectEngine::IsAnyChannelActive() const
{
  for (uint8_t i = 0U; i < m_activeChannels; ++i)
  {
    if (m_currentOutPhysicalValueColor[i] > 0U)
    {
      return true;
    }
  }
  return false;
}
/**
 * @brief 查询本次特效目标全局亮度
 * @return 亮度 0~255
 */
uint8_t LightEffectEngine::GetGlobalBrightness() const
{
  return m_globalBrightness;
}

/**
 * @brief 查询激活通道数
 * @return 通道数
 */
uint8_t LightEffectEngine::GetActiveChannels() const
{
  return m_activeChannels;
}

/**
 * @brief 回读逻辑目标色（未乘亮度、未截断）
 * @param outChannels 输出缓冲区
 * @param count       缓冲区通道数
 * @return 无
 */
void LightEffectEngine::GetTargetColor(uint16_t* outChannels,
                                       uint8_t count) const
{
  if (outChannels == nullptr)
  {
    return;
  }

  uint8_t copyCount = (count > m_activeChannels) ? m_activeChannels : count;
  memcpy(outChannels, m_originTargetColor,
         (static_cast<size_t>(copyCount) * sizeof(uint16_t)));
}

/**
 * @brief 注册定时器控制回调
 * @param callback entry 注入的 BspTimer 启停函数
 * @return 无
 */
void LightEffectEngine::RegisterTimerControlCallback(
    TimerControlCallback callback)
{
  m_pTimerCtrlCallback = callback;
}

/**
 * @brief 注册链式时序结束回调
 * @param callback LightSequenceScheduler 桥接函数
 * @return 无
 */
void LightEffectEngine::RegisterMixedTimingCallback(
    MixedTimingCallback callback)
{
  m_mixedTimingCallback = callback;
}

/**
 * @brief 逻辑通道值限幅至算子输入范围
 * @param value 输入通道值
 * @return 限幅后的值
 */
uint16_t LightEffectEngine::ClampChannel(uint16_t value) const
{
  const uint16_t maxOperatorValue = static_cast<uint16_t>(
      (static_cast<uint32_t>(1U) << m_operatorMaxPwmBits) - 1U);

  return (value > maxOperatorValue) ? maxOperatorValue : value;
}

/**
 * @brief 物理 PWM 限幅至硬件满量程
 * @param value 物理占空比
 * @return 限幅后的值
 */
uint16_t LightEffectEngine::ClampOutputChannel(uint16_t value) const
{
  const uint16_t maxOutputValue = static_cast<uint16_t>(
      (static_cast<uint32_t>(1U) << m_outputMaxPwmBits) - 1U);

  return (value > maxOutputValue) ? maxOutputValue : value;
}

/**
 * @brief 逻辑 RGBW × 亮度 → 物理 PWM
 * @param channel    逻辑通道值（100% RGBW）
 * @param brightness 全局亮度 0~255
 * @return 物理占空比；brightness=0 时返回 0
 */
uint32_t LightEffectEngine::CalcPhysicalPwmRaw(uint32_t channel,
                                               uint8_t brightness) const
{
  if (brightness == 0U)
  {
    return 0U;
  }

  return (channel * brightness + (kMaxBrightness / 2U)) / kMaxBrightness;
}

/**
 * @brief 渲染当前帧并回调 BSP
 * @return 无
 * @note 起/终点已在 StartEffect 预计算为物理 PWM，本函数仅做算子插值与输出。
 */
void LightEffectEngine::RenderCurrentEffectFrame()
{
  for (uint8_t i = 0U; i < m_activeChannels; ++i)
  {
    const uint32_t rawMixed = m_pCurrentAction(
        m_startOutColor[i], m_targetOutColor[i],
        static_cast<uint16_t>(m_singleEffectRunTime), m_totalDurationMs);

    m_currentOutColor[i]              = rawMixed;
    m_currentOutPhysicalValueColor[i] = ClampOutputChannel(
        static_cast<uint16_t>(rawMixed >> m_operatorMaxPwmBits));
  }

  if (m_pBspCallback != nullptr)
  {
    m_pBspCallback(m_currentOutPhysicalValueColor);
  }
}

/**
 * @brief 停止当前特效
 * @param clearHardwareOutput true 时清零物理输出并刷新 BSP
 * @return 无
 */
void LightEffectEngine::StopCurrentEffect(bool clearHardwareOutput)
{
  m_state          = EngineState::Idle;
  m_pCurrentAction = nullptr;

  if (m_pTimerCtrlCallback != nullptr)
  {
    m_pTimerCtrlCallback(false);
  }

  if (clearHardwareOutput)
  {
    memset(m_currentOutPhysicalValueColor, 0,
           sizeof(m_currentOutPhysicalValueColor));
    if (m_pBspCallback != nullptr)
    {
      m_pBspCallback(m_currentOutPhysicalValueColor);
    }
  }
}

/**
 * @brief 启动单次灯效
 * @param pAction        算子指针
 * @param targetChannels 逻辑目标 RGBW
 * @param brightness     目标全局亮度 0~255
 * @param durationMs     过渡时长 (ms)
 * @return 无
 * @note 起点 = 当前物理输出；终点 = RGBW×brightness/255；
 *       预计算后渲染循环不再做亮度乘法，保证切换连续。
 */
void LightEffectEngine::StartEffect(EffectRenderAction pAction,
                                    const uint16_t* targetChannels,
                                    uint8_t brightness, uint16_t durationMs)
{
  if ((pAction == nullptr) || (targetChannels == nullptr))
  {
    return;
  }

  m_state               = EngineState::Idle;
  m_pCurrentAction      = pAction;
  m_totalDurationMs     = durationMs;
  m_globalBrightness    = brightness;
  m_singleEffectRunTime = 0U;
  m_state               = EngineState::Running;

  for (uint8_t i = 0U; i < m_activeChannels; ++i)
  {
    m_originTargetColor[i] = ClampChannel(targetChannels[i]);
    m_startOutColor[i]     = m_currentOutPhysicalValueColor[i];
    m_targetOutColor[i] =
        CalcPhysicalPwmRaw(m_originTargetColor[i], m_globalBrightness);
  }

  if (m_pTimerCtrlCallback != nullptr)
  {
    m_pTimerCtrlCallback(true);
  }
}

/**
 * @brief 周期 tick 驱动渲染
 * @param elapsedMs 距上次 tick 的毫秒增量
 * @return 无
 * @note 超时后先渲染末帧再停定时器，并回调 MixedTimingCallback 推进时序链。
 */
void LightEffectEngine::UpdateTicks(uint32_t elapsedMs)
{
  if ((m_state == EngineState::Idle) || (m_pCurrentAction == nullptr))
  {
    return;
  }

  m_totalClockMs += elapsedMs;
  m_singleEffectRunTime += elapsedMs;

  const bool timedOut =
      (m_totalDurationMs > 0U) && (m_singleEffectRunTime >= m_totalDurationMs);
  if (timedOut)
  {
    m_singleEffectRunTime = m_totalDurationMs;
  }

  RenderCurrentEffectFrame();

  if (timedOut)
  {
    m_state          = EngineState::Idle;
    m_pCurrentAction = nullptr;

    if (m_pTimerCtrlCallback != nullptr)
    {
      m_pTimerCtrlCallback(false);
    }
    if (m_mixedTimingCallback != nullptr)
    {
      m_mixedTimingCallback();
    }
  }
}
