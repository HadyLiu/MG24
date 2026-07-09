
/**
 * @file BspUartLog.h
 * @brief UART 日志输出接口
 * @author hady
 * @date 2026-06-12
 * @layer BSP
 * @note 提供通过 UART 输出日志的功能，供开发调试使用。
 */
#pragma once
#include <stdarg.h>

class BspUartLog
{
  public:
    static void DebugLog(bool en, const char* fmt, ...);
};
