/**
 * @file MatterBridge.cpp
 * @brief Matter 集成中间层：组包并投递邮件至 MatterBridgeServer
 * @author hady
 * @date 2026-06-12
 * @layer Middleware
 */
#include "MatterBridge.h"

/**
 * @brief 设置邮件投递回调
 * @param poster 投递函数指针
 * @return 无
 */
void MatterBridge::SetMailPoster(MailPoster poster)
{
  m_mailPoster = poster;
}

/**
 * @brief 设置报告绕过查询回调
 * @param query 查询函数指针
 * @return 无
 */
void MatterBridge::SetReportBypassQuery(ReportBypassQuery query)
{
  m_reportBypassQuery = query;
}

/**
 * @brief 设置未配置查询回调
 * @param query 查询函数指针
 * @return 无
 */
void MatterBridge::SetUnprovisionedQuery(UnprovisionedQuery query)
{
  m_unprovisionedQuery = query;
}

/**
 * @brief 内部邮件投递函数
 * @param msg 邮件内容
 * @return 无
 */
void MatterBridge::PostMail(const MatterMailMsg& msg) const
{
  if (m_mailPoster != nullptr)
  {
    m_mailPoster(msg);
  }
}

/**
 * @brief 处理动作发起事件：组包并投递
 * @param action  动作类型
 * @param value   动作值指针（可选）
 * @param lightOn 灯状态
 * @return 无
 */
void MatterBridge::OnActionInitiated(int action, uint8_t* value, bool lightOn)
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryDownlink;
  msg.cmd      = MatterMailCmd::kDownlinkAction;
  msg.byte0    = static_cast<uint8_t>(action);
  msg.byte1    = lightOn ? 1U : 0U;
  if (value != nullptr)
  {
    msg.word0 = *value;
  }
  PostMail(msg);
}

/**
 * @brief 处理颜色事件：组包并投递
 * @param action    颜色动作类型
 * @param valueData 颜色数据指针（结构依赖于动作类型）
 * @param x         颜色 X 坐标（仅 COLOR_ACTION_XY）
 * @param y         颜色 Y 坐标（仅 COLOR_ACTION_XY）
 * @return 无
 */
void MatterBridge::OnColorEvent(uint8_t action, void* valueData, uint16_t x,
                                uint16_t y)
{
  if (valueData == nullptr)
  {
    return;
  }

  MatterMailMsg msg{};
  msg.category = kMatterCategoryDownlink;
  msg.cmd      = MatterMailCmd::kDownlinkColor;
  msg.byte0    = action;
  msg.word0    = x;
  msg.word1    = y;

#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
  const auto* colorData =
      reinterpret_cast<const RGBLEDWidget::ColorData_t*>(valueData);
  if (action == static_cast<uint8_t>(LightingManager::COLOR_ACTION_CT))
  {
    msg.word0 = colorData->ct.ctMireds;
  }
  else if (action == static_cast<uint8_t>(LightingManager::COLOR_ACTION_HSV))
  {
    msg.byte1 = colorData->hsv.h;
    msg.word1 = colorData->hsv.s;
  }
#else
  (void)valueData;
#endif

  PostMail(msg);
}

/**
 * @brief 上报开关状态
 * @param on 是否打开
 * @return 无
 */
void MatterBridge::UploadOnOff(bool on)
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryUpload;
  msg.cmd      = MatterMailCmd::kUploadOnOff;
  msg.byte0    = on ? 1U : 0U;
  PostMail(msg);
}

/**
 * @brief 上报亮度百分比
 * @param percent 亮度百分比（0-100）
 * @return 无
 */
void MatterBridge::UploadBrightnessPercent(uint8_t percent)
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryUpload;
  msg.cmd      = MatterMailCmd::kUploadBrightness;
  msg.byte0    = percent;
  PostMail(msg);
}

/**
 * @brief 上报颜色循环索引
 * @param cycleIndex 颜色循环索引
 * @return 无
 */
void MatterBridge::UploadColorFromCycleIndex(uint8_t cycleIndex)
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryUpload;
  msg.cmd      = MatterMailCmd::kUploadColorCt;
  msg.byte0    = cycleIndex;
  PostMail(msg);
}

void MatterBridge::UploadColorHsv(uint8_t hue, uint8_t saturation)
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryUpload;
  msg.cmd      = MatterMailCmd::kUploadColorHsv;
  msg.byte0    = hue;
  msg.byte1    = saturation;
  PostMail(msg);
}

/**
 * @brief 查询设备是否未配置
 * @return true 如果设备未配置，否则 false
 */
bool MatterBridge::IsUnprovisioned()
{
  if (m_unprovisionedQuery != nullptr)
  {
    return m_unprovisionedQuery();
  }
  return true;
}

/**
 * @brief 打开配网窗口
 * @return 无
 */
void MatterBridge::OpenCommissioningWindow()
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryControl;
  msg.cmd      = MatterMailCmd::kOpenCommissioning;
  PostMail(msg);
}

/**
 * @brief 关闭配网窗口
 * @return 无
 */
void MatterBridge::CloseCommissioningWindow()
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryControl;
  msg.cmd      = MatterMailCmd::kCloseCommissioning;
  PostMail(msg);
}

/**
 * @brief 触发软网络重置
 * @return 无
 */
void MatterBridge::TriggerSoftNetworkReset()
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryControl;
  msg.cmd      = MatterMailCmd::kSoftNetworkReset;
  PostMail(msg);
}

/**
 * @brief 查询是否启用报告绕过
 * @return true 如果启用，否则 false
 */
bool MatterBridge::IsReportBypassEnabled()
{
  if (m_reportBypassQuery != nullptr)
  {
    return m_reportBypassQuery();
  }
  return false;
}

void MatterBridge::RegisterDeviceEvents()
{
  MatterMailMsg msg{};
  msg.category = kMatterCategoryControl;
  msg.cmd      = MatterMailCmd::kRegisterDeviceEvents;
  PostMail(msg);
}
