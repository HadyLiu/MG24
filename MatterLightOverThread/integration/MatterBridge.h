/**
 * @file MatterBridge.h
 * @brief Matter 集成层：ZCL 属性上下行、配网窗口、Identify、软复位
 * @author hady
 * @date 2026-06-12
 * @layer Integration
 * @note 本地与 Matter 协议栈之间的唯一桥接点；上行属性写入需经 ScheduleWork 到
 * Matter 线程。
 */
#pragma once

#include "ColorLibrary.h"
#include "app/LightTypes.h"
#include <stdint.h>

/** @brief Matter 与本地 LightEngine 之间的唯一桥接模块 */
class MatterBridge
{
public:
  /** @brief 获取 MatterBridge 单例
   *  @return 单例引用 */
  static MatterBridge& instance();

  /**
   * @brief Matter 下行：开关/亮度动作
   * @param action  动作类型（ON/OFF/LEVEL）
   * @param value   亮度值指针（LEVEL 动作有效，可空）
   * @param lightOn 当前灯开关态（预留）
   * @return 无
   */
  void onActionInitiated(int action, uint8_t* value, bool lightOn);

  /**
   * @brief Matter 下行：颜色动作（HSV/CT/XY）
   * @param action    颜色动作类型
   * @param valueData 颜色数据指针
   * @param x         CIE x（XY 动作有效）
   * @param y         CIE y（XY 动作有效）
   * @return 无
   */
  void onColorEvent(uint8_t action, void* valueData, uint16_t x, uint16_t y);

  /**
   * @brief 上行 OnOff 属性
   * @param on 开关态
   * @return 无
   */
  void uploadOnOff(bool on);

  /**
   * @brief 上行亮度（百分比转 Matter level）
   * @param percent 亮度百分比 0~100
   * @return 无
   */
  void uploadBrightnessPercent(uint8_t percent);

  /**
   * @brief 上行颜色对应的近似色温(mireds)
   * @param cycleIndex 颜色循环索引 0~11
   * @return 无
   */
  void uploadColorFromCycleIndex(uint8_t cycleIndex);

  /** @brief 是否未加入任何 Fabric（未配网）
   *  @return true 未配网 */
  bool isUnprovisioned() const;

  /** @brief 打开基础配网窗口
   *  @return 无 */
  void openCommissioningWindow();

  /** @brief 关闭配网窗口（若已打开）
   *  @return 无 */
  void closeCommissioningWindow();

  /** @brief 触发软复位：仅清配网信息，不重启
   *  @return 无 */
  void triggerSoftNetworkReset();

  /** @brief 上行回调是否处于旁路（避免回环）
   *  @return true 旁路中 */
  bool isReportBypassEnabled() const;

  /** @brief 注册 Matter 设备事件与 Identify 集群
   *  @return 无 */
  void registerDeviceEvents();

  /** @brief 初始化 Identify 集群回调
   *  @return 无 */
  void initIdentifyCluster();

  /**
   * @brief 将 Matter level 钳位到最低 5（约 5 流明下限）
   * @param matterLevel 原始 Matter level（0~254）
   * @return 钳位后的 level
   */
  static uint8_t clampMatterLevel(uint8_t matterLevel);

private:
  MatterBridge() = default;
};
