/**
 * @file MatterBridge.h
 * @brief Matter 集成中间层：对外 API，经邮箱投递至 MatterBridgeServer
 * @author hady
 * @date 2026-06-12
 * @layer Middleware
 * @note 不直接调用 Matter 栈；MailPoster 与状态查询由 entry 注册。
 */
#pragma once

#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
#include "LightingManager.h"
#include "RGBLEDWidget.h"
#endif

#include "MatterMailPacket.h"
#include <cstdint>

/**
 * @brief Matter 集成桥：仅负责组包并投递邮件
 */
class MatterBridge
{
public:
  using MailPoster         = void (*)(const MatterMailMsg& msg);
  using ReportBypassQuery  = bool (*)();
  using UnprovisionedQuery = bool (*)();

  static MatterBridge& Instance()
  {
    static MatterBridge matterBridge;
    return matterBridge;
  }

  void SetMailPoster(MailPoster poster);
  void SetReportBypassQuery(ReportBypassQuery query);
  void SetUnprovisionedQuery(UnprovisionedQuery query);

  void OnActionInitiated(int action, uint8_t* value, bool lightOn);
  void OnColorEvent(uint8_t action, void* valueData, uint16_t x, uint16_t y);

  void UploadOnOff(bool on);
  void UploadBrightnessPercent(uint8_t percent);
  void UploadColorFromCycleIndex(uint8_t cycleIndex);
  void UploadColorHsv(uint8_t hue, uint8_t saturation);

  bool IsUnprovisioned();
  void OpenCommissioningWindow();
  void CloseCommissioningWindow();
  void TriggerSoftNetworkReset();

  bool IsReportBypassEnabled();
  bool IsMatterReportBypassEnabled()
  {
    return IsReportBypassEnabled();
  }

  /*注册设备事件 */
  void RegisterDeviceEvents();

private:
  MatterBridge() = default;

  void PostMail(const MatterMailMsg& msg) const;

  MailPoster m_mailPoster{nullptr};
  ReportBypassQuery m_reportBypassQuery{nullptr};
  UnprovisionedQuery m_unprovisionedQuery{nullptr};
};
