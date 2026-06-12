/**
 * @file ColorConverter.h
 * @brief 颜色转换服务：CT/HSV/XY → WRGB（封装 led_mode_convert）
 * @layer Service
 */
#pragma once

#include "app/LightTypes.h"
#include <stdint.h>

/** @brief Matter 色彩空间到本地 WRGB 的转换器 */
class ColorConverter
{
public:
    static WrgbColor fromColorTemperature(uint32_t kelvin);
    static WrgbColor fromHsv(uint8_t hue, uint8_t saturation, uint8_t value);
    static WrgbColor fromXy(uint16_t x, uint16_t y);
};
