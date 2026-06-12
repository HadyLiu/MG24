/**
 * @file IndicatorService.cpp
 * @brief 指示灯服务实现
 * @layer Service
 */
#include "IndicatorService.h"
#include "AppConfig.h"

/** @brief 获取 IndicatorService 单例 */
IndicatorService &IndicatorService::instance()
{
    static IndicatorService s_svc;
    return s_svc;
}

/** @brief 10ms 轮询白呼吸与红灯闪烁 */
void IndicatorService::tick10ms()
{
    pollWhiteBreath();
    dispatchRedBlink();
}

/** @brief 白呼吸 tick */
void IndicatorService::pollWhiteBreath()
{
    Indic_W_Breath_Poll_10ms();
}

/** @brief 红灯闪烁调度与 tick */
void IndicatorService::dispatchRedBlink()
{
    Indic_Red_Blink_Control_Dispatch();
    Indic_Red_Blink_Poll_10ms();
}

/** @brief 启动单速红灯闪烁 */
void IndicatorService::startRedBlinkNormal(uint16_t periodMs, uint16_t count)
{
    SILABS_LOG("[Indicator] red blink normal period=%u count=%u", periodMs, count);
    blink_normal_cfg_t cfg = {periodMs, count};
    Indic_Red_Blink_Normal_Flag_Set(true, &cfg);
}

/** @brief 启动双速混合红灯闪烁（恢复出厂预警） */
void IndicatorService::startRedBlinkMixed(uint16_t m1Period, uint16_t m1Count, uint16_t m2Period, uint16_t m2Count)
{
    SILABS_LOG("[Indicator] red blink mixed m1=%u/%u m2=%u/%u", m1Period, m1Count, m2Period, m2Count);
    blink_mixed_cfg_t cfg = {m1Period, m1Count, m2Period, m2Count};
    Indic_Red_Blink_Mixed_Flag_Set(true, &cfg);
}

/** @brief 停止所有红灯闪烁 */
void IndicatorService::stopRedBlink()
{
    Indic_Red_Blink_Stop();
    Indic_Red_Blink_Normal_Flag_Set(false, nullptr);
}

/** @brief 启动白呼吸（充电等场景，brightness 为百分比） */
void IndicatorService::startWhiteBreath(uint8_t brightnessPercent)
{
    SILABS_LOG("[Indicator] white breath start bright=%u%%", brightnessPercent);
    Indic_W_Breath_Start(brightnessPercent);
}

/** @brief 停止白呼吸 */
void IndicatorService::stopWhiteBreath()
{
    Indic_W_Breath_Stop();
}
