/**
 * @file ButtonPolicy.cpp
 * @brief 按键领域策略实现：100/50/关循环、网关单按关、双击换色、长按复位
 * @layer Policy
 */
#include "ButtonPolicy.h"
#include "../driver/powerManage.h"
#include "AppConfig.h"

/** @brief 获取 ButtonPolicy 单例 */
ButtonPolicy &ButtonPolicy::instance()
{
    static ButtonPolicy s_policy;
    return s_policy;
}

/** @brief 分发按键事件到对应处理函数 */
void ButtonPolicy::handle(const ButtonEvent &ev)
{
    switch (ev.type)
    {
    case ButtonEventType::ShortPress:
        SILABS_LOG("[ButtonPolicy] short press btn=%u", ev.buttonIndex);
        onShortPress();
        break;
    case ButtonEventType::DoublePress:
        SILABS_LOG("[ButtonPolicy] double press btn=%u", ev.buttonIndex);
        onDoublePress();
        break;
    case ButtonEventType::LongPressing:
        if (BatteryPolicy::instance().allowUserLightControl())
            FactoryResetManager::instance().onLongPressTick(ev.longPressCount);
        m_savedLongPressCount = ev.longPressCount;
        break;
    case ButtonEventType::LongPressRelease:
        SILABS_LOG("[ButtonPolicy] long press release count=%u", m_savedLongPressCount);
        if (BatteryPolicy::instance().allowUserLightControl())
            FactoryResetManager::instance().onLongPressRelease(m_savedLongPressCount);
        break;
    default:
        break;
    }
}

/** @brief 短按：100% → 50% → 关；网关已开时单按关；同步 Matter */
void ButtonPolicy::onShortPress()
{
    LightEngine  &le = LightEngine::instance();
    MatterBridge &mb = MatterBridge::instance();

    CommissioningManager::instance().onManualTrigger();

    if (le.changeOrigin() == ChangeOriginMatterApp && le.isOn())
    {
        SILABS_LOG("[ButtonPolicy] remote-on override: turn off");
        le.updateNormalState(false, 0, le.rawColor());
        le.setChangeOrigin(ChangeOriginLocalKey);
        le.saveStateToFlash();
        le.startFadeToCurrent(le.calcFadeMsForBrightnessDelta(le.brightness(), 0), FadePolicyProportionalBrightness);
        mb.uploadOnOff(false);
        mb.uploadBrightnessPercent(0);
        return;
    }

    if (BatteryPolicy::instance().isLowBattery())
        BatteryPolicy::instance().onTryTurnOn();

    if (!BatteryPolicy::instance().allowUserLightControl())
    {
        SILABS_LOG("[ButtonPolicy] short press blocked: power protect");
        return;
    }

    uint8_t prevBright = le.brightness();

    if (!le.isOn() || le.brightness() == 0)
        le.updateNormalState(true, LED_BRIGHTNESS_MAX, le.rawColor());
    else if (le.brightness() == LED_BRIGHTNESS_MAX)
        le.updateNormalState(true, LED_BRIGHTNESS_MAX >> 1, le.rawColor());
    else if (le.brightness() == (LED_BRIGHTNESS_MAX >> 1))
        le.updateNormalState(false, 0, le.rawColor());
    else
        le.updateNormalState(false, 0, le.rawColor());

    le.setChangeOrigin(ChangeOriginLocalKey);
    le.saveStateToFlash();
    le.startFadeToCurrent(le.calcFadeMsForBrightnessDelta(prevBright, le.brightness()), FadePolicyProportionalBrightness);

    mb.uploadOnOff(le.isOn());
    mb.uploadBrightnessPercent(static_cast<uint8_t>((static_cast<uint16_t>(le.brightness()) + 1) * 100 >> 8));

    SILABS_LOG("[ButtonPolicy] cycle result on=%d bright=%u fade=%ums", le.isOn(), le.brightness(),
               le.calcFadeMsForBrightnessDelta(prevBright, le.brightness()));

    if (le.isOn())
        CommissioningManager::instance().onLightTurnedOn();
}

/** @brief 双击：循环颜色库并上报 Matter 色温 */
void ButtonPolicy::onDoublePress()
{
    LightEngine &le = LightEngine::instance();
    if (!le.isOn())
    {
        SILABS_LOG("[ButtonPolicy] double press ignored: light off");
        return;
    }

    if (BatteryPolicy::instance().isLowBattery())
        BatteryPolicy::instance().onTryTurnOn();
    if (!BatteryPolicy::instance().allowUserLightControl())
        return;

    uint8_t next = ColorLibrary::instance().nextCycleIndex(le.colorCycleIndex());
    uint8_t libId = ColorLibrary::instance().libraryIdAtCycleIndex(next);
    SILABS_LOG("[ButtonPolicy] color cycle idx=%u libId=%u", next, libId);

    le.setColorFromCycleIndex(next, LED_FADE_COLOR_SWITCH_MS);
    le.setChangeOrigin(ChangeOriginLocalKey);
    le.saveStateToFlash();

    MatterBridge::instance().uploadOnOff(le.isOn());
    MatterBridge::instance().uploadBrightnessPercent(static_cast<uint8_t>((static_cast<uint16_t>(le.brightness()) + 1) * 100 >> 8));
    MatterBridge::instance().uploadColorFromCycleIndex(next);
}
