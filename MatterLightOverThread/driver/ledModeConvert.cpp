#include "ledModeConvert.h"
/**
 * @brief  智能照明专用：HSV 转 RGB（输入范围 0-254/255）
 * @param  h: 色调 (0 ~ 254) -> 对应 0~360度
 * @param  s: 饱和度 (0 ~ 254) -> 对应 0~100%
 * @param  v: 明度 (0 ~ 254) -> 通常传全局亮度，或直接填 254
 * @out    r, g, b: 输出驱动值 (0 ~ 255)
 */
void LedDriver_ConvertHsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s == 0)
    {
        *r = *g = *b = v;
        return;
    }

    // 0-254 的 H 映射到 6 个扇区 (255 / 6 = 42.5)
    // 为了高效率，直接用查表或固定步长
    uint8_t region = h / 43;
    if (region > 5)
        region = 5;

    // 计算当前扇区内的残留分量 (等价于 f)
    uint32_t remainder = (h % 43) * 6; // 放大系数以匹配精度

    uint8_t p = (v * (254 - s)) / 254;
    uint8_t q = (v * (254 - ((s * remainder) >> 8))) / 254;
    uint8_t t = (v * (254 - ((s * (254 - remainder)) >> 8))) / 254;

    switch (region)
    {
    case 0:
        *r = v;
        *g = t;
        *b = p;
        break;
    case 1:
        *r = q;
        *g = v;
        *b = p;
        break;
    case 2:
        *r = p;
        *g = v;
        *b = t;
        break;
    case 3:
        *r = p;
        *g = q;
        *b = v;
        break;
    case 4:
        *r = t;
        *g = p;
        *b = v;
        break;
    default:
        *r = v;
        *g = p;
        *b = q;
        break;
    }
}

/**
 * @brief 纯整数 WRGB 色温基准 PWM 计算（最大输出 255）
 * @param kelvin 输入绝对色温 (2200 ~ 6500)
 * @param out_w  输出 W 通道基准 PWM (0 ~ 255)
 * @param out_r  输出 R 通道基准 PWM (0 ~ 255)
 * @param out_g  输出 G 通道基准 PWM (0 ~ 255)
 * @param out_b  输出 B 通道基准 PWM (0 ~ 255)
 */
void Light_Calc_CT_To_WRGB(uint32_t kelvin, uint8_t *out_w, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b)
{
#define PWM_MAX_255 255  // 硬件最大PWM值 (8位)
#define SCALE_10BIT 1024 // 内部高精度计算基底 (2^10)
    // 1. 边界安全限幅
    if (kelvin < 2200)
    {
        kelvin = 2200;
    }
    if (kelvin > 6500)
    {
        kelvin = 6500;
    }

    uint32_t w_factor = SCALE_10BIT; // 1024
    uint32_t r_factor = 0;
    uint32_t g_factor = 0;
    uint32_t b_factor = 0;

    // 2. 分区间计算各个通道的放大因子 (0 ~ 1024)
    if (kelvin == 2700)
    {
        w_factor = SCALE_10BIT;
    }
    else if (kelvin < 2700)
    {
        // 【暖调光区间】2200K ~ 2699K -> W全开 + R + 少许 G
        uint32_t range_factor = ((2700 - kelvin) * SCALE_10BIT) / (2700 - 2200);

        w_factor = SCALE_10BIT;
        r_factor = (range_factor * 358) >> 10; // 35.0% 红光
        g_factor = (range_factor * 82) >> 10;  // 8.0% 绿光校准
        b_factor = 0;
    }
    else
    {
        // 【冷调光区间】2701K ~ 6500K -> W渐减 + B + G
        uint32_t range_factor = ((kelvin - 2700) * SCALE_10BIT) / (6500 - 2700);

        w_factor = SCALE_10BIT - ((range_factor * 205) >> 10); // W 渐减至 80%
        r_factor = 0;
        b_factor = (range_factor * 563) >> 10; // 55.0% 蓝光
        g_factor = (range_factor * 410) >> 10; // 40.0% 绿光
    }

    // 3. 将 1024 基底精确定位到 0 ~ 255 空间
    // 数学公式：实际输出 = (factor * 255) / 1024，利用 >> 10 代替除法
    *out_w = (uint8_t)((w_factor * PWM_MAX_255) >> 10);
    *out_r = (uint8_t)((r_factor * PWM_MAX_255) >> 10);
    *out_g = (uint8_t)((g_factor * PWM_MAX_255) >> 10);
    *out_b = (uint8_t)((b_factor * PWM_MAX_255) >> 10);

    // 4. 严谨性限幅保护
    if (*out_w > PWM_MAX_255)
        *out_w = PWM_MAX_255;
    if (*out_r > PWM_MAX_255)
        *out_r = PWM_MAX_255;
    if (*out_g > PWM_MAX_255)
        *out_g = PWM_MAX_255;
    if (*out_b > PWM_MAX_255)
        *out_b = PWM_MAX_255;
}

void MyCalculatedRGB(uint16_t chipX, uint16_t chipY, uint8_t *outR, uint8_t *outG, uint8_t *outB)
{
    // 1. 还原为标准的 0.0 ~ 1.0 的 CIE 坐标
    float x = (float)chipX / 65536.0f;
    float y = (float)chipY / 65536.0f;
    float z = 1.0f - x - y;

    // 2. 这里的 Y 顺应亮度，暂设为 1.0 满亮度
    float Y = 1.0f;
    float X = (y > 0) ? (x * Y) / y : 0;
    float Z = (y > 0) ? (z * Y) / y : 0;

    // 3. 使用标准的 sRGB 逆转换矩阵（无视官方指示灯色域）
    float r = 3.2406f * X - 1.5372f * Y - 0.4986f * Z;
    float g = -0.9689f * X + 1.8758f * Y + 0.0415f * Z;
    float b = 0.0557f * X - 0.2040f * Y + 1.0570f * Z;

    // 4. 约束并放大到 0-255 硬件空间
    int R = (r < 0) ? 0 : ((r > 1) ? 255 : (int)(r * 255));
    int G = (g < 0) ? 0 : ((g > 1) ? 255 : (int)(g * 255));
    int B = (b < 0) ? 0 : ((b > 1) ? 255 : (int)(b * 255));
}