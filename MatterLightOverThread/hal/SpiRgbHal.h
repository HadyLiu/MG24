/**
 * @file SpiRgbHal.h
 * @brief SPI RGB 芯片 HAL 门面（委托 HalSm15135e）
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 */
#pragma once

/** @brief SPI RGB 驱动 HAL 门面 */
class SpiRgbHal
{
public:
  /** @brief 初始化 SM15135E 与 SPI 总线
   *  @return 无 */
  static void init();
};
