/**
 * @file led_pwm.cpp
 * @brief 主灯 W 通道与指示灯 PWM 底层驱动实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 */
#include "led_pwm.h"
#include "SpiSm15135eHal.h"

extern sl_pwm_instance_t sl_pwm_w_led0;
extern sl_pwm_instance_t sl_pwm_Indic_led0;

SpiSm15135eHal& rgbDriverInstance();

/**
 * @brief 按照千分比(0~1000)设置PWM占空比
 * @param pwm PWM实例指针
 * @param duty_val 占空比数值，范围 0 到 1023（512代表50%）
 */
void my_pwm_set_duty_cycle_10bit_resolution(sl_pwm_instance_t* pwm,
                                            uint16_t duty_val)
{
  if (duty_val >= 1023)
  {
    duty_val = 1023; // 限幅防越界
  }

#if defined(_SILICON_LABS_32B_SERIES_2)
  // 适用于 Series 2 芯片 (如 EFR32MG24)
  uint32_t top         = TIMER_TopGet(pwm->timer);
  uint32_t compare_val = (top * duty_val); // 右移9位相当于除以512
  compare_val /= 638;
  TIMER_CompareBufSet(pwm->timer, pwm->channel, compare_val);
#else
  // 适用于 Series 3 芯片
  uint32_t top = sl_hal_timer_get_top(pwm->timer);
  // 右移10位相当于除以1024
  sl_hal_timer_channel_set_compare_buffer(pwm->timer, pwm->channel,
                                          (top * duty_val) >> 10);
#endif
}

/**
 * @brief 获取当前的千分比占空比
 * @return 返回 0 ~ 1023 的数值
 */
uint16_t my_pwm_get_duty_cycle_10bit_resolution(sl_pwm_instance_t* pwm)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
  uint32_t top     = TIMER_TopGet(pwm->timer);
  uint32_t compare = TIMER_CaptureGet(pwm->timer, pwm->channel);
#else
  uint32_t top     = sl_hal_timer_get_top(pwm->timer);
  uint32_t compare = sl_hal_timer_channel_get_compare(pwm->timer, pwm->channel);
#endif
  if (top == 0)
  {
    return 0;
  }
  return (uint16_t)((compare * 1023) / top);
}

void LED_W_SetDuty(uint16_t w)
{
  if (w == 0)
  {
    sl_pwm_stop(&sl_pwm_w_led0);
  }
  else
  {
    my_pwm_set_duty_cycle_10bit_resolution(&sl_pwm_w_led0, w);
    sl_pwm_start(&sl_pwm_w_led0);
  }
}

void Indic_SetDuty(uint16_t w_led)
{
  if (w_led == 0)
  {
    sl_pwm_stop(&sl_pwm_Indic_led0);
  }
  else
  {
    my_pwm_set_duty_cycle_10bit_resolution(&sl_pwm_Indic_led0, w_led);
    sl_pwm_start(&sl_pwm_Indic_led0);
  }
}

/** @brief 设置主灯 WRGB 四通道 PWM（W=PWM，RGB=SM15135E SPI） */
void LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b)
{
  uint32_t sm_r = static_cast<uint32_t>(r) << 6;
  uint32_t sm_g = static_cast<uint32_t>(g) << 6;
  uint32_t sm_b = static_cast<uint32_t>(b) << 6;

  LED_W_SetDuty(w);

  if (r >= 0x3FF)
  {
    sm_r = 0xFFFF;
  }
  if (g >= 0x3FF)
  {
    sm_g = 0xFFFF;
  }
  if (b >= 0x3FF)
  {
    sm_b = 0xFFFF;
  }

  SpiSm15135eHal& driver = rgbDriverInstance();
  driver.SetAllGain(CurrentGain::Gain_101_1MA);
  driver.SetRgbwy(static_cast<uint16_t>(sm_b), static_cast<uint16_t>(sm_g),
                  static_cast<uint16_t>(sm_r), 0, 0);
  driver.SendFrame();
}
