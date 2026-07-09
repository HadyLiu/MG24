/**
 * @file LightEffectProcessor.h
 * @brief 灯效原子算子库（Middleware层，无状态、纯算法）
 * @author hady
 * @date 2026-06-15
 * @layer Middlewares
 */

#pragma once

#include <stdint.h>

class LightEffectProcessor
{
  public:
    /* 保持对齐  */

    /* 混合输出4096*aim的值 使用需>>12  */
    /* 计算呼吸灯计算 (0-4096) */
    static uint32_t CalcBreath80BytesFactor(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /* 淡入曲线插值计算 */
    static uint32_t GetBezier80BytesFactorFadeIn(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);
    static uint32_t GetBezier40BytesFactorFadeIn(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /* 淡出曲线插值计算 */
    static uint32_t GetBezier80BytesFactorFadeOut(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);
    static uint32_t GetBezier40BytesFactorFadeOut(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /* 线性插值计算 用于淡变平滑过渡 */
    static uint32_t GetLerp(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /* 占空比闪烁算子 */
    static uint32_t GetBlink(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /* 保持静态快照的占空比输出，直到超时自动释放 */
    static uint32_t GetKeep(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /* 预计算表的最大因子值 */
    static uint8_t GetMaxFactorBits()
    {
        return kMaxFactorBits;
    }
    static uint16_t GetMaxFactor()
    {
        return kMaxFactor;
    }

  private:
    static constexpr uint8_t  kMaxFactorBits = 12U;
    static constexpr uint16_t kMaxFactor     = (1U << kMaxFactorBits);

    static uint32_t blendingOperator(uint32_t start, uint32_t end, uint32_t factor);
};