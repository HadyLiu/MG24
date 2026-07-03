#include "BspLedIndicatorRed.h"

/**
 * @brief 构造函数：完美对齐头文件声明顺序进行初始化
 **/
BspLedIndicatorRed::BspLedIndicatorRed()
    : indicatorRed_(INDIC_R_LED_PORT, INDIC_R_LED_PIN)
{
  // 构造函数体内部可以留空，或者做其他事情
}

/**
 * @brief 初始化红色指示灯
 **/
void BspLedIndicatorRed::Init()
{
  // 初始化红色指示灯 GPIO，假设 GPIO 0
  indicatorRed_.Init(SL_GPIO_MODE_PUSH_PULL,
                     HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
}

/**
 * @brief 设置红色指示灯开关状态
 * @param on true 打开指示灯，false 关闭指示灯
 **/
void BspLedIndicatorRed::SetRedIndicator(bool on)
{
  if (on)
  {
    indicatorRed_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_SET);
  }
  else
  {
    indicatorRed_.SetGpioPinState(HalGpio::GpioPinStateEnum::GPIO_PIN_RESET);
  }
}
