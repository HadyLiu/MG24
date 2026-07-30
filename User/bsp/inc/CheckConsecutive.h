/**
 * @file CheckConsecutive.h
 * @brief 连续判定（时间消抖）：连续 N 次 In >= Threshold 才通过
 * @layer BSP
 * @note 注解给出的 Check_Consecutive；单次不满足则清零计数。
 */
#pragma once

#include <cstdint>

namespace bsp
{
/**
 * @brief 连续阈值判定
 */
class CheckConsecutive
{
  public:
    /**
     * @brief 判定输入是否连续 MaxCount 次 >= Threshold
     * @param in        当前采样
     * @param threshold 判定阈值
     * @param pCount    连续满足次数（就地更新）
     * @param maxCount  需要的连续次数
     * @return true=已连续通过；false=未达到或被打断清零
     */
    static bool Apply(uint32_t in, uint32_t threshold, uint8_t* pCount, uint8_t maxCount)
    {
        if ((pCount == nullptr) || (maxCount == 0U))
        {
            return false;
        }

        if (in >= threshold)
        {
            if ((*pCount) < maxCount)
            {
                (*pCount)++;
            }
            if ((*pCount) >= maxCount)
            {
                (*pCount) = maxCount;
                return true;
            }
            return false;
        }

        (*pCount) = 0U;
        return false;
    }

    /**
     * @brief 判定输入是否连续 MaxCount 次 < Threshold（用于清除累计）
     */
    static bool ApplyBelow(uint32_t in, uint32_t threshold, uint8_t* pCount, uint8_t maxCount)
    {
        if ((pCount == nullptr) || (maxCount == 0U))
        {
            return false;
        }

        if (in < threshold)
        {
            if ((*pCount) < maxCount)
            {
                (*pCount)++;
            }
            if ((*pCount) >= maxCount)
            {
                (*pCount) = maxCount;
                return true;
            }
            return false;
        }

        (*pCount) = 0U;
        return false;
    }
};
} // namespace bsp
