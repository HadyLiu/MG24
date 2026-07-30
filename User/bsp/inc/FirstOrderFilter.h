/**
 * @file FirstOrderFilter.h
 * @brief 一阶低通（增量式无符号整数），供 ADC 防抖
 * @layer BSP
 * @note 注解：避免单次误采样进入保护；无 FPU / 无负数运算。
 */
#pragma once

#include <cstdint>

namespace bsp
{
/**
 * @brief 一阶低通滤波器（y[n] = y[n-1] + A/256 * (x[n] - y[n-1])）
 */
class FirstOrderFilter
{
  public:
    /**
     * @brief 就地更新滤波输出
     * @param in   当前采样值
     * @param alpha 平滑因子 A，范围 [1, 255]；越大跟踪越快
     * @param pOut 历史输出指针，更新为 y[n]
     */
    static void Apply(uint32_t in, uint8_t alpha, uint32_t* pOut)
    {
        if (pOut == nullptr)
        {
            return;
        }

        if (alpha == 0U)
        {
            *pOut = in;
            return;
        }

        uint32_t temp = 0U;
        if (in < (*pOut))
        {
            temp = (*pOut) - in;
            temp *= alpha;
            temp += 0x80U;
            temp >>= 8U;
            (*pOut) -= temp;
        }
        else if (in > (*pOut))
        {
            temp = in - (*pOut);
            temp *= alpha;
            temp += 0x80U;
            temp >>= 8U;
            (*pOut) += temp;
        }
    }
};
} // namespace bsp
