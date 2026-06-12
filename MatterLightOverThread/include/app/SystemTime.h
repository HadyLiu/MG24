/**
 * @file SystemTime.h
 * @brief 系统毫秒时钟工具
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 由 driver/time 迁入；基于 CMSIS-RTOS2 内核 tick 换算。
 */
#pragma once

#include <stdint.h>

/** @brief 系统时间工具，提供毫秒级单调时钟 */
class SystemTime
{
public:
  /** @brief 获取自启动以来的毫秒数
   *  @return 毫秒 tick */
  static uint32_t nowMs();
};
