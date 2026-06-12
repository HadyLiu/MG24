/**
 * @file SpiRgbHal.cpp
 * @brief HAL：SM15135E 初始化实现
 * @layer HAL
 */
#include "SpiRgbHal.h"
#include "../sm15135e.h"

/** @brief 初始化 SM15135E RGB 驱动芯片 */
void SpiRgbHal::init()
{
    sm15135e_init();
}
