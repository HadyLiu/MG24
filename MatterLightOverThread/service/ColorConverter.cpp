/**
 * @file ColorConverter.cpp
 * @brief 颜色转换服务实现
 * @layer Service
 */
#include "ColorConverter.h"
#include "../driver/led_mode_convert.h"

/** @brief 色温(K) → WRGB */
WrgbColor ColorConverter::fromColorTemperature(uint32_t kelvin)
{
    WrgbColor c = {0, 0, 0, 0};
    Light_Calc_CT_To_WRGB(kelvin, &c.w, &c.r, &c.g, &c.b);
    return c;
}

/** @brief HSV → RGB（W=0） */
WrgbColor ColorConverter::fromHsv(uint8_t hue, uint8_t saturation, uint8_t value)
{
    WrgbColor c = {0, 0, 0, 0};
    LedDriver_ConvertHsvToRgb(hue, saturation, value, &c.r, &c.g, &c.b);
    return c;
}

/** @brief CIE XY → RGB（W=0） */
WrgbColor ColorConverter::fromXy(uint16_t x, uint16_t y)
{
    WrgbColor c = {0, 0, 0, 0};
    Light_Calc_XY_To_RGB(x, y, &c.r, &c.g, &c.b);
    return c;
}
