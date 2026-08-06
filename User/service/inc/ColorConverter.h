/**
 * @file ColorConverter.h
 * @brief 颜色转换服务：CT/HSV/XY → WRGB（封装 led_mode_convert）
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 纯静态转换工具，无状态；输出为 0~1023 的 WRGB 原始分辨率。
 */
#pragma once

#include "LightTypes.h"
#include <stdint.h>

/** @brief Matter 色彩空间到本地 WRGB 的转换器 */
class ColorConverter
{
  public:
    /**
     * @brief 色温(K) → WRGB（Matter CT 下行专用）
     * @param kelvin 色温（2200~6500，超出限幅）
     * @return WRGB（0~1023）；纯 W（2700K）时 W=100%；W+RGB 混色时 W=40%，亮度不衰减
     */
    static LightTypes::WrgbColor FromColorTemperature(uint32_t kelvin);

    /**
     * @brief WRGB → 色温(K)
     * @param color 输入的 WRGB 颜色结构体（0~1023）
     * @return uint32_t 还原后的色温（2200~6500K）
     * @note 根据正向分段函数的特殊性，通过 R 和 B 通道反向推导 kelvin
     */
    static uint32_t ToColorTemperature(const LightTypes::WrgbColor& color);

    /**
     * @brief HSV → WRGB（W=0）
     * @param hsv 输入的 HSV 结构体 (H: 0~254, S: 0~254, V: 0~254)
     * @return WRGB（0~1023）
     */
    static LightTypes::WrgbColor FromHsv(LightTypes::HsvColor hsv);

    /**
     * @brief WRGB → HSV
     * @param color 局限于 RGB 分量的颜色结构体（忽略 W 通道）
     * @return HsvColor 还原后的 HSV 结构体 (H: 0~254, S: 0~254, V: 0~254)
     */
    static LightTypes::HsvColor ToHsv(const LightTypes::WrgbColor& color);

    /**
     * @brief CIE XY → WRGB（W=0）
     * @param xy 输入的 CIE XY 结构体（0~65535）
     * @return WRGB（0~1023）
     */
    static LightTypes::WrgbColor FromXy(LightTypes::XyColor xy);

    /**
     * @brief WRGB → CIE XY
     * @param color 输入的 WRGB 颜色结构体（0~1023）
     * @return LightTypes::XyColor 还原后的 CIE XY 结构体（16位定点化，0~65535）
     * @note 根据正向分段函数的特殊性，通过 RGB 分量反向推导 x、y
     */
    static LightTypes::XyColor ToXy(const LightTypes::WrgbColor& color);
};
