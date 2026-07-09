/**
 * @file PowerServer.h
 * @brief 电源服务：电池放电、USB 充电、电量监测
 * @author hady
 * @date 2026-06-25
 * @layer Service
 *
 * @details 设计思想
 * 1. 五段分区（见 PowerServer.cpp）：定时器 / BSP 桥接 / 回调注册 / 对外 API /
 *    主逻辑。
 * 2. 显式状态机 PowerRunState：转换集中在 SetRunStateRaw，Poll 统一 switch。
 * 3. Settle 防抖与 8h 充电会话计时独立维护，不与 GPIO 写入耦合。
 * 4. 单入口收拢：FetchPowerMonitorSnapshotRaw() 是唯一 BSP 读入口。
 * 5. 充电评估：EvaluateChargeRaw() 一次算出综合状态与充电使能；指示灯由 IndicatorServer 仲裁。
 */
#pragma once

#include "BspPowerMonitor.h"
#include "BspTimer.h"
#include "LightDecisionTypes.h"
#include "LightEffectEngine.h"
#include <cstdint>

/**
 * @class PowerServer
 * @brief 电源管理单例服务
 * @note 上游：entry 注册回调并注入主灯状态；下游：BspPowerMonitor 硬件控制。
 */
class PowerServer
{
  public:
    /** @brief 电池电压等级变化回调（→ entry → LightDecisionCenter） */
    using BatteryVoltHandler = void (*)(BatteryVoltLevel level);

    /**
     * @brief 充电综合状态变化回调（→ IndicatorServer 仲裁指示灯）
     * @note PowerServer 仅上报充电快照，灯效仲裁由 IndicatorServer 负责。
     */
    using ChargeStatusHandler = void (*)(const BatteryChargeSnapshot& snapshot);

    /** @brief 供电通路就绪回调（→ entry 刷新主灯 PWM） */
    using LightPowerPathReadyHandler = void (*)();

    static PowerServer& Instance()
    {
        static PowerServer instance;
        return instance;
    }

    /** @brief 初始化：注册回调、首次 Fetch、推导状态机、按需启停定时器 */
    void Init();

    /** @brief 去初始化：停定时器，关充电/放电，熄灭充电指示灯 */
    void DeInit();

    /** @brief 注册电池电量等级变化回调 */
    void RegisterBatteryVoltHandler(BatteryVoltHandler handler);

    /** @brief 注册充电指示灯状态回调（USB 模式下补发当前快照） */
    void RegisterChargeStatusHandler(ChargeStatusHandler handler);

    /** @brief 注册供电通路就绪回调 */
    void RegisterLightPowerPathReadyHandler(LightPowerPathReadyHandler handler);

    /**
     * @brief 主灯物理输出状态变化通知
     * @param mainLightActive true=有通道输出, false=全灭
     */
    void OnLightOutputChanged(bool mainLightActive);

    /**
     * @brief USB 拔插中断入口（薄封装）
     * @param usbStatus BSP 上报的 USB 连接状态
     */
    void PowerUsbIsr(UsbConnectionStatusEnum usbStatus);

  private:
    /** @brief 供电来源（由 USB 是否接入决定） */
    enum class SupplyMode : uint8_t
    {
        Battery = 0, /**< 电池供电 */
        UsbPowered   /**< USB 供电，不对外放电 */
    };

    /**
     * @brief 运行状态机（PowerPoll 唯一 switch 分发入口）
     * @note 禁止直接赋值 m_runState，须经 SetRunStateRaw。
     */
    enum class PowerRunState : uint8_t
    {
        BatteryIdle = 0,    /**< 电池 + 灯灭：关放电，停轮询 */
        BatteryDischarging, /**< 电池 + 灯亮：开放电，轮询电压 */
        UsbChargeManage     /**< USB：充电仲裁 + 指示灯上报 */
    };

    /**
     * @brief 充电综合评估结果（状态 + 硬件使能，一次算出）
     * @note 充电使能与综合状态同源；指示灯仲裁见 IndicatorServer。
     */
    struct ChargeEvaluation
    {
        BatteryChargeStatus status;         /**< 充电综合状态 */
        bool                allowCharge;    /**< 是否允许开充，由 status 推导 */
        uint8_t             fastChargeFlag; /**< 1=快充(灯灭), 0=慢充(灯亮) */
    };

    /**
     * @brief BSP 电源监测快照（仅 FetchPowerMonitorSnapshotRaw 写入）
     * @note 电池模式仅填充 usbStatus/按需 voltStatus；USB 模式填充全部字段。
     */
    struct alignas(1) PowerMonitorSnapshot
    {
        UsbConnectionStatusEnum usbStatus{UsbConnectionStatusEnum::UsbNotConnected};
        BatteryTempStatusEnum   tempStatus{BatteryTempStatusEnum::TEMP_NORMAL};
        BatteryVoltStatusEnum   voltStatus{BatteryVoltStatusEnum::VOLT_NORMAL};
        ChargeChipStatusEnum    chipStatus{ChargeChipStatusEnum::CHARGE_INIT};
        bool                    chargeEnabled{false};
    };

    PowerServer()                              = default;
    ~PowerServer()                             = default;
    PowerServer(const PowerServer&)            = delete;
    PowerServer& operator=(const PowerServer&) = delete;

    // ---- §1 定时器 ----
    void StartPollTimerRaw(bool enable);
    void SyncPollTimerFromRunStateRaw();
    void PowerPoll(uint16_t elapsedMs);

    // ---- §2 BSP 桥接（读：唯一漏斗；写：独立出口）----
    void                        FetchPowerMonitorSnapshotRaw();
    const PowerMonitorSnapshot& GetPowerSnapshotRaw() const;
    bool                        IsChargeChipReadableNowRaw(const PowerMonitorSnapshot& snapshot) const;
    void                        SetBatteryOutEnableRaw(bool enable);
    void                        SetBatteryChargeEnableRaw(bool enable, uint8_t fastCharge);

    // ---- §5 主逻辑：状态机 ----
    void SetRunStateRaw(PowerRunState nextState);
    void RefreshRunStateRaw();
    void RequestSupplyApplyRaw();
    void ApplySupplyModeHardwareRaw();

    void        TickBatteryIdleRaw(uint16_t elapsedMs);
    void        TickBatteryDischargingRaw(uint16_t elapsedMs);
    void        TickUsbChargeManageRaw(uint16_t elapsedMs);
    static void TickSettleCountdownRaw(uint16_t& counterMs, uint16_t elapsedMs);

    void OnMainLightChangedRaw(bool mainLightActive);
    void OnUsbConnectionChangedRaw(UsbConnectionStatusEnum usbStatus);

    void ApplyBatteryDischargeRaw();
    void EnableBatteryDischargeRaw();
    void DisableBatteryDischargeRaw();

    void PollBatteryVoltRaw();
    void NotifyPowerPathReadyRaw();

    static BatteryChargeStatus ResolveChargeStatusFromSnapshotRaw(const PowerMonitorSnapshot& snapshot, bool chipValid,
                                                                  bool chargeFaultLatched, bool chargeSessionTimeout);
    bool                       ShouldLatchChargeFaultRaw(const PowerMonitorSnapshot& snapshot) const;
    ChargeEvaluation           EvaluateChargeRaw(const PowerMonitorSnapshot& snapshot) const;
    static bool                DeriveAllowChargeFromStatusRaw(BatteryChargeStatus status);
    void ApplyChargeSnapshotFromEvalRaw(const ChargeEvaluation& eval, const PowerMonitorSnapshot& snapshot,
                                        bool chipValid);
    void UpdateChargeSettleAfterDecisionRaw(bool allowCharge, bool wasChargeEnabled);
    void RefreshChargeSnapshotRaw();
    void UpdateChargeControlAndNotifyRaw();

    static BatteryVoltLevel MapVoltLevelRaw(BatteryVoltStatusEnum status);

    void TickChargingSessionRaw(uint16_t elapsedMs);
    void ResetChargingSessionRaw();
    void ClearChargeIndicatorRaw();
    void EvaluateChargeStatusRaw();
    void UpdateChargeControlRaw();

    static void PollTimerBridgeImpl(uint16_t elapsedMs);
    static void UsbNotifyBridgeImpl(UsbConnectionStatusEnum usbStatus);

    // ---- 回调指针 ----
    BatteryVoltHandler         m_batteryVoltHandler{nullptr};
    ChargeStatusHandler        m_chargeStatusHandler{nullptr};
    LightPowerPathReadyHandler m_powerPathReadyHandler{nullptr};

    // ---- 状态机 ----
    SupplyMode    m_supplyMode{SupplyMode::Battery};
    PowerRunState m_runState{PowerRunState::BatteryIdle};

    // ---- 单入口快照（防多读冲突）----
    PowerMonitorSnapshot m_powerSnapshot{};           /**< USB */
    bool                 m_powerSnapshotValid{false}; /**< 快照是否有效 */
    bool                 m_fetchInProgress{false};    /**< Fetch 重入保护 */

    // ---- 上报基准 ----
    BatteryVoltLevel      m_lastReportedVoltLevel{BatteryVoltLevel::Normal};
    BatteryChargeSnapshot m_lastReportedChargeSnapshot{};
    BatteryChargeSnapshot m_currentChargeSnapshot{};

    // ---- 运行标志 ----
    bool m_pollTimerEnabled{false};
    bool m_mainLightActive{false};
    bool m_batteryOutEnabled{false};
    bool m_batteryVoltPollEnabled{false};
    bool m_pendingSupplyApply{false}; /**< ISR 后待 Poll 刷新硬件 */
    bool m_chargeSnapshotValid{false};
    bool m_chargeSessionTimeout{false}; /**< 8h 超时视为充满 */
    bool m_chargeFaultLatched{false};   /**< 充电故障锁存 */

    // ---- 计时器（与硬件控制隔离）----
    uint32_t m_chargeSessionElapsedMs{0U};
    uint16_t m_batterySettleMs{0U}; /**< 开放电后延迟读电压 */
    uint16_t m_chargeSettleMs{0U};  /**< 开充后延迟读芯片 */

    static constexpr uint16_t kPollIntervalMs       = 200U;
    static constexpr uint16_t kBatterySettleMs      = 500U;
    static constexpr uint16_t kChargeSettleMs       = 300U;
    static constexpr uint32_t kChargeSessionLimitMs = 8U * 60U * 60U * 1000U;
};
