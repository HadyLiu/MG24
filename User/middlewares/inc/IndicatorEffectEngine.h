/**
 * @file IndicatorEffectEngine.h
 * @brief 指示灯灯效引擎：白光呼吸 / 红光闪烁（算子来自 LightEffectProcessor）
 * @author hady
 * @date 2026-06-26
 * @layer Middlewares
 * @note 仿 LightEffectEngine 精简版，专用于 BspLedIndicatorWhite / Red。
 *       纯技术中间件，不含业务逻辑；10ms tick 由 Init 内 BspTimer 注册；
 *       白/红两路独立渲染，白呼吸与故障红闪可叠加显示。
 */
#pragma once

#include <cstdint>

/**
 * @class IndicatorEffectEngine
 * @brief 双通道指示灯渲染：算子驱动、物理 PWM 插值、白/红独立输出
 * @note
 * 数据流：Start* 分别设定白/红通道 → UpdateTicks 周期插值 → >>operatorBits
 * 归一化 → Apply*Raw 写 BSP。白灯 PWM 调光，红灯仅开关；两路互不干扰。
 */
class IndicatorEffectEngine
{
  public:
    /** @brief 引擎运行状态（由白/红通道聚合） */
    enum class EngineState : uint8_t
    {
        Idle = 0, /**< 两路均熄灭，定时器挂起 */
        Steady,   /**< 至少一路常亮，无动态 tick */
        Running   /**< 至少一路呼吸/闪烁，10ms tick 周期渲染 */
    };

    /** @brief 白灯通道灯效模式 */
    enum class WhiteEffectMode : uint8_t
    {
        None = 0,
        On,     /**< 白灯常亮 */
        Breath, /**< 白光呼吸（CalcBreath80BytesFactor） */
        Blink   /**< 白光闪烁（GetBlink） */
    };

    /** @brief 红灯通道灯效模式 */
    enum class RedEffectMode : uint8_t
    {
        None = 0,
        On,   /**< 红灯常亮 */
        Blink /**< 红光闪烁（GetBlink，输出为开关量） */
    };

    /**
     * @brief 灯效算子函数指针（与 LightEffectEngine 同签名）
     * @param start     插值起点（物理 PWM，通常为 0）
     * @param end       插值终点（物理 PWM 峰值，0~kIndicatorPwmMax）
     * @param elapsedMs 当前周期内已运行时间 (ms)
     * @param totalMs   单周期总时长 (ms)
     * @return Q12 定点混合值，须 >>m_operatorBits 得到物理 PWM
     */
    using EffectRenderAction = uint32_t (*)(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /**
     * @brief 获取引擎单例
     * @return IndicatorEffectEngine 引用
     */
    static IndicatorEffectEngine& Instance()
    {
        static IndicatorEffectEngine instance;
        return instance;
    }

    /** @brief 初始化引擎状态并注册 10ms 渲染定时器 */
    void Init();

    /**
     * @brief 白灯常亮
     * @param brightness 逻辑亮度 0~255，默认 255
     */
    void StartWhiteOn(uint8_t brightness = 255U);

    /** @brief 红灯常亮 */
    void StartRedOn();

    /**
     * @brief 白光呼吸（默认 60% 亮度，3200ms 周期循环）
     * @param brightness 峰值亮度 0~255，默认 153（约 60%）
     * @param cycleMs    完整呼吸周期 (ms)，默认 3200，限幅 200~10000
     */
    void StartWhiteBreath(uint8_t brightness = 153U, uint16_t cycleMs = kDefaultBreathCycleMs);

    /**
     * @brief 白光闪烁
     * @param brightness 亮段峰值亮度 0~255，默认 255
     * @param cycleMs    完整亮灭周期 (ms)，默认 800，限幅 200~10000
     */
    void StartWhiteBlink(uint8_t brightness = 255U, uint16_t cycleMs = kDefaultBlinkCycleMs);

    /**
     * @brief 红光闪烁
     * @param cycleMs 完整亮灭周期 (ms)，默认 800，限幅 200~10000
     */
    void StartRedBlink(uint16_t cycleMs = kDefaultBlinkCycleMs);

    /** @brief 停止白灯通道并熄灭白灯 */
    void StopWhite();

    /** @brief 停止红灯通道并熄灭红灯 */
    void StopRed();

    /** @brief 停止两路灯效并熄灭白/红指示灯 */
    void Stop();

    /** @brief 查询引擎聚合运行状态 */
    EngineState GetEngineState() const;

    /** @brief 查询白灯通道模式 */
    WhiteEffectMode GetWhiteEffectMode() const;

    /** @brief 查询红灯通道模式 */
    RedEffectMode GetRedEffectMode() const;

  private:
    IndicatorEffectEngine()                                        = default;
    ~IndicatorEffectEngine()                                       = default;
    IndicatorEffectEngine(const IndicatorEffectEngine&)            = delete;
    IndicatorEffectEngine& operator=(const IndicatorEffectEngine&) = delete;

    /** @brief 动态通道运行时参数 */
    struct DynamicChannelRuntime
    {
        EffectRenderAction pAction{nullptr};
        uint32_t           peakPwm{0U};
        uint32_t           elapsedMs{0U};
        uint16_t           cycleMs{0U};
    };

    /** @brief 启停 10ms 渲染定时器（幂等） */
    void TimerStartStop(bool start);
    /** @brief 按白/红通道当前模式渲染一帧并写 BSP */
    void RenderAllFramesRaw();
    /** @brief 渲染白灯通道一帧 */
    void RenderWhiteFrameRaw();
    /** @brief 渲染红灯通道一帧 */
    void RenderRedFrameRaw();
    /** @brief 定时器 tick：累加动态通道 elapsedMs 并刷新帧 */
    void UpdateTicks(uint16_t elapsedMs);
    /** @brief 根据两路动态状态同步定时器启停 */
    void SyncRenderTimerRaw();
    /** @brief 聚合两路状态得到 EngineState */
    EngineState DeriveEngineStateRaw() const;
    /** @brief 算子 Q12 输出 >>bits 后限幅至 kIndicatorPwmMax */
    uint16_t ClampDutyRaw(uint32_t rawMixed) const;
    /** @brief 闪烁/呼吸周期限幅至 [kMinBlinkCycleMs, kMaxBlinkCycleMs] */
    uint16_t NormalizeBlinkCycleMsRaw(uint16_t cycleMs) const;
    /** @brief 逻辑亮度 0~255 → 物理 PWM 0~kIndicatorPwmMax */
    uint16_t BrightnessToPwmRaw(uint8_t brightness) const;
    /** @brief 仅写白灯 PWM，不影响红灯 */
    void ApplyWhiteDutyRaw(uint16_t duty);
    /** @brief 仅写红灯开关，不影响白灯 */
    void ApplyRedOnRaw(bool on);
    /**
     * @brief 启动白灯动态灯效
     * @note 参数与当前动态白通道完全一致时跳过，避免重复复位相位
     */
    void StartWhiteDynamicRaw(WhiteEffectMode mode, EffectRenderAction action, uint32_t peakPwm, uint16_t cycleMs);
    /**
     * @brief 启动红灯动态灯效
     * @note 参数与当前动态红通道完全一致时跳过，避免重复复位相位
     */
    void StartRedDynamicRaw(RedEffectMode mode, EffectRenderAction action, uint32_t peakPwm, uint16_t cycleMs);

    WhiteEffectMode       m_whiteMode{WhiteEffectMode::None};
    RedEffectMode         m_redMode{RedEffectMode::None};
    DynamicChannelRuntime m_whiteDynamic{};
    DynamicChannelRuntime m_redDynamic{};
    uint32_t              m_whiteSteadyPwm{0U}; /**< 白灯常亮峰值 PWM */
    uint8_t               m_operatorBits{12U};  /**< 算子 Q12 右移位宽 */

    static constexpr uint16_t kDefaultBreathCycleMs    = 3200U;
    static constexpr uint16_t kDefaultBlinkCycleMs     = 800U;
    static constexpr uint16_t kMinBlinkCycleMs         = 200U;
    static constexpr uint16_t kMaxBlinkCycleMs         = 10000U;
    static constexpr uint16_t kIndicatorPwmMax         = 1023U;
    static constexpr uint8_t  kDefaultBreathBrightness = 153U;
};
