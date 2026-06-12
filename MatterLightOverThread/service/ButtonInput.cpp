/**
 * @file ButtonInput.cpp
 * @brief 按键输入服务实现
 * @layer Service
 */
#include "ButtonInput.h"
#include "AppConfig.h"

/** @brief 获取 ButtonInput 单例 */
ButtonInput &ButtonInput::instance()
{
    static ButtonInput s_input;
    return s_input;
}

/** @brief 初始化底层按键驱动与定时器 */
void ButtonInput::init()
{
    SILABS_LOG("[ButtonInput] init");
    MyCustomButtonInit();
}

/** @brief 注册按键事件回调（通常为 ButtonPolicy::handle） */
void ButtonInput::setHandler(Handler handler)
{
    m_handler = handler;
}

/** @brief 将 AppButtonEvent 转为 ButtonEvent 并分发 */
void ButtonInput::onAppEvent(AppEvent *aEvent)
{
    if (!m_handler || !aEvent)
        return;

    auto *pBtn = reinterpret_cast<AppButtonEvent *>(aEvent);
    ButtonEvent ev{};
    ev.buttonIndex = pBtn->ButtonEvent.ButtonIdx;
    ev.longPressCount = pBtn->ButtonEvent.LongPressCount;

    switch (pBtn->ButtonEvent.Action)
    {
    case AppButtonEvent::kButtonAction_ShortPress:
        ev.type = ButtonEventType::ShortPress;
        break;
    case AppButtonEvent::kButtonAction_DoublePress:
        ev.type = ButtonEventType::DoublePress;
        break;
    case AppButtonEvent::kButtonAction_LongPressStart:
        ev.type = ButtonEventType::LongPressStart;
        SILABS_LOG("[ButtonInput] long press start btn=%u", ev.buttonIndex);
        break;
    case AppButtonEvent::kButtonAction_LongPressing:
        ev.type = ButtonEventType::LongPressing;
        break;
    case AppButtonEvent::kButtonAction_LongPressRelease:
        ev.type = ButtonEventType::LongPressRelease;
        break;
    default:
        return;
    }

    m_handler(ev);
}

/** @brief C 桥接：AppTask 注册的按键处理入口 */
void MyButtonActionHandler(AppEvent *aEvent)
{
    ButtonInput::instance().onAppEvent(aEvent);
}
