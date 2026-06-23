/**
 * @file BspTimer.h
 * @brief 支持多实例静态分配的 EFR32MG24 Sleeptimer 封装类
 * @layer BSP (Board Support Package)
 */

#pragma once

#include "sl_sleeptimer.h"
#include <cassert>
#include <stdint.h>

class BspTimer
{
public:
  // 统一定时器回调函数指针类型
  using TimerTickCallback = void (*)(uint32_t elapsedMs);

  // 显式构造函数，初始化内部干净的状态
  BspTimer()
      : m_tickCallback(nullptr), m_periodMs(0), m_isInitialized(false),
        m_isRunning(false)
  {
    // 将底层底层句柄结构体清零，防止野指针
    m_timerHandle = {0};
  }

  // 防御性编程：禁止拷贝和赋值，从语法层杜绝内存碎片与非预期的对象复制
  BspTimer(const BspTimer&)            = delete;
  BspTimer& operator=(const BspTimer&) = delete;

  /**
   * @brief 初始化特定的定时器实例
   * @param tickCallback 定时器周期触发时的上层回调函数
   * @param periodMs     定时器的触发周期 (ms)
   */
  void Init(TimerTickCallback tickCallback, uint32_t periodMs)
  {
    assert(tickCallback != nullptr);
    assert(periodMs > 0);
    assert(!m_isInitialized); // 防止重复初始化

    m_tickCallback  = tickCallback;
    m_periodMs      = periodMs;
    m_isInitialized = true;
  }

  /**
   * @brief 提供给上层（如 LightEffectEngine）注册的标准接口控制函数
   * @note  由于成员函数包含 this 指针，我们需要通过静态包装或直接调用该实例方法
   * @param start true 为开启时钟，false 为挂起时钟
   */
  void Control(bool start)
  {
    assert(m_isInitialized == true);

    if (start)
    {
      if (!m_isRunning)
      {
        // 启动 Gecko SDK 周期性定时器
        sl_status_t status = sl_sleeptimer_start_periodic_timer_ms(
            &m_timerHandle, m_periodMs, BspTimer::SleeptimerCallbackBridge,
            this, // 将当前实例的 this 指针作为上下文传入
            0,    // 默认优先级
            SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);

        if (status == SL_STATUS_OK)
        {
          m_isRunning = true;
        }
      }
    }
    else
    {
      if (m_isRunning)
      {
        sl_sleeptimer_stop_timer(&m_timerHandle);
        m_isRunning = false;
      }
    }
  }

  bool IsRunning() const
  {
    return m_isRunning;
  }

private:
  /**
   * @brief 底层标准的硬回调桥接函数（静态方法）
   * @param data 传入的 context 指针，此处承载的是对应 BspTimer 实例的 this 指针
   */
  static void SleeptimerCallbackBridge(sl_sleeptimer_timer_handle_t* handle,
                                       void* data)
  {
    (void)handle;
    assert(data != nullptr);

    // 将 void* 降维还原为具体的类实例指针，实现多实例数据隔离分发
    BspTimer* instance = static_cast<BspTimer*>(data);

    if (instance->m_isRunning && instance->m_tickCallback != nullptr)
    {
      instance->m_tickCallback(instance->m_periodMs);
    }
  }

  sl_sleeptimer_timer_handle_t
      m_timerHandle;                ///< 每个实例拥有独立的硬件管理句柄结构体
  TimerTickCallback m_tickCallback; ///< 独立的回调函数指针
  uint32_t m_periodMs;              ///< 独立的周期时间
  bool m_isInitialized;             ///< 独立初始化状态
  bool m_isRunning;                 ///< 独立运行状态
};
