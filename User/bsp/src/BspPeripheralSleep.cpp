/**
 * @file BspPeripheralSleep.cpp
 * @brief BSP 外设休眠门面实现：空闲关外设，需要时再开
 * @author hady
 * @date 2026-07-30
 * @layer BSP
 */
#include "BspPeripheralSleep.h"

#include "BspLedIndicatorRed.h"
#include "BspLedIndicatorWhite.h"
#include "BspLedWrgb.h"
#include "DebugLog.h"
#include "em_cmu.h"

#ifndef APP_LOG_CAT_LP
#define APP_LOG_CAT_LP 1
#endif
#define LOG_LP(fmt, ...) APP_LOG_IMPL(APP_LOG_CAT_LP, fmt, ##__VA_ARGS__)

/**
 * @brief 上电后标记外设已就绪
 */
void BspPeripheralSleep::MarkReadyAfterBoot() {
  m_isSuspended = false;
  m_bootMarked = true;
  LOG_LP("[LP] peripheral boot ready");
}

/**
 * @brief 进入空闲：关闭当前不需要的外设
 * @note 严禁硬关 EUSART1/SPI 时钟（会导致 SPIDRV 挂死）。
 */
void BspPeripheralSleep::SuspendForSleep() {
  if (m_isSuspended) {
    return;
  }

  SuspendMainLightBusRaw();
  SuspendIndicatorPwmRaw();
  SuspendIadcClockRaw();
  m_isSuspended = true;
  LOG_LP("[LP] peripheral suspend done");
}

/**
 * @brief 退出空闲：按需恢复外设
 */
void BspPeripheralSleep::ResumeFromSleep() {
  if (!m_isSuspended) {
    return;
  }

  ResumeIadcClockRaw();
  ResumeMainLightBusRaw();
  ResumeIndicatorPwmRaw();
  m_isSuspended = false;
  LOG_LP("[LP] peripheral resume done");
}

/**
 * @brief 输出前确保总线可用
 */
void BspPeripheralSleep::EnsureReadyBeforeOutput() {
  if (m_isSuspended) {
    ResumeFromSleep();
  }
}

/**
 * @brief 主灯：芯片 Sleep 帧 + 停白光 TIMER
 */
void BspPeripheralSleep::SuspendMainLightBusRaw() {
  BspLedWrgb::Instance().SuspendBusForIdle();
}

/**
 * @brief 主灯：SPI Reset + 默认增益，供后续再出光
 */
void BspPeripheralSleep::ResumeMainLightBusRaw() {
  (void)m_bootMarked;
  BspLedWrgb::Instance().ResumeBusForActive();
}

/**
 * @brief 指示灯：白 PWM stop、红灯关
 */
void BspPeripheralSleep::SuspendIndicatorPwmRaw() {
  BspLedIndicatorWhite::Instance().IndicatorWhiteSetDuty(0U);
  BspLedIndicatorRed::Instance().SetRedIndicator(false);
}

/**
 * @brief 指示灯恢复占位（由 IndicatorEffectEngine 下一拍重下发）
 */
void BspPeripheralSleep::ResumeIndicatorPwmRaw() {
}

/**
 * @brief 关闭 IADC0 时钟（采样路径按需 Init 会再开）
 */
void BspPeripheralSleep::SuspendIadcClockRaw() {
  CMU_ClockEnable(cmuClock_IADC0, false);
}

/**
 * @brief 打开 IADC0 时钟
 */
void BspPeripheralSleep::ResumeIadcClockRaw() {
  CMU_ClockEnable(cmuClock_IADC0, true);
}
