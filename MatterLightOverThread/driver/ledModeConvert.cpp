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