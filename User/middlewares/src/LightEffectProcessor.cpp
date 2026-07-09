/**
 * @file LightEffectProcessor.cpp
 * @brief 灯效原子算子实现
 * @author hady
 * @date 2026-06-15
 * @layer Middlewares
 */

#include "LightEffectProcessor.h"
#include <math.h>

/* cubic-bezier(0.45,0,0.55,1) 曲线预计算表，80 分段，范围 0~4096 */
/* 呼吸80点 */
const uint16_t cubic_bezier045_0_055_1_80bytes_buff[80] = {
    0U,    52U,   104U,  156U,  207U,  259U,  311U,  363U,  415U,  467U,  518U,  570U,  622U,  674U,  726U,  778U,
    829U,  881U,  933U,  985U,  1037U, 1089U, 1140U, 1192U, 1244U, 1296U, 1348U, 1400U, 1451U, 1503U, 1555U, 1607U,
    1659U, 1711U, 1763U, 1815U, 1866U, 1918U, 1970U, 2022U, 2074U, 2126U, 2177U, 2229U, 2281U, 2333U, 2385U, 2437U,
    2488U, 2540U, 2592U, 2644U, 2696U, 2748U, 2800U, 2851U, 2903U, 2955U, 3007U, 3059U, 3111U, 3163U, 3214U, 3266U,
    3318U, 3370U, 3422U, 3474U, 3526U, 3577U, 3629U, 3681U, 3733U, 3785U, 3837U, 3889U, 3940U, 3992U, 4044U, 4096U};

/* cubic-bezier(0.11,0,0.5,0) 曲线预计算表，80 分段，范围 0~4096 */
/* 淡入40点 */
const uint16_t cubic_bezier011_0_05_01_40bytes_buff[40] = {0,    31,   75,   131,  197,  272,  355,  445,  541,  641,
                                                           745,  852,  961,  1073, 1185, 1298, 1412, 1526, 1641, 1755,
                                                           1869, 1983, 2097, 2210, 2323, 2435, 2547, 2657, 2768, 2877,
                                                           2987, 3095, 3203, 3311, 3419, 3526, 3634, 3741, 3849, 4096};
/* 淡入80点 */
const uint16_t cubic_bezier011_0_05_01_80bytes_buff[80] = {
    0,    0,    0,    0,    1,    1,    2,    3,    4,    6,    8,    11,   14,   18,   23,   28,
    34,   41,   48,   57,   66,   77,   88,   101,  115,  130,  146,  164,  182,  203,  224,  247,
    272,  299,  327,  356,  388,  421,  456,  493,  532,  573,  615,  661,  708,  757,  809,  863,
    919,  977,  1038, 1102, 1168, 1237, 1308, 1382, 1459, 1539, 1621, 1706, 1794, 1886, 1980, 2077,
    2178, 2281, 2388, 2499, 2612, 2729, 2850, 2973, 3101, 3232, 3366, 3505, 3647, 3793, 3942, 4096};

/* cubic-bezier(0.5,1,0.89,1) 曲线预计算表，40 分段，范围 0~4096 */
/* 淡出40点 */
const uint16_t cubic_bezier05_1_089_1_40bytes_buff[40] = {0,    31,   75,   131,  197,  272,  355,  445,  541,  641,
                                                          745,  852,  961,  1073, 1185, 1298, 1412, 1526, 1641, 1755,
                                                          1869, 1983, 2097, 2210, 2323, 2435, 2547, 2657, 2768, 2877,
                                                          2987, 3095, 3203, 3311, 3419, 3526, 3634, 3741, 3849, 4096};
/* 淡出80点 */
const uint16_t cubic_bezier05_1_089_1_80bytes_buff[80] = {
    0,    154,  303,  449,  591,  730,  864,  995,  1123, 1246, 1367, 1484, 1597, 1708, 1815, 1918,
    2019, 2116, 2210, 2302, 2390, 2475, 2557, 2637, 2714, 2788, 2859, 2928, 2994, 3058, 3119, 3177,
    3233, 3287, 3339, 3388, 3435, 3481, 3523, 3564, 3603, 3640, 3675, 3708, 3740, 3769, 3797, 3824,
    3849, 3872, 3893, 3914, 3932, 3950, 3966, 3981, 3995, 4008, 4019, 4030, 4039, 4048, 4055, 4062,
    4068, 4073, 4078, 4082, 4085, 4088, 4090, 4092, 4093, 4094, 4095, 4095, 4096, 4096, 4096, 4096};

/**
 * @brief 糅合输入输出范围的通用混合算子
 * @param start 起始值
 * @param end   目标值
 * @param factor 预计算表输出的缩放因子 (0~4096)
 * @return 当前时刻的插值结果
 */
uint32_t LightEffectProcessor::blendingOperator(uint32_t start, uint32_t end, uint32_t factor)
{
    uint32_t delta;
    uint32_t mixed;
    if (end >= start)
    {
        /* 正向调光演进：从暗到亮呼吸 */
        delta = static_cast<uint32_t>(end - start);
        mixed = (factor * delta) + (static_cast<uint32_t>(start) * kMaxFactor);
    }
    else
    {
        /* 逆向调光演进：从亮到暗呼吸 */
        delta = static_cast<uint32_t>(start - end);
        mixed = (static_cast<uint32_t>(start) * kMaxFactor) - (factor * delta);
    }
    return mixed;
}

/**
 * @brief 计算呼吸灯因子 糅合输入输出范围
 * @param start 起始亮度/颜色值
 * @param end   目标亮度/颜色值
 * @param elapsedMs 当前效果运行的时间
 * @param periodMs  呼吸周期时长
 * @return 缩放因子 (0=灭, 4096=最亮)
 * @note 使用预计算的 cubic-bezier(0.45,0,0.55,1)
 * 缓动表，保持高精度平滑曲线，避免实时计算开销。
 */
uint32_t LightEffectProcessor::CalcBreath80BytesFactor(uint32_t start, uint32_t end, uint16_t elapsedMs,
                                                       uint16_t totalMs)
{
    uint32_t factor = 0U;

    // 1. 启动特效时（非高频 Tick 中断）只算一次变量除法
    uint32_t breathStepQ16 = (320U * 65536U) / totalMs;

    // 2. 在 10ms 一次的高频 Tick 中断算子中：
    uint32_t idx = (static_cast<uint32_t>(elapsedMs) * breathStepQ16) >> 16;

    if (idx < 80)
    {
        factor = cubic_bezier045_0_055_1_80bytes_buff[idx];
    }
    else if (idx < 160)
    {
        factor = kMaxFactor;
    }
    else if (idx < 240)
    {
        // 最大索引79
        factor = cubic_bezier045_0_055_1_80bytes_buff[static_cast<uint8_t>(239 - idx)];
    }
    else
    {
        factor = 0U;
    }

    return blendingOperator(start, end, factor);
}

/**
 * @brief 淡入曲线插值计算，使用 cubic-bezier(0.11,0,0.5,0) 预计算表
 * @param start 起始值
 * @param end   目标值
 * @param elapsedMs 已经过去的时间
 * @param totalMs   总的过渡时间
 * @return 当前时刻的插值结果
 */
uint32_t LightEffectProcessor::GetBezier40BytesFactorFadeIn(uint32_t start, uint32_t end, uint16_t elapsedMs,
                                                            uint16_t totalMs)
{
    uint32_t factor = 0U;
    uint8_t  idx    = 0U;
    if (elapsedMs >= totalMs)
    {
        idx = 39;
    }
    else
    {
        // 计算索引
        idx = (elapsedMs * 40) / totalMs;
        if (idx >= 1)
        {
            idx -= 1;
        }
    }
    factor = cubic_bezier011_0_05_01_40bytes_buff[idx];
    // 映射到索引 0-39
    return blendingOperator(start, end, factor);
}

/**
 * @brief 淡入曲线插值计算，使用 cubic-bezier(0.11,0,0.5,0) 预计算表
 * @param start 起始值
 * @param end   目标值
 * @param elapsedMs 已经过去的时间
 * @param totalMs   总的过渡时间
 * @return 当前时刻的插值结果
 */
uint32_t LightEffectProcessor::GetBezier80BytesFactorFadeIn(uint32_t start, uint32_t end, uint16_t elapsedMs,
                                                            uint16_t totalMs)
{
    uint32_t factor = 0U;
    uint8_t  idx    = 0U;
    if (elapsedMs >= totalMs)
    {
        idx = 79;
    }
    else
    {
        // 计算索引
        idx = (elapsedMs * 80) / totalMs;
        if (idx >= 1)
        {
            idx -= 1;
        }
    }
    factor = cubic_bezier011_0_05_01_80bytes_buff[idx];
    // 映射到索引 0-79
    return blendingOperator(start, end, factor);
}

/**
 * @brief 淡出曲线插值计算，使用 cubic-bezier(0.5,1,0.89,1) 预计算表
 * @param start 起始亮度/颜色值
 * @param end   目标亮度/颜色值
 * @param elapsedMs 已经过去的时间
 * @param totalMs   总的过渡时间
 * @return 当前时刻的插值结果
 * @note 使用预计算的 cubic-bezier(0.5,1,0.89,1)
 * 缓动表，保持高精度平滑曲线，避免实时计算开销。
 */
uint32_t LightEffectProcessor::GetBezier40BytesFactorFadeOut(uint32_t start, uint32_t end, uint16_t elapsedMs,
                                                             uint16_t totalMs)
{
    uint32_t factor = 0U;
    uint8_t  idx    = 0U;
    if (elapsedMs >= totalMs)
    {
        idx = 39;
    }
    else
    {
        // 计算索引
        idx = (elapsedMs * 40) / totalMs;
        if (idx >= 1)
        {
            idx -= 1;
        }
    }
    factor = cubic_bezier05_1_089_1_40bytes_buff[idx];
    // 映射到索引 0-39
    return blendingOperator(start, end, factor);
}
/**
 * @brief 淡出曲线插值计算，使用 cubic-bezier(0.5,1,0.89,1) 预计算表
 * @param start 起始亮度/颜色值
 * @param end   目标亮度/颜色值
 * @param elapsedMs 已经过去的时间
 * @param totalMs   总的过渡时间
 * @return 当前时刻的插值结果
 * @note 使用预计算的 cubic-bezier(0.5,1,0.89,1)
 * 缓动表，保持高精度平滑曲线，避免实时计算开销。
 */
uint32_t LightEffectProcessor::GetBezier80BytesFactorFadeOut(uint32_t start, uint32_t end, uint16_t elapsedMs,
                                                             uint16_t totalMs)
{
    uint32_t factor = 0U;
    uint8_t  idx    = 0U;
    if (elapsedMs >= totalMs)
    {
        idx = 79;
    }
    else
    {
        // 计算索引
        idx = (elapsedMs * 80) / totalMs;
        if (idx >= 1)
        {
            idx -= 1;
        }
    }
    factor = cubic_bezier05_1_089_1_80bytes_buff[idx];
    // 映射到索引 0-79
    return blendingOperator(start, end, factor);
}

/**
 * @brief 线性插值计算 用于淡变平滑过渡
 * @param start 起始亮度/颜色值
 * @param end   目标亮度/颜色值
 * @param elapsedMs 已经过去的时间
 * @param totalMs   总的过渡时间
 * @return 当前时刻的插值结果
 */
uint32_t LightEffectProcessor::GetLerp(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs)
{
    uint32_t factor = 0U;

    factor = (elapsedMs * kMaxFactor) / totalMs;

    return blendingOperator(start, end, factor);
}

/**
 * @brief 占空比闪烁算子
 * @note 在目标颜色与全灭（0）之间周期性进行二值化状态翻转
 * @param start 起始占空比
 * @param end   目标占空比
 * @param elapsedMs 已经过去的时间
 * @param totalMs   总的闪烁周期时间
 */
uint32_t LightEffectProcessor::GetBlink(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs)
{
    uint32_t factor = 0U;
    if (elapsedMs < (totalMs >> 1))
    {
        factor = kMaxFactor;
    }
    else
    {
        factor = 0;
    }

    return blendingOperator(0, end, factor);
}

/**
 * @brief 保持静态快照的占空比输出，直到超时自动释放
 * @param start 起始占空比
 * @param end   目标占空比
 * @param elapsedMs 已经过去的时间
 * @param totalMs   总的保持时间
 */
uint32_t LightEffectProcessor::GetKeep(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs)
{
    return blendingOperator(start, end, kMaxFactor);
}
