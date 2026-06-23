/**
 * @file BspLedWrgb.h
 * @brief 主灯 WRGB LED BSP 接口
 * @author hady
 * @date 2026-06-12
 * @layer BSP
 * @note 系统启动阶段调用 init 一次；led_pwm 通过 rgbDriverInstance 访问驱动。
 */
#pragma once

/** @brief 获取 WRGB SPI 驱动单例
 *  @return BspLedWrgb 引用
 *  @note
 */
#include "HalPwm.h"
#include "HalSpiSm15135e.h"

/* SPI WRGB 驱动 HAL 门面 */
class BspLedWrgb
{
public:
  /* 获取全局唯一实例的静态接口 */
  static BspLedWrgb& Instance()
  {
    static BspLedWrgb bspLedWrgb;
    return bspLedWrgb;
  }

  /* 初始化WRGB */
  void Init();

  /* 设置 RGBW 四通道 PWM（0~1023）*/
  void LedWrgbSetDuty(uint16_t w, uint16_t r, uint16_t g, uint16_t b);

  static uint8_t GetLedMaxNum()
  {
    return kLedMaxNum;
  }
  static uint8_t GetMaxPwmBits()
  {
    return kMaxPwmBits;
  }

  static uint16_t GetMaxPwmValue()
  {
    return kMaxPwmValue;
  }

private:
  static constexpr uint8_t kLedMaxNum    = 4U;
  static constexpr uint8_t kMaxPwmBits   = 10U;
  static constexpr uint16_t kMaxPwmValue = ((1U << kMaxPwmBits) - 1);
  HalPwm whitePwm_;
  HalSpiSm15135e rgb_;

  /* 私有构造函数，禁止外部实例化 */
  BspLedWrgb();
};
