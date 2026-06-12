/**
 * @file ColorLibrary.cpp
 * @brief 颜色库服务实现：12 色循环顺序、WRGB 预设与色温(mireds)映射
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 产品颜色库循环顺序固定为库 ID {5,6,7,8,9,10,13,20,25,30,34,40}；
 *       开箱默认 #7（2450K 暖白）。WRGB 预设沿用既有标定值，
 *       仅将 #7 修正为 2450K 暖白。色温(mireds)为近似值，仅用于 APP 显示同步，
 *       不改变设备侧实际 WRGB 输出。
 */
#include "ColorLibrary.h"

/**
 * @brief 颜色库循环顺序（库 ID）
 * @note 下标即「循环索引」(cycle index)，0~11；元素为产品颜色库 ID。
 *       默认 #7 位于循环索引 2（见 kDefaultCycleIndex）。
 */
static const uint8_t kLibraryIds[ColorLibrary::kCycleCount] = {
    5, 6, 7, 8, 9, 10, 13, 20, 25, 30, 34, 40};

/**
 * @brief 12 个循环颜色的 WRGB 预设（下标 = 循环索引，与 kLibraryIds 一一对应）
 * @note 沿用既有标定值；循环索引 2（库 #7）修正为 2450K 暖白
 *       = ColorConverter::fromColorTemperature(2450) = {1023,178,40,0}。
 */
static const WrgbColor kCycleColors[ColorLibrary::kCycleCount] = {
    //   W,    R,    G,    B    // cycleIdx (libId)
    {409, 1023, 0, 0},   // 0 (#5)
    {409, 0, 235, 563},  // 1 (#6)
    {1023, 178, 40, 0},  // 2 (#7)  2450K 暖白（开箱默认）
    {327, 0, 235, 1023}, // 3 (#8)
    {0, 1023, 179, 0},   // 4 (#9)
    {0, 1023, 102, 0},   // 5 (#10)
    {0, 1023, 0, 0},     // 6 (#13)
    {0, 1023, 102, 59},  // 7 (#20)
    {0, 1023, 240, 404}, // 8 (#25)
    {0, 240, 240, 1023}, // 9 (#30)
    {0, 522, 844, 522},  // 10 (#34)
    {0, 522, 1023, 159}  // 11 (#40)
};

/**
 * @brief 12 个循环颜色的近似色温(mireds)（下标 = 循环索引）
 * @note 仅用于 Matter ColorControl CT 上行，使 APP 不同颜色显示有所区分；
 *       受 ColorTempPhysicalMin/Max(153~454) 限幅。库 #7 = 2450K -> 408。
 *       饱和色无真实色温，此处为落在合法区间内的近似占位值。
 */
static const uint16_t kCycleMireds[ColorLibrary::kCycleCount] = {
    370, // 0 (#5)
    200, // 1 (#6)
    408, // 2 (#7)  2450K
    180, // 3 (#8)
    360, // 4 (#9)
    370, // 5 (#10)
    400, // 6 (#13)
    390, // 7 (#20)
    340, // 8 (#25)
    170, // 9 (#30)
    250, // 10 (#34)
    220  // 11 (#40)
};

/** @brief 获取 ColorLibrary 单例 */
ColorLibrary& ColorLibrary::instance()
{
  static ColorLibrary s_lib;
  return s_lib;
}

/**
 * @brief 将库 ID 反查为循环索引
 * @param libraryId 颜色库 ID
 * @return 命中返回 0~11；未命中返回 kCycleCount（无效）
 */
static uint8_t cycleIndexOfLibraryId(uint8_t libraryId)
{
  for (uint8_t i = 0; i < ColorLibrary::kCycleCount; ++i)
  {
    if (kLibraryIds[i] == libraryId)
      return i;
  }
  return ColorLibrary::kCycleCount;
}

/**
 * @brief 按颜色库 ID 查 WRGB
 * @param libraryId 颜色库 ID
 * @return 命中返回对应 WRGB；未命中返回全黑
 */
WrgbColor ColorLibrary::colorByLibraryId(uint8_t libraryId) const
{
  uint8_t idx = cycleIndexOfLibraryId(libraryId);
  if (idx >= kCycleCount)
  {
    return WrgbColor{0, 0, 0, 0};
  }
  return kCycleColors[idx];
}

/**
 * @brief 通过循环索引获取颜色库 ID
 * @param cycleIndex 循环索引 0~11（越界归零）
 * @return 颜色库 ID
 */
uint8_t ColorLibrary::libraryIdAtCycleIndex(uint8_t cycleIndex) const
{
  if (cycleIndex >= ColorLibrary::kCycleCount)
    cycleIndex = 0;
  return kLibraryIds[cycleIndex];
}

/**
 * @brief 通过循环索引获取颜色库颜色
 * @param cycleIndex 循环索引 0~11（越界归零）
 * @return 对应 WRGB 预设
 */
WrgbColor ColorLibrary::colorAtCycleIndex(uint8_t cycleIndex) const
{
  if (cycleIndex >= ColorLibrary::kCycleCount)
    cycleIndex = 0;
  return kCycleColors[cycleIndex];
}

/**
 * @brief 获取下一个循环索引（到末尾回到 0）
 * @param current 当前循环索引
 * @return 下一个循环索引
 */
uint8_t ColorLibrary::nextCycleIndex(uint8_t current) const
{
  return (current + 1 >= kCycleCount) ? 0 : (current + 1);
}

/**
 * @brief 将库 ID 映射为近似色温(mireds)
 * @param libraryId 颜色库 ID
 * @return 命中返回近似 mireds；未命中返回 #7 默认值 408
 */
uint16_t ColorLibrary::libraryIdToMireds(uint8_t libraryId) const
{
  uint8_t idx = cycleIndexOfLibraryId(libraryId);
  if (idx >= kCycleCount)
    return 408; // 默认回退到 #7 (2450K)
  return kCycleMireds[idx];
}
