/**
 * @file ColorLibrary.cpp
 * @brief 颜色库服务实现：WRGB 预设与色温映射
 * @layer Service
 */
#include "ColorLibrary.h"
#include "../driver/led_mode_convert.h"

// 颜色库 ID 顺序：5,6,7,8,9,10,13,20,25,30,34,40
static const uint8_t kLibraryIds[ColorLibrary::kCycleCount] = {5, 6, 7, 8, 9, 10, 13, 20, 25, 30, 34, 40};

// 完整 WRGB 预设表（索引 = 颜色库 ID，0-40）
static const WrgbColor kFullColorTable[41] = {
    {1023, 0, 0, 0},     // 0
    {409, 1023, 0, 0},   // 1
    {409, 0, 235, 563},  // 2 ~2450K
    {327, 0, 235, 1023}, // 3
    {0, 1023, 179, 0},   // 4
    {0, 1023, 102, 0},   // 5
    {0, 1023, 0, 0},     // 6
    {0, 1023, 102, 59},  // 7 ~2450K 默认
    {0, 1023, 240, 404}, // 8
    {0, 240, 240, 1023}, // 9
    {0, 522, 844, 522},  // 10
    {0, 522, 1023, 159}, // 11
    {0, 1023, 322, 0},   // 12
    {0, 800, 900, 200},  // 13
    {0, 0, 0, 0},        // 14-19 placeholder
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {350, 700, 800, 300}, // 20
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {450, 600, 750, 350}, // 25
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {500, 550, 700, 400}, // 30
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {550, 500, 650, 450}, // 34
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {600, 450, 600, 500}, // 40
};

/** @brief 色温(K)转 WRGB，用于库 ID 无预设时回退 */
static WrgbColor kelvinToWrgb(uint16_t kelvin)
{
    WrgbColor c = {0, 0, 0, 0};
    Light_Calc_CT_To_WRGB(kelvin, &c.w, &c.r, &c.g, &c.b);
    return c;
}

/** @brief 获取 ColorLibrary 单例 */
ColorLibrary &ColorLibrary::instance()
{
    static ColorLibrary s_lib;
    return s_lib;
}

/** @brief 按颜色库 ID 查 WRGB，无预设则按色温估算 */
WrgbColor ColorLibrary::colorByLibraryId(uint8_t libraryId) const
{
    if (libraryId < 41)
    {
        WrgbColor c = kFullColorTable[libraryId];
        if (c.w != 0 || c.r != 0 || c.g != 0 || c.b != 0)
        {
            return c;
        }
    }
    // 未知 ID 按色温估算
    uint16_t kelvin = 2200 + (uint16_t)libraryId * 100;
    if (kelvin > 6500)
        kelvin = 6500;
    return kelvinToWrgb(kelvin);
}

uint8_t ColorLibrary::libraryIdAtCycleIndex(uint8_t cycleIndex) const
{
    if (cycleIndex >= ColorLibrary::kCycleCount)
        cycleIndex = 0;
    return kLibraryIds[cycleIndex];
}

WrgbColor ColorLibrary::colorAtCycleIndex(uint8_t cycleIndex) const
{
    return colorByLibraryId(libraryIdAtCycleIndex(cycleIndex));
}

uint8_t ColorLibrary::nextCycleIndex(uint8_t current) const
{
    return (current + 1 >= kCycleCount) ? 0 : (current + 1);
}

uint16_t ColorLibrary::libraryIdToMireds(uint8_t libraryId) const
{
    // 2450K (#7) -> 408 mireds
    static const uint16_t kMiredsMap[] = {
        400, 370, 408, 350, 320, 300, 280, 408, 260, 240, 220, 200, 190, 333, 0, 0, 0, 0, 0, 0,
        286, 0, 0, 0, 0, 250, 0, 0, 0, 0, 233, 0, 0, 0, 217, 0, 0, 0, 0, 0, 200};
    if (libraryId < 41 && kMiredsMap[libraryId] != 0)
        return kMiredsMap[libraryId];
    return 370;
}
