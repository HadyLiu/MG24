/**
 * @file ButtonService.h
 * @brief 按键业务服务层：ButtonInput 语义 → KeyEventType
 * @author hady
 * @date 2026-06-15
 * @layer Service
 * @note
 * 仅负责 ButtonMailMsg → KeyEventType 翻译；
 * 不感知 LightDecisionCenter，下游由 entry 注册 KeyEventHandler 注入。
 */
#pragma once

#include "ButtonEventPacket.h"
#include "LightDecisionTypes.h"

/**
 * @class ButtonService
 * @brief 按键语义翻译服务
 */
class ButtonService
{
  public:
    /** @brief 按键语义事件回调，由 entry 注册并转发至 LightDecisionCenter */
    using KeyEventHandler = void (*)(KeyEventType event);

    /**
     * @brief 获取按键服务单例
     * @return ButtonService 引用
     */
    static ButtonService& Instance()
    {
        static ButtonService instance;
        return instance;
    }

    /** @brief 复位服务状态 */
    void Init();

    /**
     * @brief 注册按键语义下游回调
     * @param handler 由 entry 注入
     */
    void RegisterKeyEventHandler(KeyEventHandler handler);

    /**
     * @brief 消费 ButtonInput 投递的邮件
     * @param msg 按键语义包
     */
    void DispatchMail(const ButtonMailMsg& msg);

    /** @brief 任务上下文分发按键语义（ISR 经 DeferredKeyEventDispatch 转入） */
    void DispatchKeyEventInTaskRaw(KeyEventType event);

  private:
    ButtonService() = default;

    void OnShortPress(uint8_t buttonIdx);
    void OnDoublePress(uint8_t buttonIdx);
    void OnLongPressStart(uint8_t buttonIdx);
    void OnLongPressing(uint8_t buttonIdx, uint16_t count);
    void OnLongPressRelease(uint8_t buttonIdx, uint16_t durationMs);
    void PostKeyEventRaw(KeyEventType event);

    KeyEventHandler m_keyHandler{nullptr};
};
