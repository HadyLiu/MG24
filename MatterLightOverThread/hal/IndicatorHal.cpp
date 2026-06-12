/**
 * @file IndicatorHal.cpp
 * @brief 红/白指示灯 HAL 实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 */
#include "IndicatorHal.h"
#include "PinHal.h"
#include "led_pwm.h"
#include <stdint.h>

namespace
{

struct BlinkNormalCfg
{
  uint16_t duration_ms;
  uint16_t blink_count;
};

struct BlinkMixedCfg
{
  uint16_t m1_duration_ms;
  uint16_t m1_count;
  uint16_t m2_duration_ms;
  uint16_t m2_count;
};

typedef enum
{
  MODE_NORMAL      = 0,
  MODE_MIXED_BLINK = 1
} blink_mode_t;

typedef enum
{
  STAGE_MODE1_SPEED = 0,
  STAGE_MODE2_SPEED = 1
} blink_stage_t;

volatile bool s_blink_normal_flag = false;
volatile bool s_blink_mixed_flag  = false;

bool s_blink_active           = false;
bool s_led_is_on              = false;
uint16_t s_timer_down_counter = 0;
uint16_t s_cur_reload_ticks   = 0;
uint16_t s_rem_blink_count    = 0;
blink_mode_t s_blink_mode     = MODE_NORMAL;
blink_stage_t s_blink_stage   = STAGE_MODE1_SPEED;
uint16_t s_m1_ticks           = 0;
uint16_t s_m2_ticks           = 0;
uint16_t s_rem_m1_count       = 0;
uint16_t s_rem_m2_count       = 0;
BlinkNormalCfg s_normal_cfg   = {400, 0};
BlinkMixedCfg s_mixed_cfg     = {800, 3, 2400, 1};

const uint16_t ease_quad_table_4096[81] = {
    0,    1,    5,    11,   20,   32,   46,   62,   81,   103,  128,  154,
    184,  216,  251,  288,  327,  370,  414,  462,  512,  564,  620,  677,
    737,  800,  865,  933,  1004, 1077, 1152, 1230, 1311, 1394, 1479, 1567,
    1658, 1750, 1845, 1943, 2048, 2153, 2251, 2346, 2438, 2529, 2617, 2702,
    2785, 2866, 2944, 3019, 3092, 3163, 3231, 3296, 3359, 3419, 3476, 3532,
    3584, 3634, 3682, 3726, 3769, 3808, 3845, 3880, 3912, 3942, 3968, 3993,
    4015, 4034, 4050, 4064, 4076, 4085, 4091, 4095, 4096};

bool s_breath_active   = false;
uint16_t s_breath_tick = 0;
uint8_t s_brightness   = 100;

/** @brief 停止红灯闪烁（内部） */
void stopRedBlinkInternal()
{
  s_blink_active = false;
  PinHal::setRedIndicator(false);
}

/** @brief 启动普通红灯闪烁（内部） */
void startRedBlinkInternal(const BlinkNormalCfg* cfg)
{
  if (cfg == nullptr)
  {
    return;
  }

  s_cur_reload_ticks = (cfg->duration_ms / 10) >> 1;
  if (s_cur_reload_ticks == 0)
  {
    s_cur_reload_ticks = 1;
  }
  s_timer_down_counter = s_cur_reload_ticks;
  s_rem_blink_count    = cfg->blink_count;
  s_blink_mode         = MODE_NORMAL;
  s_led_is_on          = true;
  PinHal::setRedIndicator(true);
  s_blink_active = true;
}

/** @brief 启动混合红灯闪烁（内部） */
void startMixedBlinkInternal(const BlinkMixedCfg* cfg)
{
  if (cfg == nullptr)
  {
    return;
  }

  s_m1_ticks = (cfg->m1_duration_ms / 10) >> 1;
  s_m2_ticks = (cfg->m2_duration_ms / 10) >> 1;
  if (s_m1_ticks == 0)
  {
    s_m1_ticks = 1;
  }
  if (s_m2_ticks == 0)
  {
    s_m2_ticks = 1;
  }

  s_blink_mode         = MODE_MIXED_BLINK;
  s_blink_stage        = STAGE_MODE1_SPEED;
  s_rem_m1_count       = cfg->m1_count;
  s_rem_m2_count       = cfg->m2_count;
  s_cur_reload_ticks   = s_m1_ticks;
  s_timer_down_counter = s_m1_ticks;
  s_led_is_on          = true;
  PinHal::setRedIndicator(true);
  s_blink_active = true;
}

/** @brief 红灯闪烁调度（内部） */
void dispatchRedBlinkInternal()
{
  if (s_blink_mixed_flag)
  {
    if (!s_blink_active || (s_blink_mode != MODE_MIXED_BLINK))
    {
      startMixedBlinkInternal(&s_mixed_cfg);
    }
    return;
  }

  if (s_blink_normal_flag)
  {
    if (!s_blink_active)
    {
      startRedBlinkInternal(&s_normal_cfg);
    }
  }
}

/** @brief 红灯 10ms tick（内部） */
void pollRedBlinkInternal()
{
  if (s_blink_active)
  {
    if (s_blink_mode == MODE_NORMAL && !s_blink_normal_flag)
    {
      stopRedBlinkInternal();
      return;
    }
    if (s_blink_mode == MODE_MIXED_BLINK && !s_blink_mixed_flag)
    {
      stopRedBlinkInternal();
      return;
    }
  }
  else
  {
    return;
  }

  if (--s_timer_down_counter == 0)
  {
    s_timer_down_counter = s_cur_reload_ticks;

    if (s_led_is_on)
    {
      s_led_is_on = false;
      PinHal::setRedIndicator(false);

      if (s_blink_mode == MODE_NORMAL)
      {
        if (s_rem_blink_count > 0)
        {
          if (--s_rem_blink_count == 0)
          {
            stopRedBlinkInternal();
            s_blink_normal_flag = false;
            return;
          }
        }
      }
      else
      {
        if (s_blink_stage == STAGE_MODE1_SPEED)
        {
          if (--s_rem_m1_count == 0)
          {
            s_blink_stage        = STAGE_MODE2_SPEED;
            s_cur_reload_ticks   = s_m2_ticks;
            s_timer_down_counter = s_m2_ticks;
          }
        }
        else
        {
          if (--s_rem_m2_count == 0)
          {
            stopRedBlinkInternal();
            s_blink_mixed_flag = false;
            return;
          }
        }
      }
    }
    else
    {
      s_led_is_on = true;
      PinHal::setRedIndicator(true);
    }
  }
}

/** @brief 获取呼吸波形比例（内部） */
uint16_t getBreathScale(uint16_t tick)
{
  if (tick < 80)
  {
    return ease_quad_table_4096[tick];
  }
  if (tick < 160)
  {
    return 4096;
  }
  if (tick < 240)
  {
    return ease_quad_table_4096[240 - tick];
  }
  return 0;
}

/** @brief App 百分比映射到 14-bit 权重（内部） */
uint32_t mapAppPercentToOutputScale14bit(uint16_t app_percent)
{
  if (app_percent <= 1)
  {
    return 204;
  }
  if (app_percent >= 100)
  {
    return 10240;
  }
  return 204 + ((uint32_t)(app_percent - 1) * 10035) / 99;
}

} // namespace

/** @brief 启动单速红灯闪烁 */
void IndicatorHal::startRedBlinkNormal(uint16_t periodMs, uint16_t count)
{
  BlinkNormalCfg cfg = {periodMs, count};
  s_blink_normal_flag = true;
  s_normal_cfg        = cfg;
}

/** @brief 启动双速混合红灯闪烁 */
void IndicatorHal::startRedBlinkMixed(uint16_t m1Period, uint16_t m1Count,
                                     uint16_t m2Period, uint16_t m2Count)
{
  BlinkMixedCfg cfg = {m1Period, m1Count, m2Period, m2Count};
  s_blink_mixed_flag = true;
  s_mixed_cfg        = cfg;
}

/** @brief 停止红灯闪烁 */
void IndicatorHal::stopRedBlink()
{
  stopRedBlinkInternal();
  s_blink_normal_flag = false;
  s_blink_mixed_flag  = false;
}

/** @brief 红灯闪烁调度与 10ms tick */
void IndicatorHal::pollRedBlink10ms()
{
  dispatchRedBlinkInternal();
  pollRedBlinkInternal();
}

/** @brief 启动白呼吸 */
void IndicatorHal::startWhiteBreath(uint8_t brightnessPercent)
{
  s_brightness = brightnessPercent > 100 ? 100 : brightnessPercent;
  s_breath_tick   = 0;
  s_breath_active = true;
}

/** @brief 停止白呼吸 */
void IndicatorHal::stopWhiteBreath()
{
  s_breath_active = false;
  Indic_SetDuty(0);
}

/** @brief 白呼吸 10ms tick */
void IndicatorHal::pollWhiteBreath10ms()
{
  if (!s_breath_active)
  {
    return;
  }

  uint32_t breath_scale = getBreathScale(s_breath_tick);

  if (++s_breath_tick >= 320)
  {
    s_breath_tick = 0;
  }

  uint32_t out_scale_14bit = mapAppPercentToOutputScale14bit(s_brightness);
  uint16_t target_pwm =
      (uint16_t)(((out_scale_14bit * breath_scale) + 32768) >> 16);
  Indic_SetDuty(target_pwm);
}
