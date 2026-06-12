/**
 * @file ColorLibrary.h
 * @brief 颜色库服务：12 色循环顺序与 2450K(#7) 默认色
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 循环顺序固定为库 ID {5,6,7,8,9,10,13,20,25,30,34,40}，默认
 * #7（2450K）。 色温(mireds)为近似值，仅用于 APP 显示同步。
 */
#pragma once

#include "app/LightTypes.h"
#include <stdint.h>

/** @brief 产品颜色库：顺序 ID {5,6,7,...,40}，默认 #7=2450K */
class ColorLibrary
{
public:
  static constexpr uint8_t kCycleCount = 12; ///< 循环颜色数量
  static constexpr uint8_t kDefaultCycleIndex =
      2; ///< 默认循环索引（对应库 #7=2450K）

  /** @brief 获取 ColorLibrary 单例
   *  @return 单例引用 */
  static ColorLibrary& instance();

  /**
   * @brief 通过循环索引获取颜色库颜色
   * @param cycleIndex 循环索引 0~11（越界归零）
   * @return 对应 WRGB 预设
   */
  WrgbColor colorAtCycleIndex(uint8_t cycleIndex) const;

  /**
   * @brief 按颜色库 ID 查 WRGB
   * @param libraryId 颜色库 ID
   * @return 命中返回对应 WRGB，未命中返回全黑
   */
  WrgbColor colorByLibraryId(uint8_t libraryId) const;

  /**
   * @brief 通过循环索引获取颜色库 ID
   * @param cycleIndex 循环索引 0~11（越界归零）
   * @return 颜色库 ID
   */
  uint8_t libraryIdAtCycleIndex(uint8_t cycleIndex) const;

  /**
   * @brief 获取下一个循环索引（到末尾回到 0）
   * @param current 当前循环索引
   * @return 下一个循环索引
   */
  uint8_t nextCycleIndex(uint8_t current) const;

  /** @brief 获取默认循环索引（库 #7）
   *  @return 默认循环索引 */
  uint8_t defaultCycleIndex() const
  {
    return kDefaultCycleIndex;
  }

  /**
   * @brief 将库 ID 映射为近似色温(mireds)
   * @param libraryId 颜色库 ID
   * @return 命中返回近似 mireds，未命中返回 #7 默认值 408
   */
  uint16_t libraryIdToMireds(uint8_t libraryId) const;

private:
  ColorLibrary() = default;
};
