/**
 * @file ButtonInput.h
 * @brief 按键输入服务：去抖事件转强类型 ButtonEvent，分发给 Policy
 * @layer Service
 */
#pragma once

#include "../driver/button.h"
#include "AppEvent.h"

/** @brief 按键事件类型 */
enum class ButtonEventType : uint8_t
{
    ShortPress,
    DoublePress,
    LongPressStart,
    LongPressing,
    LongPressRelease
};

/** @brief 强类型按键事件，供 ButtonPolicy 消费 */
struct ButtonEvent
{
    uint8_t         buttonIndex;
    ButtonEventType type;
    uint16_t        longPressCount;
};

/** @brief 按键输入服务，桥接 button.cpp 与 ButtonPolicy */
class ButtonInput
{
public:
    using Handler = void (*)(const ButtonEvent &);

    static ButtonInput &instance();

    void init();
    void setHandler(Handler handler);
    void onAppEvent(AppEvent *event);

private:
    Handler m_handler = nullptr;
};
