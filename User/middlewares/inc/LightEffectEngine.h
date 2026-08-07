/**
 * @file LightEffectEngine.h
 * @brief 多通道灯效时序渲染引擎（物理 PWM 插值量产版）
 * @author hady
 * @date 2026-06-19
 * @layer Middlewares
 * @note 纯技术中间件，不含业务逻辑。10ms tick 与 BSP 输出均由 entry
 * 注册回调注入；算子来自 LightEffectProcessor；链式时序由
 * LightSequenceScheduler 经 MixedTimingCallback 倒灌。
 */
#pragma once

#include <cstdint>

/**
 * @class LightEffectEngine
 * @brief 多通道灯效渲染核心：算子驱动、物理 PWM 插值、低功耗时钟自管理
 * @note
 * 数据流：StartEffect 预计算起/终点物理占空比 → UpdateTicks 调用算子插值
 * → >>12 归一化 → BSP 回调输出。逻辑 RGBW 保存在 m_originTargetColor，
 * 与物理输出隔离，供上层回读。
 */
class LightEffectEngine
{
  public:
    /** @brief 引擎运行状态 */
    enum class EngineState : uint8_t
    {
        Idle = 0, ///< 空闲：无特效，定时器挂起，保持上一帧物理输出
        Running   ///< 运行：定时器拉起，UpdateTicks 周期渲染
    };

    /**
     * @brief 灯效算子函数指针
     * @param start     插值起点（物理 PWM，0~outputMax）
     * @param end       插值终点（物理 PWM，0~outputMax）
     * @param elapsedMs 当前特效已运行时间 (ms)
     * @param totalMs   特效总时长 (ms)
     * @return Q12 定点混合值，须 >>operatorMaxPwmBits 得到物理 PWM
     */
    using EffectRenderAction = uint32_t (*)(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /** @brief 单步特效结束回调，供 LightSequenceScheduler 链式推进 */
    using MixedTimingCallback = void (*)();

    /** @brief 物理输出活跃状态变化回调（供 PowerServer 评估电池供电） */
    using OutputActivityCallback = void (*)(bool isActive);

    /**
     * @brief 获取引擎单例
     * @return LightEffectEngine 引用
     */
    static LightEffectEngine& Instance()
    {
        static LightEffectEngine instance;
        return instance;
    }

    /* 初始化引擎 */
    void Init();

    /* 查询引擎状态 */
    EngineState GetEngineState() const;

    /* 检查当前是否有任意 WRGB 通道正在物理输出 */
    bool IsAnyChannelActive() const;
    /**
     * @brief 查询本次特效目标全局亮度
     * @return 亮度 0~255
     */
    uint8_t GetGlobalBrightness() const;

    /**
     * @brief 查询硬件 PWM 满量程值
     * @return (1 << outputPwmMaxBits) - 1
     */
    uint16_t GetOutputMaxLimit() const;

    /**
     * @brief 查询激活通道数
     * @return 通道数
     */
    uint8_t GetActiveChannels() const;

    /**
     * @brief 回读逻辑目标色（未乘亮度、未截断）
     * @param outChannels 输出缓冲区
     * @param count       缓冲区通道数
     * @return 无
     */
    void GetTargetColor(uint16_t* outChannels, uint8_t count) const;

    /**
     * @brief 同步外部输入快照（Matter/上层直控预留）
     * @param channels   逻辑 RGBW 数组
     * @param count      通道数
     * @param brightness 全局亮度 0~255
     * @return 无
     */
    void UpdateInputSnapshot(const uint16_t* channels, uint8_t count, uint8_t brightness);

    /** 停止当前特效 */
    void StopCurrentEffect(bool clearHardwareOutput);

    /* 注册链式时序结束回调 */
    void RegisterMixedTimingCallback(MixedTimingCallback callback);

    /** @brief 注册物理输出活跃状态变化回调 */
    void RegisterOutputActivityCallback(OutputActivityCallback callback);

    /** @brief 电池通路就绪后重推当前物理 PWM（USB→电池切换后调用） */
    void RefreshHardwareOutput();

    /**
     * @brief 启动单次灯效
     * @param pAction        算子指针（如 GetBezier40BytesFactorFadeIn）
     * @param targetChannels 逻辑目标 RGBW（100% 色值，0~outputMax）
     * @param brightness     目标全局亮度 0~255
     * @param durationMs     过渡时长 (ms)
     * @return 无
     * @note 起点快照为当前物理输出，终点 = RGBW×§17占空比缩放；
     *       保证亮度切换时曲线连续无跳变。
     */
    void StartEffect(EffectRenderAction pAction, const uint16_t* targetChannels, uint8_t brightness,
                     uint16_t durationMs);

    /**
     * @brief 周期 tick 驱动渲染
     * @param elapsedMs 距上次 tick 的毫秒增量（通常 10）
     * @return 无
     * @note 由 entry 注册的 BspTimer 回调调用；超时后渲染末帧并触发
     *       MixedTimingCallback。
     */
    void UpdateTicks(uint32_t elapsedMs);

  private:
    LightEffectEngine()                                    = default;
    ~LightEffectEngine()                                   = default;
    LightEffectEngine(const LightEffectEngine&)            = delete;
    LightEffectEngine& operator=(const LightEffectEngine&) = delete;

    /// 内部工具函数
    void    TimerStartStop(bool start);
    void    LightOutput(uint16_t* channelDuties);
    uint8_t LedGetNumberOfChannels();
    uint8_t LedGetMaxPwmBits();
    uint8_t LedGetOperatorMaxPwmBits();

    /** @brief 逻辑通道值限幅至算子输入范围 */
    uint16_t ClampChannel(uint16_t value) const;
    /** @brief 物理 PWM 限幅至硬件满量程 */
    uint16_t ClampOutputChannel(uint16_t value) const;
    /** @brief 逻辑 RGBW × 亮度 → 物理 PWM（四舍五入） */
    uint32_t CalcPhysicalPwmRaw(uint32_t channel, uint8_t brightness) const;
    /** @brief 渲染当前帧并回调 BSP */
    void RenderCurrentEffectFrame();
    /** @brief 内部逻辑目标色写入（预留） */
    void SetTargetColorRaw(const uint16_t* channels, uint8_t count);

    static constexpr uint8_t  kMaxChannelsSupported = 5U;
    static constexpr uint8_t  kMaxBrightnessBits    = 8U;
    static constexpr uint16_t kMaxBrightness        = ((1U << kMaxBrightnessBits) - 1);

    EngineState m_state;
    uint8_t     m_globalBrightness;
    uint8_t     m_outputMaxPwmBits;
    uint8_t     m_operatorMaxPwmBits;

    uint16_t m_originTargetColor[kMaxChannelsSupported]; /**< 逻辑 RGBW（100%） */

    MixedTimingCallback    m_mixedTimingCallback;
    OutputActivityCallback m_outputActivityCallback{nullptr};
    bool                   m_lastOutputActive{false};
    EffectRenderAction     m_pCurrentAction;

    uint8_t  m_activeChannels;
    uint32_t m_totalClockMs;
    uint32_t m_singleEffectRunTime;
    uint16_t m_totalDurationMs;

    uint32_t m_startOutColor[kMaxChannelsSupported];                /**< 插值起点：物理 PWM */
    uint32_t m_targetOutColor[kMaxChannelsSupported];               /**< 插值终点：物理 PWM */
    uint32_t m_currentOutColor[kMaxChannelsSupported];              /**< 算子 Q12 原始输出 */
    uint16_t m_currentOutPhysicalValueColor[kMaxChannelsSupported]; /**< 当前物理输出
                                                                     */
};
