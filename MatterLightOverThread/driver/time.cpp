#include "time.h"

#include <cmsis_os2.h>

/* 获取系统毫秒：用户实现 */
uint32_t LED_GetTickMs(void) // 示例使用 RTOS 的系统滴答计数
{
    // osKernelGetTickCount() returns RTOS ticks, so convert with RTOS tick frequency.
    uint64_t tick_count = osKernelGetTickCount();
    uint32_t tick_freq = osKernelGetTickFreq();

    if (tick_freq == 0)
    {
        tick_freq = 1000;
    }

    return (uint32_t)((tick_count * 1000U) / tick_freq);
}
