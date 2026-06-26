/**
 * @file ButtonService.cpp
 * @brief 按键服务层实现
 * @author hady
 * @date 2026-06-15
 * @layer Service
 * @note 管理所有按键事件的语义翻译与分发；不直接 #include ButtonInput，
 *       解耦 BSP与业务。 ButtonMailMsg → KeyEventType 翻译表：
 *       短按→亮度循环，双击→颜色循环，长按开始→清配网，长按松开→停配网。
 */
#include "ButtonService.h"
#include "DebugLog.h"

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
  LOG_BTN("短按 btn=%u", buttonIdx);
  PostKeyEventRaw(KeyEventType::ShortPressCycleBrightness);
}

/**
 * @brief 双击：WRGB 调色板循环
 * @param buttonIdx 按键索引
 * @return 无
 */
void ButtonService::OnDoublePress(uint8_t buttonIdx)
{
  LOG_BTN("双击 btn=%u", buttonIdx);
  PostKeyEventRaw(KeyEventType::DoublePressCycleColor);
}

/**
 * @brief 长按开始：清除配网
 * @param buttonIdx 按键索引
 * @return 无
 */
void ButtonService::OnLongPressStart(uint8_t buttonIdx)
{
  LOG_BTN("长按开始 btn=%u", buttonIdx);
}

/**
 * @brief 长按脉冲（仅日志）
 * @param buttonIdx 按键索引
 * @param count     脉冲计数
 * @return 无
 */
void ButtonService::OnLongPressing(uint8_t buttonIdx, uint16_t count)
{
  LOG_BTN("长按脉冲 btn=%u count=%u", buttonIdx, count);

  if (count == 25U)
  {
    PostKeyEventRaw(KeyEventType::LongPressClearNetLighting);
  }
  else if (count == 50U)
  {
    // 开始清除配网
    PostKeyEventRaw(KeyEventType::LongPressClearNet);
  }
}

/**
 * @brief 长按松开：停止配网（≥700ms）
 * @param buttonIdx   按键索引
 * @param durationMs  按下总时长
 * @return 无
 */
void ButtonService::OnLongPressRelease(uint8_t buttonIdx, uint16_t durationMs)
{
  LOG_BTN("长按松开 btn=%u duration=%ums", buttonIdx, durationMs);
  if (durationMs >= 5000U && durationMs < 10000U)
  {
    PostKeyEventRaw(KeyEventType::LongPressStopNet);
  }
}
