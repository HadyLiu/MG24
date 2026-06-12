/**
 * @file AppOrchestrator.h
 * @brief 应用编排层：系统唯一业务入口，负责 Init / Tick10ms 调度各 Service 与 Policy
 * @layer App
 */
#pragma once

#include <stdint.h>

/**
 * @class AppOrchestrator
 * @brief 单例应用编排器，替代原 entry.cpp 中的多域逻辑聚合
 */
class AppOrchestrator
{
public:
    /** @brief 获取全局单例 */
    static AppOrchestrator &instance();

    /** @brief 上电初始化：GPIO、驱动、灯光、按键、配网检测 */
    void init();

    /**
     * @brief 10ms 主循环 tick
     * @param interruptWakeUp 外部唤醒标志，入参非空且为 true 时清除并恢复高频模式
     * @return true=正常 10ms 模式；false=低功耗 400ms 模式
     */
    bool tick10ms(bool *interruptWakeUp);

private:
    AppOrchestrator() = default;

    uint16_t m_idleTime10ms = 0;   ///< 空闲计时（单位 10ms）
    uint32_t m_tickCount = 0;      ///< 全局 tick 计数
    bool     m_normalPowerMode = true; ///< true=10ms 高频；false=低功耗

    /** @brief 清除 idle 计时并恢复模块 */
    void clearTimeout();
    /** @brief 更新 idle 计时，超时进入低功耗 */
    void sleepTimeout(bool clearFlag);
    /** @brief 模块级初始化（ADC 等） */
    void moduleInit();
    /** @brief 进入低功耗前的资源释放 */
    void moduleDeinit();
};
