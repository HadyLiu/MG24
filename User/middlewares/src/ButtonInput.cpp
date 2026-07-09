/**
 * @file ButtonInput.cpp
 * @brief 按键输入中间层实现：短/双/长按状态机
 * @author hady
 * @date 2026-06-15
 * @layer Middleware
 * @note 时间轴由 entry 注册的 10ms tick 调用 UpdateTicks；语义事件经
 *       entry 注册的 MailPoster 投递至 ButtonService 邮箱。
 */
#include "ButtonInput.h"
#include "BspTimer.h"
#include "DebugLog.h"

namespace {

static constexpr uint32_t DEBOUNCE_MS     = 10U;
static constexpr uint32_t LONG_PRESS_MS   = 700U;
static constexpr uint32_t DOUBLE_GAP_MS   = 700U;
static constexpr uint32_t LONG_PULSE_MS   = 200U;
static constexpr uint32_t RELEASE_STOP_MS = 1000U;

} // namespace

static BspTimer s_buttonScanTimer; /**< 手势状态机 10ms */

void ButtonInput::TimerStartStop(bool start)
{
    if (start)
    {
        s_buttonScanTimer.TurnOnOff(true);
    }
    else if (s_buttonScanTimer.IsRunning())
    {
        s_buttonScanTimer.TurnOnOff(false);
    }
}

/**
 * @brief 复位状态机至 Idle
 * @return 无
 */
void ButtonInput::Init()
{
    LOG_BTN("[ButtonInput] init");
    m_state              = State::Idle;
    m_pressDurationMs    = 0;
    m_longCountMs        = 0;
    m_doubleClickTimerMs = 0;
    m_releaseDelayMs     = 0;
    m_longPulseIdx       = 0;
    m_longPressStarted   = false;
    m_doubleClickPending = false;

    s_buttonScanTimer.Init([](uint16_t elapsedMs) { ButtonInput::Instance().UpdateTicks(elapsedMs); }, 10U, this);
}

/**
 * @brief 注册邮箱投递回调
 * @param poster 投递函数指针
 * @return 无
 */
void ButtonInput::RegisterMailPosterCallback(MailPoster poster)
{
    if (poster == nullptr)
    {
        LOG_BTN("[ButtonInput] RegisterMailPosterCallback: null");
        return;
    }
    m_mailPoster = poster;
}

/**
 * @brief 重置长按脉冲计数索引
 * @return 无
 */
void ButtonInput::ResetLongPulseIndex()
{
    m_longPulseIdx     = 0;
    m_longPressStarted = false;
}

/**
 * @brief 投递按键语义邮件
 * @param button    按键索引
 * @param action    ButtonAction 动作码
 * @param payload   脉冲序号或松开时长 ms
 * @param fromIsr   是否 ISR 上下文
 * @return 无
 */
void ButtonInput::PostFinalEvent(uint8_t button, uint8_t action, uint16_t payload)
{
    if (m_mailPoster == nullptr)
    {
        return;
    }

    ButtonMailMsg msg{};
    msg.buttonIdx      = button;
    msg.action         = action;
    msg.longPressCount = payload;
    m_mailPoster(msg);
}

/**
 * @brief 处理硬件边沿
 * @param button    按键索引
 * @param isPressed 按下/松开
 * @param fromIsr   ISR 标志
 * @return 无
 */
void ButtonInput::PushEdgeEvent(uint8_t button, bool isPressed)
{
    m_lastButton = button;

    if (isPressed)
    {
        if (m_state == State::Idle)
        {
            // 【修改点】不再发送扫描控制邮件，直接利用注册的接口函数指针就地启动硬件时钟
            TimerStartStop(true);
            m_state       = State::WaitRelease;
            m_longCountMs = 0;
            ResetLongPulseIndex();
        }
        else if (m_state == State::WaitDouble)
        {
            m_state              = State::WaitRelease;
            m_longCountMs        = 0;
            m_doubleClickPending = true;
            ResetLongPulseIndex();
            PostFinalEvent(button, ButtonAction::kDoublePress, 0);
        }
        else if (m_state == State::WaitReleaseTimeout)
        {
            // 短按已确认后的 1s 停表等待期内允许开始下一次按键
            m_releaseDelayMs = 0;
            m_state          = State::WaitRelease;
            m_longCountMs    = 0;
            ResetLongPulseIndex();
        }
    }
    else
    {
        if (m_state == State::WaitRelease)
        {
            m_pressDurationMs = m_longCountMs;

            if (m_longCountMs >= LONG_PRESS_MS)
            {
                m_doubleClickPending = false;
                PostFinalEvent(m_lastButton, ButtonAction::kLongPressRelease, static_cast<uint16_t>(m_longCountMs));
                m_state          = State::WaitReleaseTimeout;
                m_releaseDelayMs = RELEASE_STOP_MS;
            }
            else if (m_pressDurationMs >= DEBOUNCE_MS)
            {
                if (m_doubleClickPending)
                {
                    m_doubleClickPending = false;
                    m_state              = State::WaitReleaseTimeout;
                    m_releaseDelayMs     = RELEASE_STOP_MS;
                }
                else
                {
                    m_state              = State::WaitDouble;
                    m_doubleClickTimerMs = 0;
                }
            }
            else
            {
                m_state          = State::WaitReleaseTimeout;
                m_releaseDelayMs = RELEASE_STOP_MS;
            }
        }
    }
}

/**
 * @brief 10ms tick 状态机推进
 * @param elapsedMs tick 增量毫秒
 * @return 无
 */
void ButtonInput::UpdateTicks(uint32_t elapsedMs)
{
    if (m_state == State::WaitRelease)
    {
        m_longCountMs += elapsedMs;

        if (!m_longPressStarted && m_longCountMs >= LONG_PRESS_MS)
        {
            m_longPressStarted = true;
            PostFinalEvent(m_lastButton, ButtonAction::kLongPressStart, 0);
        }
        else if (m_longPressStarted && m_longCountMs > LONG_PRESS_MS)
        {
            const uint32_t relativeTime = m_longCountMs - LONG_PRESS_MS;
            const uint32_t currIdx      = relativeTime / LONG_PULSE_MS;

            if (currIdx > m_longPulseIdx)
            {
                m_longPulseIdx = currIdx;
                uint16_t count = static_cast<uint16_t>(currIdx);
                if (count > 250U)
                {
                    count = 250U;
                }
                PostFinalEvent(m_lastButton, ButtonAction::kLongPressing, count);
            }
        }
    }
    else if (m_state == State::WaitDouble)
    {
        m_doubleClickTimerMs += elapsedMs;
        if (m_doubleClickTimerMs >= DOUBLE_GAP_MS)
        {
            if (m_pressDurationMs >= DEBOUNCE_MS && m_pressDurationMs < LONG_PRESS_MS)
            {
                PostFinalEvent(m_lastButton, ButtonAction::kShortPress, 0);
            }
            m_state          = State::WaitReleaseTimeout;
            m_releaseDelayMs = RELEASE_STOP_MS;
        }
    }
    else if (m_state == State::WaitReleaseTimeout)
    {
        if (m_releaseDelayMs <= elapsedMs)
        {
            m_state          = State::Idle;
            m_releaseDelayMs = 0;
            // 【修改点】状态机回归
            // Idle，直接利用函数指针就地关闭硬件时钟，系统进入极低功耗
            TimerStartStop(false);
        }
        else
        {
            m_releaseDelayMs -= elapsedMs;
        }
    }
}
