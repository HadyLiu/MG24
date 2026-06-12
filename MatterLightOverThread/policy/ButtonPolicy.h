/**
 * @file ButtonPolicy.h
 * @brief 按键领域策略：短按亮度循环、双击换色、长按配网复位编排
 * @layer Policy
 */
#pragma once

#include "BatteryPolicy.h"
#include "CommissioningManager.h"
#include "FactoryResetManager.h"
#include "../service/ButtonInput.h"
#include "../service/ColorLibrary.h"
#include "../service/LightEngine.h"
#include "../integration/MatterBridge.h"

/** @brief 按键业务策略，将 ButtonEvent 映射为灯光/Matter/配网动作 */
class ButtonPolicy
{
public:
    static ButtonPolicy &instance();
    void handle(const ButtonEvent &ev);

private:
    ButtonPolicy() = default;
    void onShortPress();
    void onDoublePress();
    uint16_t m_savedLongPressCount = 0;
};
