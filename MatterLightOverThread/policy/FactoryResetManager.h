/**
 * @file FactoryResetManager.h
 * @brief 恢复出厂/软复位策略：5s 预警灯效、10s 清配网、取消与完成动画
 * @layer Policy
 */
#pragma once

#include "LightEngine.h"
#include "IndicatorService.h"
#include <stdint.h>

/** @brief 长按 10s 软复位与预警灯效管理 */
class FactoryResetManager
{
public:
    static FactoryResetManager &instance();

    void onLongPressTick(uint16_t count);
    void onLongPressRelease(uint16_t savedCount);
    void cancel();

private:
    FactoryResetManager() = default;
    void startWarningEffects();
    void executeSoftReset();
    static void onResetCompleteCallback();
};
