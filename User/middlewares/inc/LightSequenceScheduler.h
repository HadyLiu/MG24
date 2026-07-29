/**
 * @file LightSequenceScheduler.h
 * @brief 全权接管型多阶段灯效时序调度器（支持无限循环量产版）
 * @layer Middlewares
 */

#pragma once

#include "LightEffectEngine.h"
#include <stdint.h>

class LightSequenceScheduler
{
  public:
    /* 根据固件资源裁剪，定义支持的最大时序步骤硬上限 */
    static constexpr uint8_t kMaxSequenceSteps = 12U;

    /* 单步时序动作描述结构体 */
    struct SequenceStep
    {
        LightEffectEngine::EffectRenderAction pAction;           ///< 调光纯算子指针
        uint16_t                              targetChannels[4]; ///< 各通道逻辑目标值
        uint8_t                               brightness;        ///< 当前步骤的全局主亮度 (0~255)
        uint16_t                              durationMs;        ///< 当前步骤持续时间 (ms)
        uint8_t                               repeatCount;       ///< 当前单步重复执行次数（0表示执行1次）
    };

    /**
     * @brief 获取时序调度器全局单例
     */
    static LightSequenceScheduler& Instance()
    {
        static LightSequenceScheduler instance;
        return instance;
    }

    /* 初始化调度器并注入底层引擎的回调纽带 */
    void Init();

    /* 启动一个链式时序灯效（支持单步退化与多步级联）*/
    void StartSequence(const SequenceStep* steps, uint8_t count, bool isLoopForever = false);

    /* 启动单步灯效（不涉及链式时序） */
    void StartSingleEffect(LightEffectEngine::EffectRenderAction pAction, const uint16_t* targetChannels,
                           uint8_t brightness, uint16_t durationMs);
    /* 终止时序链；clearHardwareOutput=false 时保留当前 PWM（便于后续渐变接续） */
    void StopSequence(bool clearHardwareOutput = true);

    /** @brief 一次性时序链自然结束时的回调（仅非循环链触发一次后自动清除） */
    using SequenceFinishedCallback = void (*)(void);
    void RegisterSequenceFinishedCallback(SequenceFinishedCallback callback);

    /* 查询当前时序链是否正在动态运行 */
    bool IsSequenceActive() const
    {
        return m_isSequenceActive;
    }
    bool IsAnyChannelActive() const
    {
        return LightEffectEngine::Instance().IsAnyChannelActive();
    }

    /* 供底层引擎在 Ticks 耗尽时执行倒灌的静态路由桥接函数 */
    static void EngineSequenceBridgeRaw();

  private:
    LightSequenceScheduler();
    ~LightSequenceScheduler()                                        = default;
    LightSequenceScheduler(const LightSequenceScheduler&)            = delete;
    LightSequenceScheduler& operator=(const LightSequenceScheduler&) = delete;

    /* 内部私有单步核心跃迁驱动程序 */
    void ExecuteCurrentStep();

    SequenceStep m_stepPool[kMaxSequenceSteps]; ///< 静态内存安全对象池
    uint8_t      m_totalSteps;                  ///< 当前加载的时序总步数
    uint8_t      m_currentStepIndex;            ///< 当前正在运转的步骤索引
    uint8_t      m_currentRepeatLeft;           ///< 当前单步剩余的重复次数
    bool         m_isSequenceActive;            ///< 调度器全局运行状态机
    bool         m_isLoopForever;               ///< 是否整链无限循环标志位
    SequenceFinishedCallback m_finishedCallback{nullptr}; ///< 非循环链完结通知
};
