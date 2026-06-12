/**
 * @file ButtonInput.h
 * @brief 按键输入服务：去抖状态机、事件转强类型 ButtonEvent，分发给 Policy
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 吸收原 driver/button 状态机；ISR 仅投递边沿事件，不含业务决策。
 */
#pragma once

#include "AppEvent.h"
#include <cmsis_os2.h>
#include <stdint.h>

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
  uint8_t buttonIndex;
  ButtonEventType type;
  uint16_t longPressCount;
};

/** @brief 底层按键 AppEvent 结构（与 AppTask 队列兼容） */
struct AppButtonEvent
{
  enum AppButtonEventType
  {
    kEventType_None   = 0,
    kEventType_Button = 1,
  };

  enum AppButtonAction
  {
    kButtonAction_ShortPress       = 0,
    kButtonAction_DoublePress      = 1,
    kButtonAction_LongPressStart   = 2,
    kButtonAction_LongPressing     = 3,
    kButtonAction_LongPressRelease = 4,
  };

  uint16_t Type;

  struct
  {
    uint8_t ButtonIdx;
    uint8_t Action;
    uint16_t LongPressCount;
  } ButtonEvent;

  typedef void (*EventHandler)(AppEvent*);
  EventHandler Handler;
};

/** @brief 按键输入服务 */
class ButtonInput
{
public:
  using Handler = void (*)(const ButtonEvent&);

  /** @brief 按键状态机状态 */
  enum class State : uint8_t
  {
    Idle = 0,
    WaitRelease,
    WaitDouble
  };

  static constexpr uint8_t kInternalEdgePress   = 0xF0;
  static constexpr uint8_t kInternalEdgeRelease = 0xF1;

  /** @brief 获取 ButtonInput 单例
   *  @return 单例引用 */
  static ButtonInput& instance();

  /** @brief 初始化按键定时器与状态机
   *  @return 无 */
  void init();

  /**
   * @brief 注册按键事件处理回调
   * @param handler 事件回调（通常指向 ButtonPolicy::handle 适配器）
   * @return 无
   */
  void setHandler(Handler handler);

  /**
   * @brief 处理来自任务队列的按键 AppEvent
   * @param event 底层按键事件指针
   * @return 无
   */
  void onAppEvent(AppEvent* event);

  /**
   * @brief 边沿事件状态机处理（任务上下文）
   * @param event AppEvent 指针
   * @return 无
   */
  static void stateMachineHandler(AppEvent* event);

private:
  static State s_state;
  static uint8_t s_lastButton;
  static osTimerId_t s_timerId;
  static uint32_t s_longCount;

  Handler m_handler = nullptr;

  /** @brief 定时器回调
   *  @param argument RTOS 定时器参数（未使用）
   *  @return 无 */
  static void timerCallback(void* argument);

  /** @brief 向 AppTask 队列投递最终按键事件
   *  @param button 按键索引
   *  @param action 动作码
   *  @param longPressCount 长按计数
   *  @return 无 */
  static void postFinalEvent(uint8_t button, uint8_t action,
                             uint16_t longPressCount);
};

/**
 * @brief SDK 按键中断回调
 * @param button   按键索引
 * @param btnAction SDK 按键动作
 * @return 无
 * @note ISR 内禁止日志/阻塞，仅 PostEvent 投递边沿事件
 */
void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction);

/**
 * @brief AppTask 队列消费入口
 * @param aEvent AppEvent 指针
 * @return 无
 */
void MyButtonActionHandler(AppEvent* aEvent);
