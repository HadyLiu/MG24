/**
 * @file LightSequenceScheduler.cpp
 * @brief 全权接管型多阶段灯效时序调度器实现
 */
#include "LightSequenceScheduler.h"
#include <cassert>
#include <cstring>

/**
 * @brief 内部使用的单步核心跃迁驱动程序
 * @note 负责根据当前步骤参数调用底层引擎启动对应的灯
 */
LightSequenceScheduler::LightSequenceScheduler()
    : m_totalSteps(0U), m_currentStepIndex(0U), m_currentRepeatLeft(0U), m_isSequenceActive(false),
      m_isLoopForever(false)
{
    memset(m_stepPool, 0, sizeof(m_stepPool));
}

/**
 * @brief 初始化灯效时序调度器
 * @note 将静态桥接路由注册到 LightEffectEngine 中
 */
void LightSequenceScheduler::Init()
{
    // 将静态桥接路由直接注册到 LightEffectEngine 中
    LightEffectEngine::Instance().RegisterMixedTimingCallback(LightSequenceScheduler::EngineSequenceBridgeRaw);
}

/**
 * @brief 启动灯效时序链
 * @param steps 灯效步骤数组指针
 * @param count 步骤数量
 * @param isLoopForever 是否循环播放
 */
void LightSequenceScheduler::StartSequence(const SequenceStep* steps, uint8_t count, bool isLoopForever)
{
    if ((steps == nullptr) || (count == 0U))
    {
        return;
    }

    // 防御性编程：越界拦截，严禁污染静态池外内存
    uint8_t safeCount = (count > kMaxSequenceSteps) ? kMaxSequenceSteps : count;

    // 静态深拷贝：全面隔离上层的临时局部变量生存期
    memcpy(m_stepPool, steps, static_cast<size_t>(safeCount) * sizeof(SequenceStep));

    m_totalSteps        = safeCount;
    m_currentStepIndex  = 0U;
    m_isLoopForever     = isLoopForever;
    m_isSequenceActive  = true;
    m_currentRepeatLeft = m_stepPool[0].repeatCount;

    // 立刻执行首步驱动
    ExecuteCurrentStep();
}

/**
 * @brief 启动单步灯效
 * @param pAction 灯效渲染算子函数指针
 * @param targetChannels 目标通道占空比数组指针
 * @param brightness 全局亮度值
 * @param durationMs 特效持续时间（毫秒）
 */
void LightSequenceScheduler::StartSingleEffect(LightEffectEngine::EffectRenderAction pAction,
                                               const uint16_t* targetChannels, uint8_t brightness, uint16_t durationMs)
{

    if ((pAction == nullptr) || (targetChannels == nullptr))
    {
        return;
    }

    // 2. 状态机切变：标记当前处于有时序活动状态，但是总步数只有 1 步
    m_isSequenceActive  = true;
    m_isLoopForever     = false;
    m_totalSteps        = 1U;
    m_currentStepIndex  = 0U;
    m_currentRepeatLeft = 0U;

    // 3. 将单步参数灌入静态池的第一项，方便后续状态机安全回读或在 UpdateTicks
    // 耗尽时正常收尾
    m_stepPool[0].pAction     = pAction;
    m_stepPool[0].brightness  = brightness;
    m_stepPool[0].durationMs  = durationMs;
    m_stepPool[0].repeatCount = 0U;

    for (uint8_t i = 0U; i < LightEffectEngine::Instance().GetActiveChannels(); ++i)
    {
        m_stepPool[0].targetChannels[i] = targetChannels[i];
    }

    // 4. 全权代客调用底层引擎拉起高频渲染
    LightEffectEngine::Instance().StartEffect(pAction, targetChannels, brightness, durationMs);
}

/**
 * @brief 停止灯效时序链
 */
void LightSequenceScheduler::StopSequence()
{
    m_isSequenceActive = false;
    m_isLoopForever    = false;
    // 直接通知底层算子彻底挂起
    LightEffectEngine::Instance().StopCurrentEffect(true);
}

/**
 * @brief 内部使用的单步核心跃迁驱动程序
 * @note 负责根据当前步骤参数调用底层引擎启动对应的灯效
 */
void LightSequenceScheduler::ExecuteCurrentStep()
{
    if (!m_isSequenceActive || (m_currentStepIndex >= m_totalSteps))
    {
        m_isSequenceActive = false;
        return;
    }

    const SequenceStep& step = m_stepPool[m_currentStepIndex];

    // 全权代客调用底层渲染引擎核心，上层不再感知引擎
    LightEffectEngine::Instance().StartEffect(step.pAction, step.targetChannels, step.brightness, step.durationMs);
}

/**
 * @brief 供底层引擎在 Ticks 耗尽时执行倒灌的静态路由桥接函数
 * @note 该函数会被 LightEffectEngine
 * 在当前特效的时间轴耗尽时回调，以触发时序链的下一个阶段
 */
void LightSequenceScheduler::EngineSequenceBridgeRaw()
{
    LightSequenceScheduler& self = LightSequenceScheduler::Instance();

    if (!self.m_isSequenceActive)
    {
        return;
    }

    // 1. 优先处理单步内部的重复次数（例如 Blink 动作的循环）
    if (self.m_currentRepeatLeft > 0U)
    {
        self.m_currentRepeatLeft--;
        self.ExecuteCurrentStep();
    }
    else
    {
        // 2. 步进到时序链的下一个阶段
        self.m_currentStepIndex++;

        if (self.m_currentStepIndex < self.m_totalSteps)
        {
            self.m_currentRepeatLeft = self.m_stepPool[self.m_currentStepIndex].repeatCount;
            self.ExecuteCurrentStep();
        }
        else
        {
            // 3. 已经走到整条链的终点，检查是否需要全链死循环
            if (self.m_isLoopForever)
            {
                self.m_currentStepIndex  = 0U; // 时间轮回，重头开始
                self.m_currentRepeatLeft = self.m_stepPool[0].repeatCount;
                self.ExecuteCurrentStep();
            }
            else
            {
                // 链路自然完结
                self.m_isSequenceActive = false;
            }
        }
    }
}
