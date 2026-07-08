/**
 * @file DebugLog.h
 * @brief 分类、可宏开关的调试串口日志封装（基于 SDK 的 SILABS_LOG）
 * @author hady
 * @date 2026-06-12
 * @layer Common
 * @note
 *   -
 * 设计目标：在不修改各业务模块调用风格的前提下，提供「按模块分类、可独立开关」
 *     的串口日志。每个分类对应一个使能宏，关闭时该分类日志在编译期被完全移除
 *     （不占用 Flash、不产生运行开销）。
 *   - 用法：模块内使用对应分类宏，例如 LightEngine 用 LOG_LIGHT(...)，
 *     ButtonPolicy 用 LOG_BTN(...)。格式串需为字符串字面量（与 printf 一致）。
 *   - 总开关 APP_LOG_ENABLED 关闭时，所有分类日志一并移除。
 *   - 约束（见 .cursorrules）：严禁在 ISR
 * 中调用同步日志；本宏仅用于任务上下文。
 */
#pragma once

#include "BspUartLog.h"
#include <stdarg.h>

/** @name 日志总开关
 *  @{ */
#ifndef APP_LOG_ENABLED
#define APP_LOG_ENABLED 1 ///< 1=启用全部分类日志；0=全部移除
#endif
/** @} */

/** @name 各业务分类开关（1=启用，0=编译期移除）
 *  @{ */
#ifndef APP_LOG_CAT_LIGHT
#define APP_LOG_CAT_LIGHT 1 ///< LightEngine 灯光引擎
#endif
#ifndef APP_LOG_CAT_BTN
#define APP_LOG_CAT_BTN 1 ///< 按键输入 / 按键策略
#endif
#ifndef APP_LOG_CAT_BAT
#define APP_LOG_CAT_BAT 1 ///< 电池 / 电源监控策略
#endif
#ifndef APP_LOG_CAT_COMMISSION
#define APP_LOG_CAT_COMMISSION 1 ///< Matter 配网
#endif
#ifndef APP_LOG_CAT_FACTORY
#define APP_LOG_CAT_FACTORY 1 ///< 恢复出厂 / 软复位
#endif
#ifndef APP_LOG_CAT_IDENTIFY
#define APP_LOG_CAT_IDENTIFY 1 ///< 设备识别 Identify
#endif
#ifndef APP_LOG_CAT_MATTER
#define APP_LOG_CAT_MATTER 1 ///< Matter 桥接 / 属性上下行
#endif
#ifndef APP_LOG_CAT_INDIC
#define APP_LOG_CAT_INDIC 1 ///< 指示灯（红灯 / 白呼吸）
#endif
#ifndef APP_LOG_CAT_APP
#define APP_LOG_CAT_APP 1 ///< 应用编排层
#endif
#ifndef APP_LOG_ENTRY_APP
#define APP_LOG_ENTRY_APP 1 ///< 应用编排层
#endif

#ifndef LOG_LIGHT_DC_APP
#define LOG_LIGHT_DC_APP 1 ///< 主灯决策中心
#endif

#ifndef APP_LOG_CAT_POWER
#define APP_LOG_CAT_POWER 1 ///< 电源
#endif
/** @} */
///< 底层日志实现接口，基于 UART 输出

/**
 * @def APP_LOG_IMPL
 * @brief 分类日志底层实现：受总开关与分类开关共同控制
 * @param en  分类使能（0/1）
 * @param fmt printf 风格格式串（字符串字面量）
 * @param ... 可变参数，与 fmt 对应
 */
#if APP_LOG_ENABLED
#define APP_LOG_IMPL(en, fmt, ...)                                                                                                                   \
    BspUartLog::DebugLog(en, "[%s:%d] " fmt, __builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__,           \
                         ##__VA_ARGS__)

#else
#define APP_LOG_IMPL(en, fmt, ...) ((void)0)
#endif

/** @name 各分类日志宏（保留调用处 "[Module] " 前缀字符串）
 *  @{ */
/** @def LOG_LIGHT
 *  @brief LightEngine 分类日志
 *  @param fmt printf 格式串
 *  @param ... 可变参数 */
#define LOG_LIGHT(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_LIGHT, fmt, ##__VA_ARGS__)
/** @def LOG_BTN
 *  @brief 按键输入/策略分类日志 */
#define LOG_BTN(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_BTN, fmt, ##__VA_ARGS__)
/** @def LOG_BAT
 *  @brief 电池/电源监控分类日志 */
#define LOG_BAT(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_BAT, fmt, ##__VA_ARGS__)

#define LOG_POWER(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_POWER, fmt, ##__VA_ARGS__)
/** @def LOG_COMMISSION
 *  @brief Matter 配网分类日志 */
#define LOG_COMMISSION(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_COMMISSION, fmt, ##__VA_ARGS__)
/** @def LOG_FACTORY
 *  @brief 恢复出厂/软复位分类日志 */
#define LOG_FACTORY(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_FACTORY, fmt, ##__VA_ARGS__)
/** @def LOG_IDENTIFY
 *  @brief Identify 分类日志 */
#define LOG_IDENTIFY(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_IDENTIFY, fmt, ##__VA_ARGS__)
/** @def LOG_MATTER
 *  @brief Matter 桥接/属性上下行分类日志 */
#define LOG_MATTER(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_MATTER, fmt, ##__VA_ARGS__)
/** @def LOG_INDIC
 *  @brief 指示灯分类日志 */
#define LOG_INDIC(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_INDIC, fmt, ##__VA_ARGS__)
/** @def LOG_APP
 *  @brief 应用编排层分类日志 */
#define LOG_APP(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_APP, fmt, ##__VA_ARGS__)
/** @} */
#define LOG_ENTRY(fmt, ...) APP_LOG_IMPL(APP_LOG_ENTRY_APP, fmt, ##__VA_ARGS__)

#define LOG_LIGHT_DC(fmt, ...) APP_LOG_IMPL(LOG_LIGHT_DC_APP, fmt, ##__VA_ARGS__)
