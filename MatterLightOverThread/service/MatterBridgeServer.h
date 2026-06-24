///**
// * @file MatterBridgeServer.h
// * @brief Matter 业务服务层：邮箱消费 + Matter 栈 ScheduleWork 操作
// * @author hady
// * @date 2026-06-16
// * @layer Service
// * @note
// * 对外仅暴露两个数据接口，数据只能经此二者进出：
// *   - UploadState：唯一上报(输入)，LDC 灯光状态(开关/亮度/WRGB) → Matter；
// *   - RegisterDownlinkHandler：唯一下行(输出)，Matter
// 数据(开关/亮度/WRGB/识别)
// * → LDC。 其余邮箱/栈操作/状态查询全部私有；MatterBridge 的投递与查询接线在
// * Init() 内部完成， 上层无需感知。
// */
// #pragma once
//
// #include "MatterMailPacket.h"
// #include <cstdint>
//
// namespace chip {
// namespace DeviceLayer {
// struct ChipDeviceEvent;
//} // namespace DeviceLayer
//} // namespace chip
//
///**
// * @class MatterBridgeServer
// * @brief Matter 邮箱消费与 ZCL 上下行服务（对外仅两个数据接口）
// */
// class MatterBridgeServer
//{
// public:
//  /**
//   * @brief Matter 下行事件类型
//   * @note 单一下行回调通过本字段区分语义，避免多个注册回调。
//   */
//  enum class DownlinkKind : uint8_t
//  {
//    kLightControl = 0,  /**< 灯光控制：on / brightness / wrgb 有效 */
//    kIdentify,          /**< 识别开始/结束：on 表示 active */
//    kCommissioningDone, /**< 配网完成 */
//    kNetworkConnected   /**< 入网就绪：请求全量回报 */
//  };
//
//  /**
//   * @brief Matter 下行统一载荷（→ LightDecisionCenter）
//   * @note kLightControl 始终携带补齐后的完整状态(开关+亮度+WRGB)。
//   */
//  struct DownlinkData
//  {
//    DownlinkKind kind;  /**< 事件类型 */
//    bool on;            /**< 开关 / 识别 active */
//    uint8_t brightness; /**< 目标亮度 0~255 */
//    uint16_t wrgb[4];   /**< 逻辑 WRGB 0~1023 */
//  };
//
//  /** @brief 唯一下行回调：Matter 数据 → LightDecisionCenter（entry 绑定） */
//  using DownlinkHandler = void (*)(const DownlinkData& data);
//
//  /**
//   * @brief 获取 Matter 服务单例
//   * @return MatterBridgeServer 引用
//   */
//  static MatterBridgeServer& Instance()
//  {
//    static MatterBridgeServer instance;
//    return instance;
//  }
//
//  /**
//   * @brief 初始化：创建邮箱并自接线 MatterBridge（投递/查询/设备事件）
//   * @return 无
//   */
//  void Init();
//
//  /**
//   * @brief 接口①·上报(输入)：开关/亮度/WRGB 一次性上报 Matter
//   * @param on         是否点亮
//   * @param brightness 亮度 0~255
//   * @param pWrgb      逻辑 WRGB 四通道（可空，空则不更新颜色）
//   * @return 无
//   * @note 由 entry 接 LDC 上报回调；内部转 HSV 后入队，CHIP
//   任务上下文写属性。
//   */
//  void UploadState(bool on, uint8_t brightness, const uint16_t* pWrgb);
//
//  /**
//   * @brief 接口②·下行(输出)：注册唯一下行回调
//   * @param handler Matter 数据 → LightDecisionCenter 的回调
//   * @return 无
//   */
//  void RegisterDownlinkHandler(DownlinkHandler handler);
//
// private:
//  MatterBridgeServer() = default;
//
//  /** @brief 邮件投递静态入口（Init 内部注册给 MatterBridge::SetMailPoster）
//  */
//  // static void MailPoster(const MatterMailMsg& msg);
//
//  void EnsureQueue();
//  // void Post(const MatterMailMsg& msg);
//  void RequestDrain();
//  static void DrainWork(intptr_t ctx);
//  void DrainMailQueue();
//  // void DispatchMail(const MatterMailMsg& msg);
//
//  void EmitDownlink(const DownlinkData& data);
//  void HandleDownlinkAction(int action, uint8_t level, bool lightOn);
//  void HandleDownlinkColor(uint8_t action, uint8_t hue, uint8_t saturation,
//                           uint16_t ctMireds, uint16_t x, uint16_t y);
//
//  /* Matter 栈操作：均经 ScheduleWork 在 CHIP 任务上下文执行 */
//  static void SafeUploadState(intptr_t ctx);
//  static void DoOpenCommissioningWindow(intptr_t);
//  static void DoCloseCommissioningWindow(intptr_t);
//  static void DoSoftNetworkResetHandler(intptr_t);
//  static void DoRegisterDeviceEvents(intptr_t);
//  static void
//  OnMatterDeviceEvent(const chip::DeviceLayer::ChipDeviceEvent* event,
//                      intptr_t arg);
//  void InitIdentifyCluster();
//
//  bool IsReportBypassEnabled() const
//  {
//    return m_bypassZclCallback;
//  }
//  bool IsUnprovisioned() const;
//  void SetReportBypass(bool enabled);
//
//  DownlinkHandler m_downlink{nullptr};
//
//  /** @brief 下行合并缓存：动作不带颜色 / 颜色不带亮度时补齐为完整状态 */
//  bool m_cacheOn{false};
//  uint8_t m_cacheBrightness{128U};
//  uint16_t m_cacheWrgb[4]{1023U, 0U, 0U, 0U};
//
//  bool m_bypassZclCallback{false};
//  bool m_drainScheduled{false};
//};
//

#pragma once

#include "MatterMailPacket.h"
#include <cstdint>

class MatterBridgeServer
{
public:
  /**
   * @brief Matter 下行事件类型
   * @note 单一下行回调通过本字段区分语义，避免多个注册回调。
   */
  enum class DownlinkKind : uint8_t
  {
    kLightControl = 0,  /**< 灯光控制：on / brightness / wrgb 均有效 */
    kIdentify,          /**< 识别开始/结束：on 表示 active */
    kCommissioningDone, /**< 配网完成 */
    kNetworkConnected   /**< 入网就绪：请求全量回报 */
  };

  /**
   * @brief Matter 下行统一载荷（→ LightDecisionCenter）
   * @note kLightControl 始终携带补齐后的完整状态(开关 + 亮度 + WRGB)。
   */
  struct DownlinkData
  {
    DownlinkKind kind;  /**< 事件类型 */
    bool on;            /**< 开关 / 识别 active */
    uint8_t brightness; /**< 目标亮度 0~255 */
    uint16_t wrgb[4];   /**< 逻辑 WRGB 0~1023 */
  };

  /** @brief 唯一下行回调：Matter 数据 → LightDecisionCenter（entry 绑定） */
  using DownlinkHandler = void (*)(const DownlinkData& data);

  static MatterBridgeServer& Instance()
  {
    static MatterBridgeServer instance;
    return instance;
  }
  void Init();

  /** @brief 注册唯一下行回调（由 entry 接线至 LightDecisionCenter） */
  void RegisterDownlinkHandler(DownlinkHandler handler);

  // 读取开关/亮度/WRGB 下发给lighting
  void OnMatterDataReceived(MatterDownlinkUploadPayload data);

private:
  MatterBridgeServer()  = default;
  ~MatterBridgeServer() = default;

  /** @brief 用缓存的完整状态构造灯控载荷并发出 */
  void EmitLightControlRaw();

  /** @brief 调用已注册的下行回调（空则丢弃） */
  void EmitDownlinkRaw(const DownlinkData& data);

  DownlinkHandler m_downlink{nullptr};

  /** @brief 下行合并缓存：动作不带颜色 / 颜色不带亮度时补齐为完整状态 */
  bool m_cacheOn{false};
  uint8_t m_cacheBrightness{255U};
  uint16_t m_cacheWrgb[4]{1023U, 0U, 0U, 0U};
};
