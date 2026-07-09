/**
 * @file IndicatorEffectEngine.h
 * @brief 指示灯灯效引擎：双通道渲染 + 可配置红闪混合时序
 * @author hady
 * @date 2026-06-26
 * @layer Middlewares
 * @note 仿 LightEffectEngine 精简版，专用于 BspLedIndicatorWhite / Red。
 *       纯渲染中间件，不含业务语义；10ms tick 内聚于 Init；
 *       上层（如 IndicatorServer）注入算子、周期与步序。
 */
#pragma once

#include <cstdint>

/**
 * @class IndicatorEffectEngine
 * @brief 双通道指示灯渲染：单步动态灯效 + 红闪混合时序链
 * @note
 * 数据流：Start* / StartRedBlinkSequence → UpdateTicks 周期插值
 * → >>m_operatorBits 归一化 → Apply*Raw 写 BSP。
 * 白灯 PWM 调光，红灯 duty>0 映射开关；两路独立、可叠加。
 */
class IndicatorEffectEngine
{
  public:
    /** @brief 红闪混合时序最大步数 */
    static constexpr uint8_t kMaxRedBlinkSequenceSteps = 8U;

    /** @brief 引擎运行状态（由白/红通道聚合） */
    enum class EngineState : uint8_t
    {
        Idle = 0, /**< 两路均熄灭，定时器挂起 */
        Steady,   /**< 至少一路常亮，无动态 tick */
        Running   /**< 至少一路呼吸/闪烁/时序链，10ms tick 渲染 */
    };

    /** @brief 白灯通道灯效模式 */
    enum class WhiteEffectMode : uint8_t
    {
        None = 0,
        On,     /**< 白灯常亮 */
        Breath, /**< 白光呼吸（单步循环） */
        Blink   /**< 白光闪烁（单步循环） */
    };

    /** @brief 红灯通道灯效模式 */
    enum class RedEffectMode : uint8_t
    {
        None = 0,
        On,           /**< 红灯常亮 */
        Blink,        /**< 红光单步闪烁（周期循环） */
        BlinkSequence /**< 红闪混合时序（多步链，步序由上层注入） */
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

    /** @brief 红闪混合时序单步描述
     * @note repeatCount 与 LightSequenceScheduler 一致：0=执行 1 次，N=额外重复 N 次（共 N+1 次）
     */
    struct BlinkSequenceStep
    {
        EffectRenderAction pAction{nullptr}; /**< 算子；nullptr 时默认 GetBlink */
        uint32_t           peakPwm{0U};      /**< 峰值 PWM；0 时使用 kIndicatorPwmMax */
        uint16_t           cycleMs{0U};      /**< 单步亮灭周期 (ms) */
        uint8_t            repeatCount{0U};  /**< 本步额外重复次数 */
    };

    /** @brief 灯效自然播完回调（loopForever=false 时序链收尾时触发，Stop* 不触发） */
    using EffectFinishedCallback = void (*)();

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
     * @brief 红光单步闪烁（固定周期循环，直至 StopRed）
     * @param cycleMs 完整亮灭周期 (ms)，默认 800，限幅 200~10000
     */
    void StartRedBlink(uint16_t cycleMs = kDefaultBlinkCycleMs);

    /**
     * @brief 启动红闪混合时序链
     * @param steps       步序数组（周期 / 重复次数 / 算子由上层定义）
     * @param count       步数，上限 kMaxRedBlinkSequenceSteps
     * @param loopForever true=整链循环；false=播完一步链后熄灭红灯
     * @note 步序深拷贝至内部静态池，调用方可释放临时数组
     */
    void StartRedBlinkSequence(const BlinkSequenceStep* steps, uint8_t count, bool loopForever = true);

    /**
     * @brief 注册红闪混合时序自然播完回调
     * @param callback 由上层（如 IndicatorServer / entry）注入；可为 nullptr
     */
    void RegisterRedBlinkSequenceFinishedCallback(EffectFinishedCallback callback);

    /** @brief 停止白灯通道并熄灭白灯 */
    void StopWhite();

    /** @brief 停止红灯通道（含混合时序）并熄灭红灯 */
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

    /** @brief 动态通道运行时参数（单步循环或时序链当前步共用） */
    struct DynamicChannelRuntime
    {
        EffectRenderAction pAction{nullptr};
        uint32_t           peakPwm{0U};
        uint32_t           elapsedMs{0U}; /**< 当前步已运行时间 (ms) */
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
    /** @brief 定时器 tick：累加 elapsedMs，驱动时序步进并刷新帧 */
    void UpdateTicks(uint16_t elapsedMs);
    /** @brief 根据两路动态状态同步定时器启停 */
    void SyncRenderTimerRaw();
    /** @brief 聚合两路状态得到 EngineState */
    EngineState DeriveEngineStateRaw() const;
    /** @brief 算子 Q12 输出 >>bits 后限幅至 kIndicatorPwmMax */
    uint16_t ClampDutyRaw(uint32_t rawMixed) const;
    /** @brief 周期限幅至 [kMinBlinkCycleMs, kMaxBlinkCycleMs] */
    uint16_t NormalizeBlinkCycleMsRaw(uint16_t cycleMs) const;
    /** @brief 逻辑亮度 0~255 → 物理 PWM 0~kIndicatorPwmMax */
    uint16_t BrightnessToPwmRaw(uint8_t brightness) const;
    /** @brief 仅写白灯 PWM，不影响红灯 */
    void ApplyWhiteDutyRaw(uint16_t duty);
    /** @brief 仅写红灯开关，不影响白灯 */
    void ApplyRedOnRaw(bool on);
    /**
     * @brief 启动白灯动态灯效（Breath / Blink）
     * @note 参数与当前动态白通道完全一致时跳过，避免重复复位相位
     */
    void StartWhiteDynamicRaw(WhiteEffectMode mode, EffectRenderAction action, uint32_t peakPwm, uint16_t cycleMs);
    /**
     * @brief 启动红灯动态灯效（Blink；BlinkSequence 由专用路径启动）
     * @note 切换至非 BlinkSequence 模式时清时序状态
     */
    void StartRedDynamicRaw(RedEffectMode mode, EffectRenderAction action, uint32_t peakPwm, uint16_t cycleMs);
    /** @brief 加载步序并进入指定步，复位相位 */
    void BeginRedBlinkSequenceStepRaw(uint8_t stepIndex);
    /** @brief 当前步周期完成：重复本步或切至下一步 / 整链收尾 */
    void AdvanceRedBlinkSequenceStepRaw();
    /** @brief 非循环时序播完：熄灭红灯并清时序状态 */
    void FinishRedBlinkSequenceRaw();
    /** @brief 触发已注册的自然播完回调 */
    void InvokeRedBlinkSequenceFinishedRaw();

    WhiteEffectMode       m_whiteMode{WhiteEffectMode::None};
    RedEffectMode         m_redMode{RedEffectMode::None};
    DynamicChannelRuntime m_whiteDynamic{};
    DynamicChannelRuntime m_redDynamic{};
    uint32_t              m_whiteSteadyPwm{0U};   /**< 白灯常亮峰值 PWM */
    uint8_t               m_redSeqStepIndex{0U};  /**< 时序链当前步索引 */
    uint8_t               m_redSeqRepeatLeft{0U}; /**< 当前步剩余额外重复次数 */
    uint8_t               m_redSeqTotalSteps{0U}; /**< 已加载步数 */
    bool                  m_redSeqLoopForever{true};
    BlinkSequenceStep     m_redSeqSteps[kMaxRedBlinkSequenceSteps]{}; /**< 步序静态池 */
    EffectFinishedCallback m_redSeqFinishedCallback{nullptr};
    uint8_t               m_operatorBits{12U}; /**< 算子 Q12 右移位宽，来自 LightEffectProcessor */

    static constexpr uint16_t kDefaultBreathCycleMs    = 3200U;
    static constexpr uint16_t kDefaultBlinkCycleMs     = 800U;
    static constexpr uint16_t kMinBlinkCycleMs         = 200U;
    static constexpr uint16_t kMaxBlinkCycleMs         = 10000U;
    static constexpr uint16_t kIndicatorPwmMax         = 1023U;
    static constexpr uint8_t  kDefaultBreathBrightness = 153U;
};
