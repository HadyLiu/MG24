/**
 * @file ButtonEventPacket.h
 * @brief 按键中间层邮箱消息、动作码与事件类型定义
 * @author hady
 * @date 2026-06-15
 * @layer Middleware
 * @note ButtonInput 投递 ButtonMailMsg；ButtonService 消费并分发。
 *       本头文件为 ButtonInput 与 ButtonService 的唯一共享协议。
 */
#pragma once

#include <stdint.h>

/** @brief 板级按键索引：与 sl_simple_button_array 顺序一致 */
namespace ButtonBoard {
static constexpr uint8_t kLightSwitchIdx = 0U; /**< btn0  PA05 开/关 */
static constexpr uint8_t kSystemResetIdx = 1U; /**< btn_rst0 PA06 系统/复位 */
} // namespace ButtonBoard

/**
 * @brief 按键语义动作码
 */
struct ButtonAction
{
    static constexpr uint8_t kShortPress       = 0x03U;
    static constexpr uint8_t kDoublePress      = 0x04U;
    static constexpr uint8_t kLongPressStart   = 0x05U;
    static constexpr uint8_t kLongPressing     = 0x06U;
    static constexpr uint8_t kLongPressRelease = 0x07U;
};

/**
 * @brief 投递给服务层的按键事件载荷
 */
enum class ButtonEventType : uint8_t
{
    ShortPress,       ///< 短按：10ms < 按下时长 < 700ms
    DoublePress,      ///< 双击：700ms 内第二次按下
    LongPressStart,   ///< 长按开始：按下达到 700ms
    LongPressing,     ///< 长按脉冲：700ms 后每 200ms
    LongPressRelease, ///< 长按松开：携带按下时长 ms
};

/**
 * @brief 服务层消费的事件结构
 */
struct ButtonEvent
{
    uint8_t         buttonIndex;
    ButtonEventType type;
    uint16_t        longPressCount;
};

/**
 * @brief 按键语义与控制邮件载荷（静态队列元素，无动态分配）
 */
struct ButtonMailMsg
{
    uint8_t  buttonIdx;      ///< 按键索引
    uint8_t  action;         ///< ButtonAction::k*
    uint16_t longPressCount; ///< LongPressing 脉冲序号；Release 时为按下时长 ms
};
