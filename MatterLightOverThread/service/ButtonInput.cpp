/**
 * @file ButtonInput.cpp
 * @brief 按键输入服务实现（含去抖状态机）
 * @author hady
 * @date 2026-06-12
 * @layer Service
 */
#include "ButtonInput.h"
#include "AppConfig.h"
#include "AppTask.h"
#include "app/AppLog.h"
#include "app/WakeControl.h"
#include <cmsis_os2.h>

#define BTN_SCAN_PERIOD_MS 50
#define LONG_PRESS_TIME_MS 700
#define DOUBLE_CLICK_GAP_MS 700
#define LONG_PRESS_PULSE_MS 200

#define TICKS_FOR_LONG_START (LONG_PRESS_TIME_MS / BTN_SCAN_PERIOD_MS)
#define TICKS_FOR_PULSE (LONG_PRESS_PULSE_MS / BTN_SCAN_PERIOD_MS)

ButtonInput::State ButtonInput::s_state       = ButtonInput::State::Idle;
uint8_t ButtonInput::s_lastButton             = 0;
osTimerId_t ButtonInput::s_timerId            = nullptr;
uint32_t ButtonInput::s_longCount             = 0;

/** @brief 获取 ButtonInput 单例 */
ButtonInput& ButtonInput::instance()
{
  static ButtonInput s_input;
  return s_input;
}

/** @brief 初始化按键定时器 */
void ButtonInput::init()
{
  LOG_BTN("[ButtonInput] init");
  s_timerId = osTimerNew(timerCallback, osTimerPeriodic, nullptr, nullptr);
}

/** @brief 注册按键事件回调 */
void ButtonInput::setHandler(Handler handler)
{
  m_handler = handler;
}

/** @brief 将 AppButtonEvent 转为 ButtonEvent 并分发 */
void ButtonInput::onAppEvent(AppEvent* aEvent)
{
  if (!m_handler || !aEvent)
  {
    return;
  }

  auto* pBtn = reinterpret_cast<AppButtonEvent*>(aEvent);
  ButtonEvent ev{};
  ev.buttonIndex    = pBtn->ButtonEvent.ButtonIdx;
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
    LOG_BTN("[ButtonInput] long press start btn=%u", ev.buttonIndex);
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

/** @brief 向 AppTask 队列投递最终按键事件 */
void ButtonInput::postFinalEvent(uint8_t button, uint8_t action,
                                 uint16_t longPressCount)
{
  AppButtonEvent final_event;
  final_event.Type                  = AppButtonEvent::kEventType_Button;
  final_event.ButtonEvent.ButtonIdx   = button;
  final_event.ButtonEvent.Action      = action;
  final_event.ButtonEvent.LongPressCount = longPressCount;
  final_event.Handler                 = MyButtonActionHandler;
  AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&final_event));
}

/** @brief SDK 按键中断回调：ISR 内仅投递边沿事件 */
void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction)
{
  static uint32_t s_last_interrupt_tick = 0;
  uint32_t current_tick                 = osKernelGetTickCount();
  if ((current_tick - s_last_interrupt_tick) < 5)
  {
    return;
  }
  s_last_interrupt_tick = current_tick;

  uint8_t is_pressed = (btnAction == 1);

  AppButtonEvent btn_event;
  btn_event.Type                  = AppButtonEvent::kEventType_Button;
  btn_event.ButtonEvent.ButtonIdx = button;
  btn_event.ButtonEvent.Action =
      is_pressed ? ButtonInput::kInternalEdgePress
                 : ButtonInput::kInternalEdgeRelease;
  btn_event.ButtonEvent.LongPressCount = 0;
  btn_event.Handler                    = ButtonInput::stateMachineHandler;
  AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event));
}

/** @brief 边沿事件状态机处理（任务上下文） */
void ButtonInput::stateMachineHandler(AppEvent* aEvent)
{
  AppButtonEvent* btn_event = reinterpret_cast<AppButtonEvent*>(aEvent);
  uint8_t button            = btn_event->ButtonEvent.ButtonIdx;
  uint8_t action            = btn_event->ButtonEvent.Action;

  ConditionalWake_up();

  if (action == ButtonInput::kInternalEdgePress)
  {
    s_lastButton = button;

    if (s_state == State::Idle)
    {
      s_state    = State::WaitRelease;
      s_longCount = 0;
      osTimerStart(s_timerId, BTN_SCAN_PERIOD_MS);
    }
    else if (s_state == State::WaitDouble)
    {
      osTimerStop(s_timerId);
      s_state = State::Idle;
      postFinalEvent(button, AppButtonEvent::kButtonAction_DoublePress, 0);
    }
  }
  else if (action == ButtonInput::kInternalEdgeRelease)
  {
    s_lastButton = button;

    if (s_state == State::WaitRelease)
    {
      osTimerStop(s_timerId);

      if (s_longCount < TICKS_FOR_LONG_START)
      {
        s_state = State::WaitDouble;
        osTimerStart(s_timerId, DOUBLE_CLICK_GAP_MS);
      }
      else
      {
        s_state = State::Idle;
        postFinalEvent(button, AppButtonEvent::kButtonAction_LongPressRelease,
                       static_cast<uint16_t>(s_longCount));
        s_longCount = 0;
      }
    }
  }
}

/** @brief 按键定时器回调 */
void ButtonInput::timerCallback(void* argument)
{
  (void)argument;
  AppButtonEvent btn_event;
  btn_event.Type                  = AppButtonEvent::kEventType_Button;
  btn_event.ButtonEvent.ButtonIdx = s_lastButton;
  btn_event.Handler               = MyButtonActionHandler;

  if (s_state == State::WaitRelease)
  {
    s_longCount++;

    if (s_longCount == TICKS_FOR_LONG_START)
    {
      btn_event.ButtonEvent.Action =
          AppButtonEvent::kButtonAction_LongPressStart;
      btn_event.ButtonEvent.LongPressCount = 1;
      AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event));
    }
    else if (s_longCount > TICKS_FOR_LONG_START &&
             ((s_longCount - TICKS_FOR_LONG_START) % TICKS_FOR_PULSE == 0))
    {
      btn_event.ButtonEvent.Action = AppButtonEvent::kButtonAction_LongPressing;
      btn_event.ButtonEvent.LongPressCount =
          static_cast<uint16_t>(
              ((s_longCount - TICKS_FOR_LONG_START) / TICKS_FOR_PULSE) + 1);
      if (btn_event.ButtonEvent.LongPressCount >= 250)
      {
        btn_event.ButtonEvent.LongPressCount = 250;
      }
      AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event));
    }
  }
  else if (s_state == State::WaitDouble)
  {
    osTimerStop(s_timerId);
    s_state = State::Idle;

    btn_event.ButtonEvent.Action         = AppButtonEvent::kButtonAction_ShortPress;
    btn_event.ButtonEvent.LongPressCount = 0;
    AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event));
  }
  else if (s_state == State::Idle && s_longCount >= TICKS_FOR_LONG_START)
  {
    osTimerStop(s_timerId);
    btn_event.ButtonEvent.Action =
        AppButtonEvent::kButtonAction_LongPressRelease;
    btn_event.ButtonEvent.LongPressCount = static_cast<uint16_t>(s_longCount);
    AppTask::GetAppTask().PostEvent(reinterpret_cast<AppEvent*>(&btn_event));
    s_longCount = 0;
  }
}

/** @brief AppTask 队列消费入口 */
void MyButtonActionHandler(AppEvent* aEvent)
{
  ButtonInput::instance().onAppEvent(aEvent);
}
