/**
 * @file ButtonService.cpp
 * @brief 按键服务层实现
 * @author hady
 * @date 2026-06-15
 * @layer Service
 * @note 管理所有按键事件的语义翻译与分发；不直接 #include ButtonInput，
 *       解耦 BSP与业务。 ButtonMailMsg → KeyEventType 翻译表：
 *       短按→亮度循环，双击→颜色循环，长按→清配网（仅 btn_rst0）。
 */
#include "ButtonService.h"
#include "DebugLog.h"

#include "FreeRTOS.h"
#include "timers.h"

namespace {
static constexpr uint8_t kLightSwitchIdx = ButtonBoard::kLightSwitchIdx;
static constexpr uint8_t kSystemResetIdx = ButtonBoard::kSystemResetIdx;

/** @brief 按键语义 ISR → 定时器服务任务，避免 Matter 上报/落盘在 ISR 内被丢弃 */
static void DeferredKeyEventDispatch(void* param1, uint32_t param2)
{
    (void)param1;
    ButtonService::Instance().DispatchKeyEventInTaskRaw(static_cast<KeyEventType>(param2));
}
} // namespace

/**
 * @brief 复位服务状态
 * @return 无
 */
void ButtonService::Init()
{
    LOG_BTN("[ButtonService] init");
}

/**
 * @brief 注册按键语义下游回调
 * @param handler 由 entry 注入
 * @return 无
 */
void ButtonService::RegisterKeyEventHandler(KeyEventHandler handler)
{
    m_keyHandler = handler;
}

/**
 * @brief 分发 ButtonInput 邮件
 * @param msg 按键语义包
 * @return 无
 */
void ButtonService::DispatchMail(const ButtonMailMsg& msg)
{
    switch (msg.action)
    {
    case ButtonAction::kShortPress:
        OnShortPress(msg.buttonIdx);
        break;
    case ButtonAction::kDoublePress:
        OnDoublePress(msg.buttonIdx);
        break;
    case ButtonAction::kLongPressStart:
        OnLongPressStart(msg.buttonIdx);
        break;
    case ButtonAction::kLongPressing:
        OnLongPressing(msg.buttonIdx, msg.longPressCount);
        break;
    case ButtonAction::kLongPressRelease:
        OnLongPressRelease(msg.buttonIdx, msg.longPressCount);
        break;
    default:
        return;
    }
}

/**
 * @brief 投递 KeyEventType 至 entry 注册的下游
 * @param event 按键语义
 * @return 无
 */
void ButtonService::PostKeyEventRaw(KeyEventType event)
{
    if (xPortIsInsideInterrupt() != pdFALSE)
    {
        BaseType_t       higherPriorityTaskWoken = pdFALSE;
        const BaseType_t posted                  = xTimerPendFunctionCallFromISR(
            DeferredKeyEventDispatch, nullptr, static_cast<uint32_t>(event), &higherPriorityTaskWoken);
        if (posted == pdPASS)
        {
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
        else
        {
            LOG_BTN("KeyEvent defer failed: ev=%u", static_cast<uint8_t>(event));
        }
        return;
    }

    DispatchKeyEventInTaskRaw(event);
}

void ButtonService::DispatchKeyEventInTaskRaw(KeyEventType event)
{
    if (m_keyHandler != nullptr)
    {
        m_keyHandler(event);
    }
}

/**
 * @brief 短按：亮度 255→128→0 循环
 * @param buttonIdx 按键索引
 * @return 无
 */
void ButtonService::OnShortPress(uint8_t buttonIdx)
{
    LOG_BTN("ShortPress btn=%u", buttonIdx);
    if (buttonIdx == kLightSwitchIdx)
    {
        PostKeyEventRaw(KeyEventType::ShortPressCycleBrightness);
        return;
    }

    if (buttonIdx == kSystemResetIdx)
    {
        // §15：短按系统键 → 未入网时进入配网；§14 已配网中刷新 15min
        PostKeyEventRaw(KeyEventType::ShortPressOpenCommissioning);
    }
}

/**
 * @brief 双击：仅开/关键切换颜色
 */
void ButtonService::OnDoublePress(uint8_t buttonIdx)
{
    LOG_BTN("DoublePress btn=%u", buttonIdx);
    if (buttonIdx != kLightSwitchIdx)
    {
        return;
    }
    PostKeyEventRaw(KeyEventType::DoublePressCycleColor);
}

/**
 * @brief 长按开始：仅系统键
 */
void ButtonService::OnLongPressStart(uint8_t buttonIdx)
{
    LOG_BTN("LongPressStart btn=%u", buttonIdx);
    if (buttonIdx != kSystemResetIdx)
    {
        return;
    }
}

/**
 * @brief 长按脉冲（仅日志）
 * @param buttonIdx 按键索引
 * @param count     脉冲计数
 * @return 无
 */
void ButtonService::OnLongPressing(uint8_t buttonIdx, uint16_t count)
{
    LOG_BTN("LongPressing btn=%u count=%u", buttonIdx, count);
    if (buttonIdx != kSystemResetIdx)
    {
        return;
    }

    if (count == 40U) // 约 8s：进入重置预警灯效
    {
        PostKeyEventRaw(KeyEventType::LongPressClearNetLighting);
    }
    else if (count == 65U) // 约 13s：武装复位（时序完结后才擦除）
    {
        PostKeyEventRaw(KeyEventType::LongPressClearNet);
    }
}

/**
 * @brief 长按松开：未满 13s 取消重置预警；≥13s 不取消（等时序完结后复位）
 * @param buttonIdx   按键索引
 * @param durationMs  按下总时长
 * @return 无
 */
void ButtonService::OnLongPressRelease(uint8_t buttonIdx, uint16_t durationMs)
{
    LOG_BTN("LongPressRelease btn=%u duration=%ums", buttonIdx, durationMs);
    if (buttonIdx != kSystemResetIdx)
    {
        return;
    }
    // 注解：松开 <13s 重置无效；≥13s 已武装则继续播完时序再复位
    if ((durationMs >= 8000U) && (durationMs < 13000U))
    {
        PostKeyEventRaw(KeyEventType::LongPressStopNet);
    }
}
