/**
 * @file IndicatorServer.h
 * @brief 指示灯管理服务：聚合多源输入并仲裁白/红输出
 * @author hady
 * @date 2026-07-09
 * @layer Service
 * @note 充电状态、低电量警告等输入在此统一仲裁，再驱动 IndicatorEffectEngine。
 *       上层仅投递快照/事件，不直接操作灯效引擎。
 */
#pragma once

#include "LightDecisionTypes.h"
#include <cstdint>

/**
 * @class IndicatorServer
 * @brief 指示灯输出仲裁与下发
 * @note 白呼吸与故障红闪可叠加；各输入源独立维护，ApplyOutputRaw 统一落地。
 */
class IndicatorServer
{
  public:
    static IndicatorServer& Instance()
    {
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

    /** @brief 低电量/临界电量警告（预留：短时红闪提示） */
    void OnBatteryLowWarn();

    /** @brief 熄灭全部指示灯并复位仲裁状态 */
    void StopAll();

    /** @brief 查询当前已下发的灯效位标志 */
    ChargeIndicatorEffect GetAppliedEffects() const;

  private:
    IndicatorServer()                                  = default;
    ~IndicatorServer()                                 = default;
    IndicatorServer(const IndicatorServer&)            = delete;
    IndicatorServer& operator=(const IndicatorServer&) = delete;

    /** @brief 由充电快照推导灯效请求（位标志，可叠加） */
    ChargeIndicatorEffect ArbitrateChargeEffectsRaw(const BatteryChargeSnapshot& snapshot) const;

    /** @brief 聚合各输入源，得到最终灯效位标志 */
    ChargeIndicatorEffect ArbitrateOutputRaw() const;

    /** @brief 将仲裁结果同步至 IndicatorEffectEngine */
    void ApplyOutputRaw(ChargeIndicatorEffect effects);

    /** @brief 充电输入变化时重新仲裁并下发 */
    void RefreshFromChargeInputRaw();

    static constexpr uint8_t kDefaultWhiteBreathBrightness = 153U;

    BatteryChargeSnapshot m_chargeInput{};
    bool                  m_chargeInputValid{false};
    ChargeIndicatorEffect m_chargeEffects{ChargeIndicatorEffect::Off};
    ChargeIndicatorEffect m_lastApplied{ChargeIndicatorEffect::Off};
};
