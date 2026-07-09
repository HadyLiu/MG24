/**
 * @file BspUartLog.cpp
 * @brief UART 日志输出实现
 * @author hady
 * @date 2026-06-12
 * @layer BSP
 * @note 通过 UART 输出日志，供开发调试使用。使用单例模式，提供全局访问接口。
 */
#include "BspUartLog.h"
#include "silabs_utils.h"
#include <cstdio>

void BspUartLog::DebugLog(bool en, const char* fmt, ...)
{
    if (!en)
    {
        return;
    }

    char    buf[192];
    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (n > 0)
    {
        SILABS_LOG("%s", buf);
    }
}
