/**
 * @file IndicatorEffectEngine.h
 * @brief 指示灯灯效引擎：白光呼吸 / 红光闪烁（算子来自 LightEffectProcessor）
 * @author hady
 * @date 2026-06-26
 * @layer Middlewares
 * @note 仿 LightEffectEngine 精简版，专用于 BspLedIndicatorWhite / Red。
 */
#pragma once

#include <cstdint>

/**
 * @class IndicatorEffectEngine
 * @brief 单通道指示灯渲染：10ms tick + 算子插值 + BSP 输出
 */
class IndicatorEffectEngine
{
public:
  enum class EngineState : uint8_t
  {
    Idle = 0,  /**< 常灭 */
    Steady,    /**< 常亮（无定时器） */
    Running    /**< 呼吸 / 闪烁等动态灯效 */
  };

  enum class EffectMode : uint8_t
  {
    None = 0,
    WhiteOn,
    RedOn,
    WhiteBreath,
    WhiteBlink,
    RedBlink
  };

  using EffectRenderAction = uint32_t (*)(uint32_t start, uint32_t end,
                                          uint16_t elapsedMs, uint16_t totalMs);

  static IndicatorEffectEngine& Instance()
  {
    static IndicatorEffectEngine instance;
    return instance;
  }

  void Init();

  /** @brief 白灯常亮 */
  void StartWhiteOn(uint8_t brightness = 255U);

  /** @brief 红灯常亮 */
  void StartRedOn();

  /** @brief 白光呼吸（默认 60% 亮度，3200ms 周期循环） */
  void StartWhiteBreath(uint8_t brightness = 153U, uint16_t cycleMs = kDefaultBreathCycleMs);

  /** @brief 白光闪烁（cycleMs 为完整亮灭周期，默认 800ms） */
  void StartWhiteBlink(uint8_t brightness = 255U,
                       uint16_t cycleMs = kDefaultBlinkCycleMs);

  /** @brief 红光闪烁（cycleMs 为完整亮灭周期，默认 800ms） */
  void StartRedBlink(uint16_t cycleMs = kDefaultBlinkCycleMs);

  /** @brief 停止灯效并熄灭白/红指示灯（常灭） */
  void Stop();

  EngineState GetEngineState() const;
  EffectMode GetEffectMode() const;

private:
  IndicatorEffectEngine()                              = default;
  ~IndicatorEffectEngine()                             = default;
  IndicatorEffectEngine(const IndicatorEffectEngine&)            = delete;
  IndicatorEffectEngine& operator=(const IndicatorEffectEngine&) = delete;

  void TimerStartStop(bool start);
  void RenderCurrentFrameRaw();
  void UpdateTicks(uint16_t elapsedMs);
  uint16_t ClampDutyRaw(uint32_t rawMixed) const;
  uint16_t NormalizeBlinkCycleMsRaw(uint16_t cycleMs) const;
  uint16_t BrightnessToPwmRaw(uint8_t brightness) const;
  void ApplyWhiteDutyRaw(uint16_t duty);
  void ApplyRedOnRaw(bool on);
  void StartDynamicEffectRaw(EffectMode mode, EffectRenderAction action,
                             uint32_t peakPwm, uint16_t cycleMs);

  EngineState m_state{EngineState::Idle};
  EffectMode m_mode{EffectMode::None};
  EffectRenderAction m_pAction{nullptr};
  uint32_t m_peakPwm{0U};
  uint32_t m_elapsedMs{0U};
  uint16_t m_cycleMs{0U};
  uint8_t m_operatorBits{12U};

  static constexpr uint16_t kDefaultBreathCycleMs = 3200U;
  static constexpr uint16_t kDefaultBlinkCycleMs  = 800U;
  static constexpr uint16_t kMinBlinkCycleMs        = 200U;
  static constexpr uint16_t kMaxBlinkCycleMs      = 10000U;
  static constexpr uint16_t kIndicatorPwmMax      = 1023U;
  static constexpr uint8_t kDefaultBreathBrightness = 153U;
};
