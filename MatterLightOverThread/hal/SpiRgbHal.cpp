/**
 * @file SpiRgbHal.cpp
 * @brief SPI RGB 芯片 HAL 门面实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 */
#include "SpiRgbHal.h"
#include "SpiSm15135eHal.h"

static SpiSm15135eHal s_rgbDriver;

/** @brief 获取 RGB 驱动实例（供 led_pwm 调用）
 *  @return HalSm15135e 引用
 */
SpiSm15135eHal& rgbDriverInstance()
{
  return s_rgbDriver;
}

/** @brief 初始化 SM15135E 与 SPI 总线 */
void SpiRgbHal::init()
{
  s_rgbDriver.Init();
  s_rgbDriver.SetAllGain(CurrentGain::Gain_101_1MA);
}
