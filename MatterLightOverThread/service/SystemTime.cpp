/**
 * @file SystemTime.cpp
 * @brief 系统毫秒时钟实现
 * @author hady
 * @date 2026-06-12
 * @layer Service
 */
#include "app/SystemTime.h"
#include <cmsis_os2.h>

/** @brief 获取自启动以来的毫秒数 */
uint32_t SystemTime::nowMs()
{
  uint64_t tick_count = osKernelGetTickCount();
  uint32_t tick_freq  = osKernelGetTickFreq();
  if (tick_freq == 0)
    tick_freq = 1000;
  return (uint32_t)((tick_count * 1000U) / tick_freq);
}
