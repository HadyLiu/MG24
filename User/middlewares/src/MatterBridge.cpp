/**
 * @file MatterBridge.cpp
 * @brief Matter 集成中间层：组包并投递邮件至 MatterBridgeServer
 * @author hady
 * @date 2026-06-12
 * @layer Middleware
 */
#include "MatterBridge.h"
#include "AppTask.h"
#include "DebugLog.h"

using namespace chip;
using namespace chip::DeviceLayer;

bool MatterBridge::g_bypass_zcl_callback = false;

/** @brief 初始化 Matter 桥 */
void MatterBridge::Init()
{
}

/**
 * @brief 注册matter下行数据
 * @return 无
 */
void MatterBridge::MatterDownlinkLocalRegister(MatterDownlinkCallback callback)
{
  m_matterDownlinkCallback = callback;
}

/**
 * @brief 本地上报
 * @param muc 上报数据
 * @return 无
 */
void MatterBridge::MatterUploadLocalReport(MatterDownlinkUploadPayload muc)
{
  switch (muc.element)
  {
    // 上报开关/亮度/颜色数据
  case MatterDataElement::kOn:
    MatterUploadSwitch(muc.on);
    break;
  case MatterDataElement::kBrightness:
    MatterUploadBrightness(muc.brightness);
    break;
  case MatterDataElement::kHsv:
    MatterUploadHsv(muc.color.hsv.hue, muc.color.hsv.saturation);
    break;
  case MatterDataElement::kCt:
    MatterUploadCt(muc.color.ct.colorTemperature);
    break;
  case MatterDataElement::kXy:
    MatterUploadXy(muc.color.xy.x, muc.color.xy.y);
    break;
  default:
    // 未知
    break;
  }
}

/**
 * @brief 执行Matter命令
 * @param executeElement 执行命令类型
 * @return 无
 */
void MatterBridge::MatterExecuteCmd(MatterExecuteElement executeElement)
{
  switch (executeElement)
  {
  case MatterExecuteElement::kClearNetwork:
    MatterClearNetwork();
    break;
  default:
    // 未知
    break;
  }
}

void MatterBridge::SetOn(bool isOn)
{
  m_matterDownlinkUploadPayload.on      = isOn;
  m_matterDownlinkUploadPayload.element = MatterDataElement::kOn;
}

void MatterBridge::SetBrightness(uint8_t brightness)
{
  m_matterDownlinkUploadPayload.brightness = brightness;
  m_matterDownlinkUploadPayload.element    = MatterDataElement::kBrightness;
}

void MatterBridge::SetHsv(uint8_t hue, uint8_t saturation)
{
  m_matterDownlinkUploadPayload.color.hsv.hue        = hue;
  m_matterDownlinkUploadPayload.color.hsv.saturation = saturation;
  m_matterDownlinkUploadPayload.element              = MatterDataElement::kHsv;
}

void MatterBridge::SetCt(uint16_t colorTemperature)
{
  m_matterDownlinkUploadPayload.color.ct.colorTemperature = colorTemperature;
  m_matterDownlinkUploadPayload.element = MatterDataElement::kCt;
}

void MatterBridge::SetXy(uint16_t x, uint16_t y)
{
  m_matterDownlinkUploadPayload.color.xy.x = x;
  m_matterDownlinkUploadPayload.color.xy.y = y;
  m_matterDownlinkUploadPayload.element    = MatterDataElement::kXy;
}
// ####################################################
//   连接到 Matter 底层接口
// ####################################################

void MatterBridge::EntryClearNetWork(AppEvent* aEvent)
{
  (void)aEvent;
  LOG_LIGHT_DC("InitiateFactoryReset (task ctx)");
  chip::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
}

/**
 * @brief 在 Matter 清理配网线程上下文执行软复位
 * @return 无
 */
void MatterBridge::MatterClearNetwork()
{
  AppEvent ev{};
  ev.Type    = AppEvent::kEventType_Button;
  ev.Handler = EntryClearNetWork;
  AppTask::GetAppTask().PostEvent(&ev);
}

/**
 * @brief 读取开关与亮度
 * @param aAction 动作类型
 * @param aValue 动作值
 * @return 无
 */
void MatterBridge::MatterOnBrightnessBridge(int aAction, uint8_t* aValue)
{
  bool change = false;
  //  判断是否是开关
  if (aAction == LightingManager::ON_ACTION ||
      aAction == LightingManager::OFF_ACTION)
  {
    if (aAction == LightingManager::ON_ACTION)
    {
      SetOn(true);
      change = true;
    }
    else if (aAction == LightingManager::OFF_ACTION)
    {
      SetOn(false);
      change = true;
    }
  }
  // 1. 判断是否是亮度
  if (aAction == LightingManager::LEVEL_ACTION) //&& g_led.is_on == true)
  {
    if (aValue != nullptr)
    {
      SetBrightness(*aValue);
      change = true;
    }
  }
  // 有变化触发回调
  if (change)
  {
    LOG_MATTER("从 Matter 读取开关/亮度数据成功");

    if (m_matterDownlinkCallback != nullptr)
    {
      m_matterDownlinkCallback(m_matterDownlinkUploadPayload);
    }
  }
}

/**
 * @brief 处理颜色相关的 Matter 下行数据
 * @param action 动作类型
 * @param valueData 动作值
 * @return 无
 */
void MatterBridge::MatterColorBridge(uint8_t action, void* valueData)
{
  if (valueData == nullptr)
  {
    return;
  }
  bool change = false;
  // 将泛型指针强转为官方标准颜色数据结构体
  auto* colorData = reinterpret_cast<RGBLEDWidget::ColorData_t*>(valueData);

  // 显式将 uint8_t 强转为官方的 Action_t 枚举，提高代码可读性
  switch (static_cast<LightingManager::Action_t>(action))
  {

  case LightingManager::COLOR_ACTION_HSV: {
    // 投递HSV色彩数据
    // 色调 (0-254)
    SetHsv(colorData->hsv.h, colorData->hsv.s);
    change = true;
    break;
  }

  case LightingManager::COLOR_ACTION_CT: {
    // 投递色温数据
    SetCt(colorData->ct.ctMireds);
    change = true;
    break;
  }

  case LightingManager::COLOR_ACTION_XY: {
    // 投递XY色彩数据
    SetXy(colorData->xy.x, colorData->xy.y);
    change = true;
    break;
  }

  default:
    // 未知
    break;
  }
  // 有变化触发回调
  if (change)
  {
    LOG_MATTER("从 Matter 读取颜色数据成功");
    if (m_matterDownlinkCallback != nullptr)
    {
      m_matterDownlinkCallback(m_matterDownlinkUploadPayload);
    }
  }
}

/**
 * @brief 本地上报回环阻断标志：true
 * 表示当前属性写入来自本地同步，不应再反向触发控制链
 * @return true 如果启用，否则 false
 */
bool MatterBridge::IsMatterReportBypassEnabled()
{
  return g_bypass_zcl_callback;
}

/**
 * @brief 在 Matter 线程执行 OnOff 属性写入
 */
void MatterBridge::Safe_Upload_OnOff_Callback(intptr_t context)
{
  const bool is_on                   = (context != 0);
  chip::EndpointId targetEndpoint    = 1;
  static uint8_t last_reported_onoff = 0xFF;
  const uint8_t onoff_value          = is_on ? 1 : 0;

  if (onoff_value == last_reported_onoff)
  {
    return;
  }

  g_bypass_zcl_callback = true;
  chip::Protocols::InteractionModel::Status status =
      chip::app::Clusters::OnOff::Attributes::OnOff::Set(targetEndpoint, is_on);
  g_bypass_zcl_callback = false;

  if (status == chip::Protocols::InteractionModel::Status::Success)
  {
    last_reported_onoff = onoff_value;
    LOG_MATTER("开关上报成功: %s \n", is_on ? "ON" : "OFF");
  }
  else
  {
    LOG_MATTER("开关上报失败: 状态码 0x%02X \n", static_cast<uint8_t>(status));
  }
}

/**
 * @brief 供驱动层调用的公开接口：上报开关状态
 */
void MatterBridge::MatterUploadSwitch(bool is_on)
{
  CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(
      Safe_Upload_OnOff_Callback, is_on ? 1 : 0);
  if (err != CHIP_NO_ERROR)
  {
    LOG_MATTER("上报开关状态失败: 0x%" CHIP_ERROR_FORMAT " \n", err.Format());
  }
}

/**
 * @brief 上报亮度百分比
 * @param driver_brightness_percent 亮度百分比（0-100）
 * @return 无
 */
void MatterBridge::Safe_Upload_Brightness_Callback(intptr_t context)
{
  uint8_t driver_brightness_percent = static_cast<uint8_t>(context);
  chip::EndpointId targetEndpoint   = 1;

  if (driver_brightness_percent > 100)
  {
    driver_brightness_percent = 100;
  }

  // 转换成 Matter 的 0~254 标准值
  uint8_t matter_level =
      (uint8_t)((uint16_t)driver_brightness_percent * 254 / 100);

  // 线程本地静态变量防重复轰炸
  static uint8_t last_reported_level = 0xFF;
  if (matter_level == last_reported_level)
  {
    return;
  }

  // 🎯 2. 此时这里也可以正确识别它，不再报错
  g_bypass_zcl_callback = true;

  // 写入属性数据库
  chip::Protocols::InteractionModel::Status status =
      chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Set(
          targetEndpoint, matter_level);

  // 解除阻断
  g_bypass_zcl_callback = false;

  if (status == chip::Protocols::InteractionModel::Status::Success)
  {
    LOG_MATTER("纯数据上报成功(未触发LEVEL_ACTION): %d%% \n",
               driver_brightness_percent);
    last_reported_level = matter_level;
  }
}

/**
 * @brief 供驱动层调用的公开接口：上报亮度百分比
 * @param driver_brightness_percent 亮度百分比（0-100）
 * @return 无
 */
void MatterBridge::MatterUploadBrightness(uint8_t driver_brightness_percent)
{
  if (driver_brightness_percent > 100)
  {
    driver_brightness_percent = 100;
  }
  // 投递到 Matter 线程
  CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(
      Safe_Upload_Brightness_Callback,
      static_cast<intptr_t>(driver_brightness_percent));
  if (err != CHIP_NO_ERROR)
  {
    LOG_MATTER("亮度上报投递失败: 0x%" CHIP_ERROR_FORMAT " \n", err.Format());
  }
}

void MatterBridge::MatterUploadHsv(uint8_t hue, uint8_t saturation)
{
  // 这里可以直接调用 Matter 的属性写入接口，或者通过 ScheduleWork 投递到 Matter
  // 线程 具体实现根据项目需求而定
}

void MatterBridge::MatterUploadCt(uint16_t colorTemperature)
{
  // 这里可以直接调用 Matter 的属性写入接口，或者通过 ScheduleWork 投递到 Matter
  // 线程 具体实现根据项目需求而定
}

void MatterBridge::MatterUploadXy(uint16_t x, uint16_t y)
{
  // 这里可以直接调用 Matter 的属性写入接口，或者通过 ScheduleWork 投递到 Matter
  // 线程 具体实现根据项目需求而定
}

/**
 * @brief Matter 设备事件回调：处理配网完成、BLE连接等事件
 * @param event 设备事件指针
 * @param arg   上下文参数（未使用）
 * @return 无
 */
void MatterBridge::OnMatterDeviceEvent(const ChipDeviceEvent* event,
                                       intptr_t arg)
{
  MatterBridge& self = MatterBridge::Instance();
  switch (event->Type)
  {
  // 🎯 核心事件：配网完成（手机成功将设备加入家庭网络）
  case DeviceEventType::kCommissioningComplete:
    LOG_MATTER("配网完成!");

    // 运行配对成功特效
    self.m_matterDownlinkUploadPayload.element =
        MatterDataElement::kCommissioningDone;
    if (self.m_matterDownlinkCallback != nullptr)
    {
      self.m_matterDownlinkCallback(self.m_matterDownlinkUploadPayload);
    }
    break;

  // 辅助判定事件：手机通过蓝牙与设备建立安全会话连接（处于扫码配对中间状态）
  case DeviceEventType::kCHIPoBLEConnectionEstablished: {
    LOG_MATTER("蓝牙连接已建立");
    break;
  }

  default:
    break;
  }
}

/**
 * @brief 注册 Matter 设备事件监听器
 * @return 无
 */
void MatterBridge::RegisterDeviceEventListener(void)
{
  // 注册 Matter 设备事件监听器，关注配网完成等关键事件
  PlatformMgr().AddEventHandler(MatterBridge::OnMatterDeviceEvent,
                                reinterpret_cast<intptr_t>(nullptr));

  // 注册 Identify 事件回调，处理设备识别请求
  //(void)gIdentify;
}

/**
 * @brief 重置网络的外部接口函数，供按键长按回调等调用
 * @note 此函数是线程安全的，可以在任意上下文（如 FreeRTOS 其他任务）中调用
 * ，判断Fabric是否存在，避免空转崩溃重启
 * @note 该函数会将重置请求投递到 Matter
 * 的主事件循环线程中异步执行，确保线程安全
 * @note 配网失败的防御机制：在核心重置函数中增加了对 Fail-Safe
 * 状态的强制解除和配网状态机的重置，确保即使处于配网失败的中间态也能安全退出，避免死锁和重启
 */
void MatterBridge::DoSoftNetworkResetHandler(intptr_t arg)
{
  LOG_MATTER("=============================================");
  LOG_MATTER("[SoftReset] 开始执行在线网络重置（不重启）...");
  LOG_MATTER("=============================================");

  // =================================================================
  // 1. 强行重置配网窗口状态
  // =================================================================
  LOG_MATTER("[SoftReset] 正在关闭并复位配网状态机...");

  // 无论当前配网是成功、失败、还是进行到中途，强行关闭配网窗口。
  // 这在 Matter 栈内部会自动释放与该配网周期关联的临时会话和未闭合的握手通道。
  chip::Server::GetInstance()
      .GetCommissioningWindowManager()
      .CloseCommissioningWindow();

  // =================================================================
  // 2. 安全检查：仅在真正配过网时才删除 Fabric
  // =================================================================
  bool hasFabrics = false;
  for (const auto& fabricInfo : chip::Server::GetInstance().GetFabricTable())
  {
    if (fabricInfo.GetFabricIndex() != chip::kUndefinedFabricIndex)
    {
      hasFabrics = true;
      LOG_MATTER("[Matter旧数据] 发现激活的 Fabric Index: 0x%X",
                 fabricInfo.GetFabricIndex());
    }
  }

  if (hasFabrics)
  {
    // 只有配过网才删除，防止未配网或配网失败中途空转导致内部迭代器断言 Crash
    chip::Server::GetInstance().GetFabricTable().DeleteAllFabrics();
    LOG_MATTER("[SoftReset] Matter Fabrics 已全部清除");
  }
  else
  {
    LOG_MATTER("[SoftReset] 未发现有效已激活 Fabric,跳过清除(安全拦截成功)");
  }

  // =================================================================
  // 3. 使用 OpenThread 原生底层机制清除 Thread 状态
  // =================================================================
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
  otInstance* otInst = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
  if (otInst != nullptr)
  {
    LOG_MATTER("[SoftReset] 正在清空 OpenThread 数据集...");
    // 显式让底层状态机断开连接，不向旧网关发送分离通知，直接抹除
    otThreadSetEnabled(otInst, false);
    otIp6SetEnabled(otInst, false);

    // 使用全 0 结构体覆盖，彻底洗干净 NVM 里的 Thread 凭证
    otOperationalDataset emptyDataset;
    std::memset(&emptyDataset, 0, sizeof(emptyDataset));
    emptyDataset.mComponents.mIsNetworkKeyPresent      = false;
    emptyDataset.mComponents.mIsExtendedPanIdPresent   = false;
    emptyDataset.mComponents.mIsPanIdPresent           = false;
    emptyDataset.mComponents.mIsChannelPresent         = false;
    emptyDataset.mComponents.mIsActiveTimestampPresent = false;

    otDatasetSetActive(otInst, &emptyDataset);
    LOG_MATTER("[SoftReset] OpenThread 数据集已强制覆盖清空");
  }
#endif

  // =================================================================
  // 4. 重新开启蓝牙广播，拉回初始配网状态
  // =================================================================
  chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
  LOG_MATTER("[SoftReset] 蓝牙配网广播已重新开启，等待新配网...");

  LOG_MATTER("=============================================");
  LOG_MATTER("[SoftReset] 在线网络重置完成！设备状态已安全归零。");
  LOG_MATTER("=============================================");
}

/**
 * @brief 供外部（如按键长按回调）调用的接口函数
 * @note 此函数是线程安全的，可以在任意上下文（如 FreeRTOS 其他任务）中调用
 */
void MatterBridge::TriggerNetworkResetWithoutReboot(void)
{
  // 将重置业务投递到 Matter 的主事件循环线程中异步执行，确保线程安全
  CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(
      DoSoftNetworkResetHandler, 0);

  if (err != CHIP_NO_ERROR)
  {
    LOG_MATTER("投递重置任务失败: %" CHIP_ERROR_FORMAT, err.Format());
  }
  else
  {
    LOG_MATTER("已成功将重置请求发送至 Matter 线程");
  }
}
