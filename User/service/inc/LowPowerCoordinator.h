/**
 * @file LowPowerCoordinator.h
 * @brief 应用层低功耗协调器：显式状态机 + 活动保持位
 * @author hady
 * @date 2026-07-30
 * @layer Service
 * @note 统一评估是否可 Suspend 外设；kGatePeripheralsOnIdle=false 时永不进 SleepPrepared。
 */
#pragma once

#include <cstdint>

/**
 * @class LowPowerCoordinator
 * @brief 空闲/活动收拢入口；外设开关只经 BspPeripheralSleep
 */
class LowPowerCoordinator {
 public:
  /** @brief 唤醒/保持原因（可按位或） */
  enum class HoldReason : uint8_t {
    None = 0U,
    MainLight = 1U << 0,
    Indicator = 1U << 1,
    Button = 1U << 2,
    UsbPower = 1U << 3,
    UserMatter = 1U << 4,
    Commissioning = 1U << 5,
    DebugForce = 1U << 7 /**< 调试强制保持 Active */
  };

  /** @brief 运行状态（仅本类 SetRunStateRaw 可改） */
  enum class RunState : uint8_t {
    Active = 0,   /**< 外设常备，可立即输出 */
    IdleSettle,   /**< 无保持源，等待 settle 后休眠 */
    SleepPrepared /**< 已 Suspend 外设（仅允许休眠时进入） */
  };

  static LowPowerCoordinator& Instance() {
    static LowPowerCoordinator instance;
    return instance;
  }

  /** @brief 初始化：强制 Active；门控关闭时置 DebugForce */
  void Init();

  /**
   * @brief 主灯物理输出变化
   * @param isActive true=有通道输出
   */
  void OnMainLightActivity(bool isActive);

  /**
   * @brief 指示灯是否有输出需求
   * @param isActive true=白/红任一需要渲染
   */
  void OnIndicatorActivity(bool isActive);

  /**
   * @brief USB 供电/充电相关活动
   * @param usbPresent true=USB 接入
   * @param allowPeripheralIo false=ISR 安全：只改保持位/状态字，不 Resume/Suspend 外设
   */
  void OnUsbPowerActivity(bool usbPresent, bool allowPeripheralIo = true);

  /**
   * @brief 用户交互（按键/Matter 本地操作）短暂拉起
   * @note 置 Button/UserMatter 保持位；由 Poll 递减超时后清除。
   */
  void RequestUserWake();

  /**
   * @brief Matter Hub 下行控制：Resume 外设 + 延长 Active（防 3s 空闲 Suspend SPI）
   * @note 比 RequestUserWake 保持更久，覆盖 ICD Active 窗口内的连续调光。
   */
  void RequestMatterControlWake();

  /**
   * @brief BLE 配网全程保持 Active（至 kCommissioningComplete / BLE 断开）
   * @param enable true=配网会话进行中
   */
  void SetCommissioningHold(bool enable);

  /**
   * @brief 周期驱动（建议挂 200ms 或电源同周期）
   * @param elapsedMs 距上次调用的间隔
   */
  void Poll(uint16_t elapsedMs);

  /** @brief 当前运行态（只读） */
  RunState GetRunState() const {
    return m_runState;
  }

  /** @brief 输出前确保外设可用（薄封装） */
  void EnsurePeripheralsReady();

 private:
  LowPowerCoordinator() = default;
  ~LowPowerCoordinator() = default;
  LowPowerCoordinator(const LowPowerCoordinator&) = delete;
  LowPowerCoordinator& operator=(const LowPowerCoordinator&) = delete;

  void SetHoldRaw(HoldReason reason, bool enable);
  void SetRunStateRaw(RunState nextState);
  void EvaluateSleepRaw();
  void EnterActiveRaw();
  void EnterSleepPreparedRaw();
  void TickUserWakeHoldRaw(uint16_t elapsedMs);
  bool HasBlockingHoldRaw() const;

  RunState m_runState{RunState::Active};
  uint8_t m_holdMask{0U};
  uint16_t m_idleElapsedMs{0U};
  uint16_t m_userWakeHoldMs{0U};
  uint16_t m_usbFallbackMs{0U}; /**< USB ADC 兜底累计 */
};
