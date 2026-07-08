/**
 * @file BspLedIndicatorRed.h
 * @brief 红色指示灯 BSP 接口
 * @author hady
 * @date 2026-06-12
 * @layer BSP
 * @note 系统启动阶段调用 init 一次；HalPwm 通过 rLedPwmInstance 访问驱动。
 */

#pragma once

#include "BspIoConfig.h"
#include "HalGpio.h"

class BspLedIndicatorRed
{
public:
  /* 获取全局唯一实例的静态接口 */
  static BspLedIndicatorRed& Instance()
  {
    static BspLedIndicatorRed bspLedIndicatorRed;
    return bspLedIndicatorRed;
  }

  /* 初始化红色指示灯 GPIO */
  void Init();

  /* 设置红色指示灯开关状态 */
  void SetRedIndicator(bool on);

private:
  HalGpio indicatorRed_;

  /* 私有构造函数，禁止外部实例化 */
  BspLedIndicatorRed();
};