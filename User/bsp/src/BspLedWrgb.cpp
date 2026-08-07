
/**
 * @file BspLedWrgb.cpp
 * @brief 主灯 WRGB LED BSP 实现
 * @author hady
 * @date 2026-06-12
 * @layer BSP
 * @note 封装 SM15135E SPI 驱动，提供 WRGB PWM 设置接口；由 HalPwm
 * 调用，Service 层禁止直接 include。
 */
#include "BspLedWrgb.h"
#include "BspPeripheralSleep.h"

/* 构造函数 */
BspLedWrgb::BspLedWrgb() : whitePwm_(), rgb_()
{
    // 构造函数体内部可以留空，或者做其他事情
}

/**
 * @brief 初始化 SM15135E 与 SPI 总线
 * @return 无
 * @note 设置默认电流增益 Gain_101_1MA
 */
void BspLedWrgb::Init()
{
    whitePwm_.Init(&sl_pwm_w_led0);
    rgb_.Init();
    rgb_.SetAllGain(HalSpiSm15135e::CurrentGain::Gain_101_1MA);
}

/**
 * @brief 设置主灯 WRGB 四通道 PWM（0~1023）
 * @param w 白光 0~1023
 * @param r 红光 0~1023
 * @param g 绿光 0~1023
 * @param b 蓝光 0~1023
 * @return 无
 * @note RGB 左移 6 位映射至 SM15135E 16-bit灰度；满量程时置 0xFFFF；硬件设计
 * b 和 r 交换
 */
void BspLedWrgb::LedWrgbSetDuty(uint16_t w, uint16_t r, uint16_t g, uint16_t b)
{
    /* 休眠兼容：若总线曾 Suspend，输出前先 Resume */
    BspPeripheralSleep::Instance().EnsureReadyBeforeOutput();

    ApplyDutyRaw(w, r, g, b);
}

/**
 * @brief 空闲挂起主灯总线
 */
void BspLedWrgb::SuspendBusForIdle()
{
    /* 直接下发，避免再走 EnsureReady（Suspend 过程中尚未置位） */
    ApplyDutyRaw(0U, 0U, 0U, 0U);
}

/**
 * @brief 活动恢复主灯总线
 * @note 不用 Init()（内含 osDelay），避免在 sleeptimer 回调里阻塞。
 */
void BspLedWrgb::ResumeBusForActive()
{
    rgb_.SendReset();
    rgb_.SetAllGain(HalSpiSm15135e::CurrentGain::Gain_101_1MA);
}

/**
 * @brief 写白光 PWM + SPI RGB 帧
 */
void BspLedWrgb::ApplyDutyRaw(uint16_t w, uint16_t r, uint16_t g, uint16_t b)
{
    uint8_t rgbMaxPwmBits;
    rgbMaxPwmBits = rgb_.GetMaxPwmBits();

    uint16_t sm_r = (r << (rgbMaxPwmBits - kMaxPwmBits));
    uint16_t sm_g = (g << (rgbMaxPwmBits - kMaxPwmBits));
    uint16_t sm_b = (b << (rgbMaxPwmBits - kMaxPwmBits));

    if (r >= kMaxPwmValue)
    {
        sm_r = rgb_.GetMaxPwmValue();
    }
    if (g >= kMaxPwmValue)
    {
        sm_g = rgb_.GetMaxPwmValue();
    }
    if (b >= kMaxPwmValue)
    {
        sm_b = rgb_.GetMaxPwmValue();
    }
    whitePwm_.PwmSetDuty(w);

    /* 硬件设计 b 和 r 交换 */
    rgb_.SetAllGain(HalSpiSm15135e::CurrentGain::Gain_70_8MA);
    rgb_.SetRgbwyDuty(sm_b, sm_g, sm_r, 0, 0);
    rgb_.SendFrame();
}
