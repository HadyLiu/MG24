/**
 * @file IndicatorServer.h
 * @brief 指示灯管理服务：聚合多源输入并仲裁白/红输出
 * @author hady
 * @date 2026-07-09
 * @layer Service
 * @note 白灯与红灯独立仲裁：充电白呼吸可与配网/低电量红闪时序叠加。
 */
#pragma once

#include "IndicatorEffectEngine.h"
#include "LightDecisionTypes.h"
#include <cstdint>

/**
 * @class IndicatorServer
 * @brief 指示灯输出仲裁与下发
 * @note 白通道：充电呼吸 或 首次配网呼吸；红通道背景为故障红闪，配网/低电量为红灯覆盖时序。
 */
class IndicatorServer {
 public:
  static IndicatorServer& Instance() {
    static IndicatorServer instance;
    return instance;
  }

  /** @brief 初始化 BSP 指示灯与 IndicatorEffectEngine */
  void Init();

  /**
   * @brief 消费 PowerServer 充电综合快照，更新充电侧灯效请求
   * @param snapshot 仲裁后的充电状态 + 芯片充电标志
   */
  void OnChargeSnapshot(const BatteryChargeSnapshot& snapshot);

  /** @brief 低电量警告：红灯一次性时序，白灯不受影响 */
  void OnBatteryLowWarn();

  /** @brief 配网重置：红灯与主灯同步（熄灭400ms→正常闪×3→慢闪×1→熄灭2s） */
  void OnNetConfigIndicatorStart();

  /** @brief 配网结束：仅停止红灯覆盖并恢复故障红闪背景 */
  void OnNetConfigIndicatorStop();

  /** @brief 首次出厂配网：系统 LED 白呼吸开始（§3.2） */
  void OnFirstCommissionBreathStart();

  /**
   * @brief 配网成功或任意按键后：停止白呼吸（§3.2）
   * @note 持久化 Dismiss 由 LDC 负责；本函数仅本会话熄灭并抑制重启
   */
  void OnFirstCommissionBreathStop();

  /** @brief 熄灭全部指示灯并复位仲裁状态 */
  void StopAll();

  /** @brief 查询当前充电侧背景灯效位标志 */
  ChargeIndicatorEffect GetAppliedEffects() const;

  /** @brief 红灯覆盖时序自然播完入口（Init 内注册至 IndicatorEffectEngine） */
  void OnRedOverrideSequenceFinishedRaw();

 private:
  IndicatorServer() = default;
  ~IndicatorServer() = default;
  IndicatorServer(const IndicatorServer&) = delete;
  IndicatorServer& operator=(const IndicatorServer&) = delete;

  /** @brief 由充电快照推导灯效请求（位标志，可叠加） */
  ChargeIndicatorEffect ArbitrateChargeEffectsRaw(const BatteryChargeSnapshot& snapshot) const;

  /** @brief 聚合各输入源背景灯效（当前仅充电侧） */
  ChargeIndicatorEffect ArbitrateBackgroundEffectsRaw() const;

  /** @brief 下发白通道：充电呼吸或首次配网呼吸 */
  void ApplyWhiteChannelRaw();

  /** @brief 下发红通道背景：故障红闪（红灯未被覆盖时） */
  void ApplyRedBackgroundRaw(bool enableBlink);

  /** @brief 充电输入变化后刷新白/红背景 */
  void RefreshFromChargeInputRaw();

  /** @brief 启动红灯覆盖时序（仅占用红通道） */
  void StartRedOverrideSequenceRaw(const IndicatorEffectEngine::BlinkSequenceStep* steps, uint8_t count,
                                   bool loopForever);

  /** @brief 红灯覆盖结束：恢复充电侧红闪背景 */
  void ResumeRedBackgroundRaw();

  static constexpr uint8_t kDefaultWhiteBreathBrightness = 153U;

  BatteryChargeSnapshot m_chargeInput{};
  bool m_chargeInputValid{false};
  ChargeIndicatorEffect m_chargeEffects{ChargeIndicatorEffect::Off};
  bool m_redOverrideActive{false};
  bool m_redOverrideLoopForever{false};
  bool m_firstCommissionBreathActive{false};
  /** @brief §3.2：用户按键或配网成功后不再自动重启首次白呼吸 */
  bool m_firstCommissionBreathSuppressed{false};
};
