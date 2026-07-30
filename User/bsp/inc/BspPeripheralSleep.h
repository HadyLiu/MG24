/**
 * @file BspPeripheralSleep.h
 * @brief BSP 外设休眠门面：Suspend/Resume 唯一漏斗
 * @author hady
 * @date 2026-07-30
 * @layer BSP
 * @note 禁止业务层直接 CMU_ClockEnable 关 SPI/EUSART。
 *       当前实现以安全占位为主；真正关时钟须在充分回归后补全。
 */
#pragma once

#include <cstdint>

/**
 * @class BspPeripheralSleep
 * @brief 主灯总线 / 指示灯 PWM / 采样通路的休眠收拢入口
 */
class BspPeripheralSleep
{
  public:
    static BspPeripheralSleep& Instance()
    {
        static BspPeripheralSleep instance;
        return instance;
    }

    /** @brief 上电后标记外设已就绪（不重复 Init 硬件） */
    void MarkReadyAfterBoot();

    /**
     * @brief 进入休眠前关外设（唯一 Suspend 入口）
     * @note 须由 LowPowerCoordinator 在允许休眠时调用。
     */
    void SuspendForSleep();

    /**
     * @brief 退出休眠后恢复外设（唯一 Resume 入口）
     * @note 开灯/按键/USB 等唤醒路径调用前须保证已 Resume。
     */
    void ResumeFromSleep();

    /** @brief 总线是否处于挂起（未 Resume） */
    bool IsSuspended() const
    {
        return m_isSuspended;
    }

    /**
     * @brief 输出前确保总线可用（单入口防护）
     * @note 若曾 Suspend，则先 Resume；常备模式下为空操作。
     */
    void EnsureReadyBeforeOutput();

  private:
    BspPeripheralSleep()                                   = default;
    ~BspPeripheralSleep()                                  = default;
    BspPeripheralSleep(const BspPeripheralSleep&)            = delete;
    BspPeripheralSleep& operator=(const BspPeripheralSleep&) = delete;

    void SuspendMainLightBusRaw();
    void ResumeMainLightBusRaw();
    void SuspendIndicatorPwmRaw();
    void ResumeIndicatorPwmRaw();
    void SuspendIadcClockRaw();
    void ResumeIadcClockRaw();

    bool m_isSuspended{false};
    bool m_bootMarked{false};
};
