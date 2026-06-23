#include "BspLedIndicatorWhite.h"

BspLedIndicatorWhite::BspLedIndicatorWhite() : IndicWhitePwm_()
{
}

/**
 * @brief 初始化白色指示灯 PWM 驱动
 * @return 无
 */
void BspLedIndicatorWhite::Init()
{
  IndicWhitePwm_.Init(&sl_pwm_w_led0);
}

/**
 * @brief 设置白色指示灯 PWM 占空比
 * @param duty 占空值（0~1023）
 * @return 无
 * @note 自动限幅；Series 2 使用 compare/top 比例换算
 */
void BspLedIndicatorWhite::IndicatorWhiteSetDuty(uint16_t duty)
{
  IndicWhitePwm_.PwmSetDuty(duty);
}
