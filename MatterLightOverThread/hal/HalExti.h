#pragma once

#include "HalGpio.h"
#include "gpiointerrupt.h"
#include "sl_gpio.h"
#include <stdint.h>

class HalExti
{
public:
  enum class EdgeTrigger : uint8_t
  {
    RISING = 0,
    FALLING,
    BOTH
  };

  // 回调函数指针新增 bool pin_state 参数 (true=高电平, false=低电平)
  typedef void (*ExtiInstanceCallback_t)(uint8_t pin, bool pin_state,
                                         void* context);

  HalExti(sl_gpio_port_t port, uint8_t pin, EdgeTrigger trigger);
  ~HalExti();

  bool Init();
  void Deinit();

  void RegisterCallback(ExtiInstanceCallback_t callback, void* context);
  void Enable(bool enable_flag);
  HalGpio::GpioPinStateEnum GetGpioPinState();

private:
  sl_gpio_t gpio;
  EdgeTrigger m_trigger;
  int32_t m_assignedIntNo;

  ExtiInstanceCallback_t m_callback;
  void* m_context;

  static bool s_isInitialized;
  static void NativeIsrHandler(uint8_t pin, void* ctx);
};
