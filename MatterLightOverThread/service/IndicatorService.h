/**
 * @file IndicatorService.h
 * @brief 指示灯服务：红灯闪烁与白呼吸的统一 API
 * @layer Service
 */
#pragma once

#include "../driver/led_red_indic.h"
#include "../driver/led_white_indic.h"
#include <stdint.h>

/** @brief 系统指示灯（红/白）特效服务 */
class IndicatorService
{
public:
    static IndicatorService &instance();

    void tick10ms();
    void pollWhiteBreath();
    void dispatchRedBlink();

    void startRedBlinkNormal(uint16_t periodMs, uint16_t count);
    void startRedBlinkMixed(uint16_t m1Period, uint16_t m1Count, uint16_t m2Period, uint16_t m2Count);
    void stopRedBlink();
    void startWhiteBreath(uint8_t brightnessPercent);
    void stopWhiteBreath();
};
