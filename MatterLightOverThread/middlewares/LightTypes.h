#pragma once

#include <stdint.h>
class LightTypes
{
public:
  /* WRGB 颜色结构体：每通道 16 位，范围 0~1023 */
  typedef struct
  {
    uint16_t w; ///< 暖白通道
    uint16_t r; ///< 红通道
    uint16_t g; ///< 绿通道
    uint16_t b; ///< 蓝通道
  } WrgbColor;

  /* CIE XY 坐标结构体（16位定点化，0~65535）*/
  struct XyColor
  {
    uint16_t x;
    uint16_t y;
  };

  /* HSV 颜色结构体（8位，0~254)*/
  struct HsvColor
  {
    uint8_t h;
    uint8_t s;
    uint8_t v;
  };
};