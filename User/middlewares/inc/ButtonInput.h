/**
 * @file ButtonInput.h
 * @brief 按键输入中间层：纯软件状态机，识别短/双/长按语义
 * @author hady
 * @date 2026-06-12
 * @layer Middleware
 * @note 硬件边沿经 PushEdgeEvent 注入（btn0/btn_rst0 均为 EXTI 中断）；
 *       MailPoster 与 10ms tick 均由 entry 注册，本层不感知 ButtonService。
 */
#pragma once

#include "ButtonEventPacket.h"
#include <cstdint>

/**
 * @brief 按键输入状态机（无 RTOS / 无硬件定时器依赖）
 */
class ButtonInput {
 public:
  /** @brief 语义事件投递回调，由 entry 注册至 ButtonService::MailPoster */
  using MailPoster = void (*)(const ButtonMailMsg& msg);

  enum class State : uint8_t { Idle, WaitRelease, WaitDouble, WaitReleaseTimeout };

  /**
   * @brief 获取 ButtonInput 单例
   * @return 单例引用
   */
  static ButtonInput& Instance() {
    static ButtonInput buttonInput;
    return buttonInput;
  }

  /**
   * @brief 复位状态机
   * @return 无
   */
  void Init();

  /* 注册邮箱投递函数（由 entry 注入） */
  void RegisterMailPosterCallback(MailPoster poster);

  /**
   * @brief 外部 10ms tick 驱动状态机计时
   * @param elapsedMs 本次 tick 毫秒增量（通常为 10）
   * @return 无
   */
  void UpdateTicks(uint32_t elapsedMs);

  /**
   * @brief 硬件边沿事件入口（GPIO/按键中断回调转发）
   * @param button    按键索引
   * @param isPressed true=按下，false=松开
   * @return 无
   */
  void PushEdgeEvent(uint8_t button, bool isPressed);

  ButtonInput(const ButtonInput&) = delete;
  ButtonInput& operator=(const ButtonInput&) = delete;

 private:
  ButtonInput() = default;
  ~ButtonInput() = default;

  void TimerStartStop(bool start);

  void PostFinalEvent(uint8_t button, uint8_t action, uint16_t payload);
  void ResetLongPulseIndex();

  MailPoster m_mailPoster{nullptr};

  State m_state{State::Idle};
  uint8_t m_lastButton{0};
  uint32_t m_pressDurationMs{0};
  uint32_t m_longCountMs{0};
  uint32_t m_doubleClickTimerMs{0};
  uint32_t m_releaseDelayMs{0};
  uint32_t m_longPulseIdx{0};
  bool m_longPressStarted{false};
  bool m_doubleClickPending{false};
};
