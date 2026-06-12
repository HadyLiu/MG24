/**
 * @file IdentifyEffect.cpp
 * @brief Matter Identify 灯效策略实现
 * @layer Policy
 */
#include "IdentifyEffect.h"
#include "AppConfig.h"

/** @brief 获取 IdentifyEffect 单例 */
IdentifyEffect &IdentifyEffect::instance()
{
    static IdentifyEffect s_effect;
    return s_effect;
}

/** @brief Identify 开始：保存快照，400/400ms 闪烁 2 次 */
void IdentifyEffect::onStart()
{
    SILABS_LOG("[Identify] start blink");
    m_saved = LightEngine::instance().captureSnapshot();
    m_active = true;
    LightEngine::instance().setBlink(m_saved.brightness, m_saved.raw_color, 800, 2);
}

/** @brief Identify 结束：800ms EaseOut 恢复原状态 */
void IdentifyEffect::onStop()
{
    if (!m_active)
        return;
    SILABS_LOG("[Identify] stop, restore snapshot");
    m_active = false;
    LightEngine::instance().stopEffect();
    LightEngine::instance().restoreSnapshot(m_saved, 800);
}
