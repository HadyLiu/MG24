/**
 * @file PowerMonitor.h
 * @brief 电源监控服务：ADC/电池状态/充电 tick 封装
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 吸收原 powerManage 全部逻辑，向 BatteryPolicy 提供面向对象访问。
 */
#pragma once

#include "app/PowerConfig.h"
#include <stdint.h>

/** @brief 电源与电池采样、充电状态机服务 */
class PowerMonitor
{
public:
  /** @brief 获取 PowerMonitor 单例
   *  @return 单例引用 */
  static PowerMonitor& instance();

  /** @brief 初始化 ADC（上电或唤醒后）
   *  @return 无 */
  void adcInit();

  /** @brief 关闭 ADC（进入低功耗前）
   *  @return 无 */
  void adcDeInit();

  /** @brief 初始化电源监控状态
   *  @return 无 */
  void init();

  /**
   * @brief 到达采样间隔时执行一次电池采样
   * @param tick10ms     当前全局 10ms tick 计数
   * @param interval10ms 采样间隔（单位 10ms，默认 100=1s）
   * @return 无
   */
  void tickBatteryIfDue(uint32_t tick10ms, uint32_t interval10ms = 100);

  /**
   * @brief 充电态 10ms tick（充电电流/时长/状态机）
   * @param tick10ms 当前全局 10ms tick 计数
   * @return 无
   */
  void tickCharge(uint32_t tick10ms);

  /** @brief 采样并更新上游电源检测状态
   *  @return 无 */
  void refreshUpstreamPower();

  /** @brief 处理外部/电池电源切换
   *  @return 无 */
  void powerSwitchAssignment();

  /** @brief 使能电池输出
   *  @return 无 */
  void batOutEnable();

  /** @brief 关闭电池输出
   *  @return 无 */
  void batOutDisable();

  /**
   * @brief 根据充电状态与主灯开关控制充电电流
   * @param lightOn 主灯是否点亮
   * @return 无
   */
  void chargeCurrentCtrl(bool lightOn);

  /** @brief 当前是否接入外部电源
   *  @return true 已接入外部电源 */
  bool powerStatus() const;

  /** @brief 上一周期外部电源状态
   *  @return true 上周期已接入 */
  bool upstreamPowerStatus() const;

  /** @brief 当前电池/充电状态枚举值
   *  @return 状态码 */
  uint8_t batteryStatus() const;

  /** @brief 当前电池/充电状态枚举
   *  @return TypedefBatEnum */
  TypedefBatEnum batteryStatusEnum() const;

  /** @brief 当前电池电压
   *  @return 电压（mV） */
  uint32_t batteryVoltageMv() const;

  /** @brief 是否处于低电锁灯保护
   *  @return true 已锁灯保护 */
  bool isPowerProtect() const;

  /**
   * @brief 设置低电锁灯保护标志
   * @param v true=置保护，false=解除
   * @return 无
   */
  void setPowerProtect(bool v);

private:
  bool m_powerProtect = false;

  void powerManageInit();
  void getDisChargeStatus();
  void chargeTimeUpdate();
  bool chargeDetect();
  void chargeLogic(bool receptionStatus);
  bool detectTemp();
  uint16_t getPowerInAdcVol();
  uint16_t getBatAdcVol();
  uint16_t getTempAdcVol();
};
