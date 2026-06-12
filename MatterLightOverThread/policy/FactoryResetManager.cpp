/**
 * @file FactoryResetManager.cpp
 * @brief 恢复出厂/软复位策略实现
 * @layer Policy
 */
#include "FactoryResetManager.h"
#include "../integration/MatterBridge.h"
#include "ColorLibrary.h"
#include "AppConfig.h"

static constexpr uint8_t kLongPressWarningCount = 25; ///< 5s（200ms 脉冲 × 25）
static constexpr uint8_t kLongPressResetCount   = 50; ///< 10s

/** @brief 获取 FactoryResetManager 单例 */
FactoryResetManager &FactoryResetManager::instance()
{
    static FactoryResetManager s_mgr;
    return s_mgr;
}

/** @brief 5s 预警：主灯混合闪烁 + 红灯混合闪烁 */
void FactoryResetManager::startWarningEffects()
{
    SILABS_LOG("[FactoryReset] 5s warning effects start");
    WrgbColor raw = LightEngine::instance().rawColor();
    WrgbColor def = ColorLibrary::instance().colorAtCycleIndex(ColorLibrary::kDefaultCycleIndex);

    MixedEffectStep steps[4] = {
        {EffectModeBlink, true, 255, raw, 800, 3},
        {EffectModeBlink, true, 255, raw, 2400, 1},
        {EffectModeHold, false, 255, def, 2000, 1},
        {EffectModeBlink, true, 255, def, 400, 2},
    };

    LightEngine::instance().runMixedSequence(steps, 4, onResetCompleteCallback);
    IndicatorService::instance().startRedBlinkMixed(800, 3, 2400, 1);
}

/** @brief 复位动画结束：淡入默认色温 #7 */
void FactoryResetManager::onResetCompleteCallback()
{
    SILABS_LOG("[FactoryReset] reset effect complete, fade to default");
    LightEngine &le = LightEngine::instance();
    le.setColorFromCycleIndex(ColorLibrary::kDefaultCycleIndex, 800);
    le.updateNormalState(true, le.brightness(), le.rawColor());
    le.saveStateToFlash();
}

/** @brief 10s 触发：仅清 Matter/Thread 配网，不重启 */
void FactoryResetManager::executeSoftReset()
{
    SILABS_LOG("[FactoryReset] 10s reached, trigger soft network reset");
    MatterBridge::instance().triggerSoftNetworkReset();
}

/** @brief 长按脉冲计数：5s 预警 / 10s 复位 */
void FactoryResetManager::onLongPressTick(uint16_t count)
{
    if (count == kLongPressWarningCount)
        startWarningEffects();
    if (count == kLongPressResetCount)
        executeSoftReset();
}

/** @brief 5~10s 之间松开：取消复位流程 */
void FactoryResetManager::onLongPressRelease(uint16_t savedCount)
{
    if (savedCount >= kLongPressWarningCount && savedCount < kLongPressResetCount)
        cancel();
}

/** @brief 取消预警灯效与红灯闪烁 */
void FactoryResetManager::cancel()
{
    SILABS_LOG("[FactoryReset] cancelled before 10s");
    LightEngine::instance().stopMixedEffects();
    IndicatorService::instance().stopRedBlink();
}
