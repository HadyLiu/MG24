/**
 * @file IadcHal.h
 * @brief IADC 模拟采样 HAL
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 由 iadc_driver 迁入；支持多通道复用同一 IADC 外设。
 */
#pragma once

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_iadc.h"

#include "HalDef.h"
#include "HalGpio.h"

/** @brief IADC 采样 HAL，绑定单路 GPIO 模拟输入 */
class HalIadc {
 public:
  /** @brief 构造函数   */
  HalIadc(uint8_t port, uint8_t pin);

  /** @brief 析构函数   */
  ~HalIadc();

  /** @brief 初始化 IADC 及 ABUS 模拟总线分配   */
  void Init();

  /** @brief 反初始化并将引脚恢复为普通 GPIO */
  void DeInit(GPIO_Mode_TypeDef newMode, HalGpio::GpioPinStateEnum pinState);

  /** @brief 读取原始 ADC 值（0~4095）*/
  int32_t ReadRawResult();

  /** @brief 读取电压（mV）*/
  uint32_t ReadVoltageMilliVolts();

  /** @brief 读取芯片 AVDD 供电电压（mV）*/
  uint32_t ReadChipAvddMilliVolts();

 private:
  uint8_t m_port;
  uint8_t m_pin;
  const uint32_t m_vRefMv = 3300;

  /** @brief 填充 IADC 公共时钟与参考电压配置 */
  void CommonConfig(IADC_Init_t& init, IADC_AllConfigs_t& allConfigs);

  /** @brief 分配 Series 2 ABUS 模拟总线至 ADC0 */
  void AllocateAnalogBus();

  /** @brief 释放 ABUS 模拟总线，恢复 TRISTATE*/
  void FreeAnalogBus();
};
