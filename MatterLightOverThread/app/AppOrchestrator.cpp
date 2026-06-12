/**
 * @file AppOrchestrator.cpp
 * @brief 应用编排层实现：Init / Tick10ms 调度电池、灯光、指示灯、配网策略
 * @author hady
 * @date 2026-06-12
 * @layer App
 * @note 高频 10ms 与低功耗 400ms 两种轮询模式自动切换；由 MyCustomMonitor
 * 直接调用。
 */
#include "AppOrchestrator.h"
#include "../hal/PinHal.h"
#include "../hal/PwmHal.h"
#include "../hal/SpiRgbHal.h"
#include "../integration/MatterBridge.h"
#include "../policy/BatteryPolicy.h"
#include "../policy/ButtonPolicy.h"
#include "../policy/CommissioningManager.h"
#include "../service/ButtonInput.h"
#include "../service/IndicatorService.h"
#include "../service/LightEngine.h"
#include "../service/PowerMonitor.h"
#include "AppConfig.h"
#include "app/AppLog.h"

/** @brief 获取 AppOrchestrator 单例 */
AppOrchestrator& AppOrchestrator::instance()
{
  static AppOrchestrator s_app;
  return s_app;
}

/** @brief 唤醒后重新初始化 ADC 等模块 */
void AppOrchestrator::moduleInit()
{
  PowerMonitor::instance().adcInit();
}

/** @brief 进入 EM2 前关闭电池输出与 PWM */
void AppOrchestrator::moduleDeinit()
{
  LOG_APP("[Orchestrator] module deinit, entering low power");
  PowerMonitor::instance().batOutDisable();
  PwmHal::enterLowPower();
  PowerMonitor::instance().adcDeInit();
}

/** @brief 外部事件唤醒时重置 idle 状态 */
void AppOrchestrator::clearTimeout()
{
  LOG_APP("[Orchestrator] wake up, clear idle timeout");
  m_idleTime10ms    = 0;
  m_normalPowerMode = true;
  moduleInit();
}

/** @brief 3s 无活动后切换到低功耗轮询 */
void AppOrchestrator::sleepTimeout(bool clearFlag)
{
  ++m_idleTime10ms;
  if (m_idleTime10ms > 300)
    m_idleTime10ms = 300;

  if (clearFlag)
  {
    m_idleTime10ms    = 0;
    m_normalPowerMode = true;
  }
  else if (m_idleTime10ms >= 300)
  {
    m_normalPowerMode = false;
    LOG_APP("[Orchestrator] idle %u ms, switch to low power mode",
            m_idleTime10ms * 10U);
  }
}

/**
 * @brief 上电初始化全流程
 */
void AppOrchestrator::init()
{
  LOG_APP("[Orchestrator] init start");
  PinHal::init();
  SpiRgbHal::init();
  ButtonInput::instance().init();
  PowerMonitor::instance().adcInit();
  PowerMonitor::instance().init();
  LightEngine::instance().init();

  ButtonInput::instance().setHandler(
      [](const ButtonEvent& ev) { ButtonPolicy::instance().handle(ev); });

  if (MatterBridge::instance().isUnprovisioned() &&
      LightEngine::instance().isOn())
  {
    LOG_APP("[Orchestrator] unprovisioned + light on, auto commissioning");
    CommissioningManager::instance().onLightTurnedOn();
  }
  LOG_APP("[Orchestrator] init done");
}

/** @brief 10ms 主循环：电池 → 灯光 tick → 指示灯 → 配网计时 → 休眠判定 */
bool AppOrchestrator::tick10ms(bool* interruptWakeUp)
{
  if (interruptWakeUp && *interruptWakeUp)
  {
    *interruptWakeUp = false;
    clearTimeout();
  }

  if (!m_normalPowerMode)
  {
    PowerMonitor& pm = PowerMonitor::instance();
    pm.refreshUpstreamPower();
    if (pm.powerStatus() != pm.upstreamPowerStatus())
      clearTimeout();
  }

  if (!m_normalPowerMode)
    return false;

  m_tickCount++;
  BatteryPolicy::instance().tick10ms(m_tickCount);
  LightEngine::instance().tick10ms();
  IndicatorService::instance().tick10ms();
  CommissioningManager::instance().tick10ms(0);

  PowerMonitor& pm = PowerMonitor::instance();
  bool stayAwake   = pm.powerStatus() ||
                   (!pm.powerStatus() && LightEngine::instance().isOn() &&
                    !pm.isPowerProtect());
  sleepTimeout(stayAwake);

  if (!m_normalPowerMode)
    moduleDeinit();

  return m_normalPowerMode;
}
