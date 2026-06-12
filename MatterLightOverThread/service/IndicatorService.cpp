/**
 * @file IndicatorService.cpp
 * @brief 指示灯服务实现
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 红灯/白呼吸与主灯独立；由 10ms tick 驱动。
 */
#include "IndicatorService.h"
#include "AppConfig.h"
#include "app/AppLog.h"

/** @brief 获取 IndicatorService 单例 */
IndicatorService& IndicatorService::instance()
{
  static IndicatorService s_svc;
  return s_svc;
}

/** @brief 10ms 轮询白呼吸与红灯闪烁 */
void IndicatorService::tick10ms()
{
  pollWhiteBreath();
  dispatchRedBlink();
}

/** @brief 白呼吸 tick */
void IndicatorService::pollWhiteBreath()
{
  IndicatorHal::pollWhiteBreath10ms();
}

/** @brief 红灯闪烁调度与 tick */
void IndicatorService::dispatchRedBlink()
{
  IndicatorHal::pollRedBlink10ms();
}

/** @brief 启动单速红灯闪烁 */
void IndicatorService::startRedBlinkNormal(uint16_t periodMs, uint16_t count)
{
  LOG_INDIC("[Indicator] red blink normal period=%u count=%u", periodMs, count);
  IndicatorHal::startRedBlinkNormal(periodMs, count);
}

/** @brief 启动双速混合红灯闪烁（恢复出厂预警） */
void IndicatorService::startRedBlinkMixed(uint16_t m1Period, uint16_t m1Count,
                                          uint16_t m2Period, uint16_t m2Count)
{
  LOG_INDIC("[Indicator] red blink mixed m1=%u/%u m2=%u/%u", m1Period, m1Count,
            m2Period, m2Count);
  IndicatorHal::startRedBlinkMixed(m1Period, m1Count, m2Period, m2Count);
}

/** @brief 停止所有红灯闪烁 */
void IndicatorService::stopRedBlink()
{
  IndicatorHal::stopRedBlink();
}

/** @brief 启动白呼吸（充电等场景，brightness 为百分比） */
void IndicatorService::startWhiteBreath(uint8_t brightnessPercent)
{
  LOG_INDIC("[Indicator] white breath start bright=%u%%", brightnessPercent);
  IndicatorHal::startWhiteBreath(brightnessPercent);
}

/** @brief 停止白呼吸 */
void IndicatorService::stopWhiteBreath()
{
  IndicatorHal::stopWhiteBreath();
}
