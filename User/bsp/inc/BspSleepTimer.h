
/**
 * @brief Sleeptimer HAL，封装睡眠定时器相关接口
 * @author hady
 * @date 2026-06-14
 * @layer HAL
 * @note 系统启动阶段调用 init 一次；AppScheduler 通过 sleeptimerInstance
 * 访问驱动。
 */
#pragma once

#include "sl_sleeptimer.h"

class BspSleepTimer
{
  public:
    /* 获取低频计时器的当前 tick 数 */
    static uint32_t BspGetLowFrequencyTick();

    /** 获取低频计时器的当前毫秒数*/
    static uint32_t BspGetLowFrequencyMs();
};
