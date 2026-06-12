/**
 * @file WakeControl.h
 * @brief 低功耗唤醒控制接口
 * @author hady
 * @date 2026-06-12
 * @layer App
 * @note 由 MyCustomMonitor 实现；按键/Matter 事件触发唤醒时调用。
 */
#pragma once

/** @brief 请求从低功耗轮询模式唤醒到正常 10ms 模式
 *  @return 无 */
void ConditionalWake_up(void);
