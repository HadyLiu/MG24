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

namespace {

static constexpr uint32_t kPwmMax1023 = 1023U;

/** @brief Matter CT 混色时 W 占空比（40%） */
static constexpr uint32_t kCtMixedWhiteFactor = (kPwmMax1023 * 40U) / 100U;

}  // namespace

/**
 * @brief 色温(K) → WRGB
 * @param kelvin 色温（2200~6500，超出限幅）
 * @return WRGB（0~1023）
 */
LightTypes::WrgbColor ColorConverter::FromColorTemperature(uint32_t kelvin) {
  LightTypes::WrgbColor c = {0, 0, 0, 0};

  // 1. 边界安全限幅
  if (kelvin < 2200U) {
    kelvin = 2200U;
  }
  if (kelvin > 6500U) {
    kelvin = 6500U;
  }

  // Matter 常用 370 mireds≈2702K，对齐到纯 W 分支（§12）
  if ((kelvin >= 2650U) && (kelvin <= 2750U)) {
    kelvin = 2700U;
  }

  uint32_t r_factor = 0U;
  uint32_t g_factor = 0U;
  uint32_t b_factor = 0U;

  // 2. 分区间计算 RGB 放大因子 (0 ~ 1023)
  if (kelvin == 2700U) {
    /* 2700K：仅 W 通道，RGB 为 0（§12 纯 W → W=100%） */
  } else if (kelvin < 2700U) {
    // 【暖调光区间】2200K ~ 2699K -> W 40% + R + 少许 G
    const uint32_t range_factor = ((2700U - kelvin) * kPwmMax1023) / (2700U - 2200U);

    r_factor = (range_factor * 358U) >> 10;  // 35.0% 红光
    g_factor = (range_factor * 82U) >> 10;   // 8.0% 绿光校准
    b_factor = 0U;
  } else {
    // 【冷调光区间】2701K ~ 6500K -> W 40% + B + G
    const uint32_t range_factor = ((kelvin - 2700U) * kPwmMax1023) / (6500U - 2700U);

    r_factor = 0U;
    b_factor = (range_factor * 563U) >> 10;  // 55.0% 蓝光
    g_factor = (range_factor * 410U) >> 10;  // 40.0% 绿光
  }

  const bool pureWhiteOnly = (r_factor == 0U) && (g_factor == 0U) && (b_factor == 0U);
  if (pureWhiteOnly) {
    c.w = static_cast<uint16_t>(kPwmMax1023);
  } else {
    c.w = static_cast<uint16_t>(kCtMixedWhiteFactor);
  }

  c.r = static_cast<uint16_t>(r_factor);
  c.g = static_cast<uint16_t>(g_factor);
  c.b = static_cast<uint16_t>(b_factor);

  if (c.r >= kPwmMax1023) {
    c.r = static_cast<uint16_t>(kPwmMax1023);
  }
  if (c.g >= kPwmMax1023) {
    c.g = static_cast<uint16_t>(kPwmMax1023);
  }
  if (c.b >= kPwmMax1023) {
    c.b = static_cast<uint16_t>(kPwmMax1023);
  }
  return c;
}

/**
 * @brief WRGB → 色温(K)
 * @param color 输入的 WRGB 颜色结构体（0~1023）
 * @return uint32_t 还原后的色温（2200~6500K）
 * @note 根据正向分段函数的特殊性，通过 R 和 B 通道反向推导 kelvin
 */
uint32_t ColorConverter::ToColorTemperature(const LightTypes::WrgbColor& color) {
  // 如果有蓝光分量，说明落在【冷调光区间】2701K ~ 6500K
  if (color.b > 0) {
    // 正向公式: b_factor = (range_factor * 563) >> 10
    // 反推 range_factor = (b_factor << 10) / 563
    uint32_t range_factor = ((uint32_t)color.b << 10) / 563;
    if (range_factor > kPwmMax1023) {
      range_factor = kPwmMax1023;
    }

    // 正向公式: range_factor = ((kelvin - 2700) * 1023) / (6500 - 2700)
    // 反推 kelvin = 2700 + (range_factor * (6500 - 2700)) / 1023
    uint32_t kelvin = 2700U + (range_factor * (6500U - 2700U)) / kPwmMax1023;

    if (kelvin > 6500) {
      return 6500;
    }
    return kelvin;
  } else if (color.r > 0) {
    // 暖调光区间 2200K ~ 2699K
    // 正向公式: r_f = (range_factor * 358) >> 10
    // 反推 range_factor = (r_f << 10) / 358
    uint32_t range_factor = ((uint32_t)color.r << 10) / 358;
    if (range_factor > kPwmMax1023) {
      range_factor = kPwmMax1023;
    }

    // 正向公式: range_factor = ((2700 - kelvin) * 1023) / (2700 - 2200)
    // 反推 kelvin = 2700 - (range_factor * (2700 - 2200)) / 1023
    uint32_t kelvin = 2700U - (range_factor * (2700U - 2200U)) / kPwmMax1023;

    if (kelvin < 2200) {
      return 2200;
    }
    return kelvin;
  }

  // 默认基准点
  return 2700;
}

/**
 * @brief HSV → WRGB（W=0）
 * @param hsv 输入的 HSV 结构体 (H: 0~254, S: 0~254, V: 0~254)
 * @return WRGB（0~1023）
 */
LightTypes::WrgbColor ColorConverter::FromHsv(LightTypes::HsvColor hsv) {
  LightTypes::WrgbColor c = {0, 0, 0, 0};

  if (hsv.s == 0) {
    c.r = c.g = c.b = hsv.v;
    return c;
  }

  // 0-254 的 H 映射到 6 个扇区 (255 / 6 = 42.5)
  // 为了高效率，直接用查表或固定步长
  uint8_t region = hsv.h / 43;
  if (region > 5) {
    region = 5;
  }
  // 计算当前扇区内的残留分量 (等价于 f)
  uint32_t remainder = (hsv.h % 43) * 6;  // 放大系数以匹配精度

  uint8_t p = (hsv.v * (254 - hsv.s)) >> 8;
  uint8_t q = (hsv.v * (254 - ((hsv.s * remainder) >> 8))) >> 8;
  uint8_t t = (hsv.v * (254 - ((hsv.s * (254 - remainder)) >> 8))) >> 8;

  uint16_t r, g, b;

  switch (region) {
    case 0:
      r = hsv.v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = hsv.v;
      b = p;
      break;
    case 2:
      r = p;
      g = hsv.v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = hsv.v;
      break;
    case 4:
      r = t;
      g = p;
      b = hsv.v;
      break;
    default:
      r = hsv.v;
      g = p;
      b = q;
      break;
  }

  c.r = r << 2;
  c.g = g << 2;
  c.b = b << 2;

  return c;
}

/**
 * @brief WRGB → HSV
 * @param color 局限于 RGB 分量的颜色结构体（忽略 W 通道）
 * @return LightTypes::HsvColor 还原后的 HSV 结构体 (H: 0~254, S: 0~254, V:
 * 0~254)
 */
LightTypes::HsvColor ColorConverter::ToHsv(const LightTypes::WrgbColor& color) {
  LightTypes::HsvColor hsv = {0, 0, 0};

  // 将 10位 变换回 8位 匹配你原算法的输入基准
  uint8_t r = (uint8_t)(color.r >> 2);
  uint8_t g = (uint8_t)(color.g >> 2);
  uint8_t b = (uint8_t)(color.b >> 2);

  uint8_t max_val = r;
  if (g > max_val) {
    max_val = g;
  }
  if (b > max_val) {
    max_val = b;
  }

  uint8_t min_val = r;
  if (g < min_val) {
    min_val = g;
  }
  if (b < min_val) {
    min_val = b;
  }

  // 1. 计算 Value (明度)
  hsv.v = max_val;
  if (hsv.v == 0) {
    hsv.h = 0;
    hsv.s = 0;
    return hsv;
  }

  // 2. 计算 Saturation (饱和度)
  uint32_t delta = max_val - min_val;
  if (delta == 0) {
    hsv.h = 0;
    hsv.s = 0;
    return hsv;
  }
  hsv.s = (uint8_t)((delta * 254) / max_val);

  // 3. 计算 Hue (色相)
  int32_t h_deg = 0;
  if (max_val == r) {
    h_deg = ((int32_t)(g - b) * 43) / (int32_t)delta;
  } else if (max_val == g) {
    h_deg = 86 + ((int32_t)(b - r) * 43) / (int32_t)delta;
  } else {
    h_deg = 172 + ((int32_t)(r - g) * 43) / (int32_t)delta;
  }

  if (h_deg < 0) {
    h_deg += 254;
  }

  hsv.h = (uint8_t)h_deg;
  if (hsv.h > 254) {
    hsv.h = 254;
  }

  return hsv;
}

/**
 * @brief CIE XY → WRGB（W=0）
 * @param xy 输入的 CIE XY 结构体（0~65535）
 * @return WRGB（0~1023）
 */
LightTypes::WrgbColor ColorConverter::FromXy(LightTypes::XyColor xy) {
  LightTypes::WrgbColor c = {0, 0, 0, 0};
  // 1. 将 0-65535 的整数还原为 0.0 - 1.0 的浮点相对坐标
  float x = (float)xy.x / 65535.0f;
  float y = (float)xy.y / 65535.0f;

  // 安全保护：防止除以 0 导致芯片死机
  if (y < 0.001f) {
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
  if (r_f < 0.0f) {
    r_f = 0.0f;
  } else if (r_f > 1.0f) {
    r_f = 1.0f;
  }
  if (g_f < 0.0f) {
    g_f = 0.0f;
  } else if (g_f > 1.0f) {
    g_f = 1.0f;
  }
  if (b_f < 0.0f) {
    b_f = 0.0f;
  } else if (b_f > 1.0f) {
    b_f = 1.0f;
  }

  // 5. 最终输出传递给 PWM 驱动的 10位 RGB 值
  // 从 0-255 映射到 0-1023
  c.r = ((uint16_t)(r_f * 1023.0f));
  c.g = ((uint16_t)(g_f * 1023.0f));
  c.b = ((uint16_t)(b_f * 1023.0f));
  return c;
}

/**
 * @brief WRGB → CIE XY
 * @param color 输入的 WRGB 颜色结构体（忽略 W 通道）
 * @return XyColor 还原后的 CIE xy 结构体（0~65535）
 * @note 通过正向 XYZ->RGB 矩阵的逆矩阵进行推导，保持物理层对齐
 */
LightTypes::XyColor ColorConverter::ToXy(const LightTypes::WrgbColor& color) {
  LightTypes::XyColor xy = {0, 0};

  // 1. 将 10 位整型还原为 0.0f - 1.0f 的相对浮点数
  float r_f = (float)color.r / 1023.0f;
  float g_f = (float)color.g / 1023.0f;
  float b_f = (float)color.b / 1023.0f;

  // 2. 使用正向矩阵的逆矩阵推导大写 XYZ
  // 正向矩阵：
  // [ R ]   [  3.2404  -1.5371  -0.4985 ]   [ X ]
  // [ G ] = [ -0.9692   1.8760   0.0415 ] * [ Y ]
  // [ B ]   [  0.0556  -0.2040   1.0572 ]   [ Z ]
  // 逆矩阵推导结果如下：
  float X = 0.4124f * r_f + 0.3576f * g_f + 0.1805f * b_f;
  float Y = 0.2126f * r_f + 0.7152f * g_f + 0.0722f * b_f;
  float Z = 0.0193f * r_f + 0.1192f * g_f + 0.9505f * b_f;

  // 3. 计算从大写 XYZ 到小写 xy 坐标的转换
  float sum = X + Y + Z;
  if (sum < 0.00001f) {
    // 规避全黑状态下的除零风险，返回中心白点 D65 的近似值
    xy.x = (uint16_t)(0.3127f * 65535.0f);
    xy.y = (uint16_t)(0.3290f * 65535.0f);
    return xy;
  }

  float x_f = X / sum;
  float y_f = Y / sum;

  // 4. 限幅与 16 位整数定点化映射 (0~65535)
  if (x_f < 0.0f) {
    x_f = 0.0f;
  }
  if (x_f > 1.0f) {
    x_f = 1.0f;
  }
  if (y_f < 0.0f) {
    y_f = 0.0f;
  }
  if (y_f > 1.0f) {
    y_f = 1.0f;
  }

  xy.x = (uint16_t)(x_f * 65535.0f);
  xy.y = (uint16_t)(y_f * 65535.0f);

  return xy;
}
