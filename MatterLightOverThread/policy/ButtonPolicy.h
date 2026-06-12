/**
 * @file ButtonPolicy.h
 * @brief 按键领域策略：短按亮度循环、双击换色、长按配网复位编排
 * @author hady
 * @date 2026-06-12
 * @layer Policy
 * @note 消费 ButtonInput 产出的 ButtonEvent，编排
 * LightEngine/MatterBridge/配网/复位。
 */
#pragma once

#include "../integration/MatterBridge.h"
#include "../service/ButtonInput.h"
#include "../service/ColorLibrary.h"
#include "../service/LightEngine.h"
#include "BatteryPolicy.h"
#include "CommissioningManager.h"
#include "FactoryResetManager.h"

/** @brief 按键业务策略，将 ButtonEvent 映射为灯光/Matter/配网动作 */
class ButtonPolicy
{
public:
  /** @brief 获取 ButtonPolicy 单例
   *  @return 单例引用 */
  static ButtonPolicy& instance();

  /**
   * @brief 分发按键事件到对应处理逻辑
   * @param ev 强类型按键事件
   * @return 无
   */
  void handle(const ButtonEvent& ev);

private:
  ButtonPolicy() = default;

  /** @brief 短按：标准三态循环(关->100%->50%->关)/网关单按关，并同步 Matter
   *  @return 无 */
  void onShortPress();

  /** @brief 双击：循环颜色库并上报 Matter 色温
   *  @return 无 */
  void onDoublePress();

  uint16_t m_savedLongPressCount = 0; ///< 长按释放时保存的脉冲计数
};
