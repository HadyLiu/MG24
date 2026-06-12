/**
 * @file ColorLibrary.h
 * @brief 颜色库服务：12 色循环顺序与 2450K(#7) 默认色
 * @layer Service
 */
#pragma once

#include "app/LightTypes.h"
#include <stdint.h>

/** @brief 产品颜色库：顺序 ID {5,6,7,...,40}，默认 #7=2450K */
class ColorLibrary
{
public:
    static constexpr uint8_t kCycleCount = 12;
    static constexpr uint8_t kDefaultCycleIndex = 2; ///< 颜色库 #7

    static ColorLibrary &instance();

    WrgbColor colorAtCycleIndex(uint8_t cycleIndex) const;
    WrgbColor colorByLibraryId(uint8_t libraryId) const;
    uint8_t   libraryIdAtCycleIndex(uint8_t cycleIndex) const;
    uint8_t   nextCycleIndex(uint8_t current) const;
    uint8_t   defaultCycleIndex() const { return kDefaultCycleIndex; }
    uint16_t  libraryIdToMireds(uint8_t libraryId) const;

private:
    ColorLibrary() = default;
};
