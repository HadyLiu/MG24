/**
 * @file MatterBridgeServer.h
 * @brief Matter 业务服务层：邮箱消费 + Matter 栈 ScheduleWork 操作
 * @author hady
 * @date 2026-06-16
 * @layer Service
 * @note
 * 拥有 osMessageQueue；MailPoster 由 entry 注册给 MatterBridge。
 * 不感知 LightDecisionCenter；灯光下行/设备事件由 entry 注册回调注入。
 */
#pragma once

#include "MatterMailPacket.h"
#include <cstdint>

/**
 * @class MatterBridgeServer
 * @brief Matter 邮箱消费与 ZCL 上下行服务
 */
class MatterBridgeServer
{
public:
  /** @brief Matter 开关/亮度下行回调（entry 翻译为 LDC 命令） */
  using DownlinkActionHandler = void (*)(int action, uint8_t level, bool lightOn);
  /** @brief Matter 颜色下行回调（已转换为 WRGB 四通道） */
  using DownlinkColorHandler = void (*)(const uint16_t wrgb[4]);
  /** @brief 配网完成事件 */
  using CommissioningCompleteHandler = void (*)();
  /** @brief 识别开始/结束事件 */
  using IdentifyHandler = void (*)(bool active);
  /** @brief 入网 IP 地址变化事件 */
  using NetworkConnectedHandler = void (*)();

  /**
   * @brief 获取 Matter 服务单例
   * @return MatterBridgeServer 引用
   */
  static MatterBridgeServer& Instance()
  {
    static MatterBridgeServer instance;
    return instance;
  }

  /** @brief 初始化邮箱队列 */
  void Init();

  void RegisterDownlinkActionHandler(DownlinkActionHandler handler);
  void RegisterDownlinkColorHandler(DownlinkColorHandler handler);
  void RegisterCommissioningCompleteHandler(CommissioningCompleteHandler handler);
  void RegisterIdentifyHandler(IdentifyHandler handler);
  void RegisterNetworkConnectedHandler(NetworkConnectedHandler handler);

  /** @brief 邮件投递静态入口（注册给 MatterBridge::SetMailPoster） */
  static void MailPoster(const MatterMailMsg& msg);

  void Post(const MatterMailMsg& msg);
  void DrainMailQueue();

  bool IsReportBypassEnabled() const
  {
    return m_bypassZclCallback;
  }

  bool IsUnprovisioned() const;

  void SetReportBypass(bool enabled);
  void InitIdentifyCluster();

  void NotifyCommissioningComplete();
  void NotifyNetworkConnected();
  void NotifyIdentify(bool active);

private:
  MatterBridgeServer() = default;

  void EnsureQueue();
  void RequestDrain();
  void DispatchMail(const MatterMailMsg& msg);

  static void DrainWork(intptr_t ctx);

  void HandleDownlinkAction(int action, uint8_t level, bool lightOn);
  void HandleDownlinkColor(uint8_t action, uint8_t hue, uint8_t saturation,
                           uint16_t ctMireds, uint16_t x, uint16_t y);

  static uint8_t ClampMatterLevel(uint8_t matterLevel);

  DownlinkActionHandler m_downlinkAction{nullptr};
  DownlinkColorHandler m_downlinkColor{nullptr};
  CommissioningCompleteHandler m_commissioningComplete{nullptr};
  IdentifyHandler m_identify{nullptr};
  NetworkConnectedHandler m_networkConnected{nullptr};

  bool m_bypassZclCallback{false};
  bool m_drainScheduled{false};
};
