/**
 * @file bsp_led_indic_white.h
 * @brief 白色指示灯 BSP 接口
 * @author hady
 * @date 2026-06-12
 * @layer BSP
 * @note 系统启动阶段调用 init 一次；HalPwm 通过 rLedPwmInstance 访问驱动。
 */

#pragma once

#include "HalPwm.h"

class BspLedIndicatorWhite
{
public:
  // 获取全局唯一实例的静态接口
  static BspLedIndicatorWhite& Instance()
  {
    static BspLedIndicatorWhite bspLedIndicatorWhite;
    return bspLedIndicatorWhite;
  }

  /** @brief 初始化白色指示灯 PWM 驱动*/
  void Init();

  /** @brief 设置白色指示灯 PWM 占空比(0-1023) */
  void IndicatorWhiteSetDuty(uint16_t duty);

private:
  HalPwm IndicWhitePwm_;

  /* 私有构造函数，禁止外部实例化 */
  BspLedIndicatorWhite();
};
