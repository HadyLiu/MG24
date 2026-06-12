/**
 * @file ColorConverter.cpp
 * @brief 颜色转换服务实现
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 纯静态无状态转换；输出 0~1023 WRGB 原始分辨率。
 */
#include "ColorConverter.h"
#include <stdint.h>

/** @brief 色温(K) → WRGB 0 ~ 1023 */
WrgbColor ColorConverter::fromColorTemperature(uint32_t kelvin)
{
  WrgbColor c = {0, 0, 0, 0};

#define PWM_MAX_1023 1023 // 硬件最大PWM值 (10位)
#define SCALE_10BIT 1023  // 内部高精度计算基底 (2^10)
  // 1. 边界安全限幅
  if (kelvin < 2200)
  {
    kelvin = 2200;
  }
  if (kelvin > 6500)
  {
    kelvin = 6500;
  }

  uint32_t w_factor = PWM_MAX_1023;
  uint32_t r_factor = 0;
  uint32_t g_factor = 0;
  uint32_t b_factor = 0;

  // 2. 分区间计算各个通道的放大因子 (0 ~ 1023)
  if (kelvin == 2700)
  {
    w_factor = PWM_MAX_1023;
  }
  else if (kelvin < 2700)
  {
    // 【暖调光区间】2200K ~ 2699K -> W全开 + R + 少许 G
    uint32_t range_factor = ((2700 - kelvin) * PWM_MAX_1023) / (2700 - 2200);

    w_factor = PWM_MAX_1023;
    r_factor = (range_factor * 358) >> 10; // 35.0% 红光
    g_factor = (range_factor * 82) >> 10;  // 8.0% 绿光校准
    b_factor = 0;
  }
  else
  {
    // 【冷调光区间】2701K ~ 6500K -> W渐减 + B + G
    uint32_t range_factor = ((kelvin - 2700) * PWM_MAX_1023) / (6500 - 2700);

    w_factor = PWM_MAX_1023 - ((range_factor * 205) >> 10); // W 渐减至 80%
    r_factor = 0;
    b_factor = (range_factor * 563) >> 10; // 55.0% 蓝光
    g_factor = (range_factor * 410) >> 10; // 40.0% 绿光
  }

  // 3. 将 1023 基底精确定位到 0 ~  空间
  // 数学公式：实际输出 = (factor * 255) / 1023，利用 >> 8 代替除法
  c.w = (uint16_t)(w_factor);
  c.r = (uint16_t)(r_factor);
  c.g = (uint16_t)(g_factor);
  c.b = (uint16_t)(b_factor);

  // 4. 严谨性限幅保护
  if (c.w >= PWM_MAX_1023)
  {
    c.w = PWM_MAX_1023;
  }
  if (c.r >= PWM_MAX_1023)
  {
    c.r = PWM_MAX_1023;
  }
  if (c.g >= PWM_MAX_1023)
  {
    c.g = PWM_MAX_1023;
  }
  if (c.b >= PWM_MAX_1023)
  {
    c.b = PWM_MAX_1023;
  }
  return c;
}

/** @brief HSV → RGB（W=0） 0-1023 */
WrgbColor ColorConverter::fromHsv(uint8_t hue, uint8_t saturation,
                                  uint8_t value)
{
  WrgbColor c = {0, 0, 0, 0};

  if (saturation == 0)
  {
    c.r = c.g = c.b = value;
    return c;
  }

  // 0-254 的 H 映射到 6 个扇区 (255 / 6 = 42.5)
  // 为了高效率，直接用查表或固定步长
  uint8_t region = hue / 43;
  if (region > 5)
  {
    region = 5;
  }
  // 计算当前扇区内的残留分量 (等价于 f)
  uint32_t remainder = (hue % 43) * 6; // 放大系数以匹配精度

  uint8_t p = (value * (254 - saturation)) >> 8;
  uint8_t q = (value * (254 - ((saturation * remainder) >> 8))) >> 8;
  uint8_t t = (value * (254 - ((saturation * (254 - remainder)) >> 8))) >> 8;

  uint16_t r, g, b;

  switch (region)
  {
  case 0:
    r = value;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = value;
    b = p;
    break;
  case 2:
    r = p;
    g = value;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = value;
    break;
  case 4:
    r = t;
    g = p;
    b = value;
    break;
  default:
    r = value;
    g = p;
    b = q;
    break;
  }

  c.r = r << 2;
  c.g = g << 2;
  c.b = b << 2;

  return c;
}

/** @brief CIE XY → RGB（W=0） 0~1023 */
WrgbColor ColorConverter::fromXy(uint16_t CIExy_x, uint16_t CIExy_y)
{
  WrgbColor c = {0, 0, 0, 0};
  // 1. 将 0-65535 的整数还原为 0.0 - 1.0 的浮点相对坐标
  float x = (float)CIExy_x / 65535.0f;
  float y = (float)CIExy_y / 65535.0f;

  // 安全保护：防止除以 0 导致芯片死机
  if (y < 0.001f)
  {
    y = 0.001f;
  }

  // 2. 依据 CIE 1931 标准计算大写的大写 XYZ 空间
  float z = 1.0f - x - y;
  float X = (x / y);
  float Y = 1.0f;
  float Z = (z / y);

  // 3. 🎯 【终极精准矩阵】根据你抓到的 3 组黄金物理点对齐推导出的系数
  float r_f = 3.2404f * X - 1.5371f * Y - 0.4985f * Z;
  float g_f = -0.9692f * X + 1.8760f * Y + 0.0415f * Z;
  float b_f = 0.0556f * X - 0.2040f * Y + 1.0572f * Z;

  // 4. 边界保护限制 (防止数学计算溢出导致灯光颜色变异)
  if (r_f < 0.0f)
  {
    r_f = 0.0f;
  }
  else if (r_f > 1.0f)
  {
    r_f = 1.0f;
  }
  if (g_f < 0.0f)
  {
    g_f = 0.0f;
  }
  else if (g_f > 1.0f)
  {
    g_f = 1.0f;
  }
  if (b_f < 0.0f)
  {
    b_f = 0.0f;
  }
  else if (b_f > 1.0f)
  {
    b_f = 1.0f;
  }

  // 5. 最终输出传递给 PWM 驱动的 10位 RGB 值
  // 从 0-255 映射到 0-1023
  c.r = ((uint16_t)(r_f)) << 10;
  c.g = ((uint16_t)(g_f)) << 10;
  c.b = ((uint16_t)(b_f)) << 10;
  return c;
}
