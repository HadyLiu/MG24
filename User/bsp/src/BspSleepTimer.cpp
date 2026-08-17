/**
 * @file BspSleepTimer.cpp
 * @author hady
 * @brief 低频计时器相关接口实现
 * @version 0.1
 * @date 2026-06-14
 */
#include "BspSleepTimer.h"

uint32_t BspSleepTimer::BspGetLowFrequencyTick(void) {
  return sl_sleeptimer_get_tick_count();
}

uint32_t BspSleepTimer::BspGetLowFrequencyMs(void) {
  return sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count());
}
