/**
 * @file ColorConverter.h
 * @brief 颜色转换服务：CT/HSV/XY → WRGB（封装 led_mode_convert）
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 纯静态转换工具，无状态；输出为 0~1023 的 WRGB 原始分辨率。
 */
#pragma once

#include "app/LightTypes.h"
#include <stdint.h>

/** @brief Matter 色彩空间到本地 WRGB 的转换器 */
class ColorConverter
{
public:
  /**
   * @brief 色温(K) → WRGB
   * @param kelvin 色温（2200~6500，超出限幅）
   * @return WRGB（0~1023）
   */
  static WrgbColor fromColorTemperature(uint32_t kelvin);

  /**
   * @brief HSV → WRGB（W=0）
   * @param hue        色相 0~254
   * @param saturation 饱和度 0~254
   * @param value      明度 0~254
   * @return WRGB（0~1023）
   */
  static WrgbColor fromHsv(uint8_t hue, uint8_t saturation, uint8_t value);

  /**
   * @brief CIE XY → WRGB（W=0）
   * @param x CIE x（0~65535）
   * @param y CIE y（0~65535）
   * @return WRGB（0~1023）
   */
  static WrgbColor fromXy(uint16_t x, uint16_t y);
};
