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

/** @brief IADC 采样 HAL，绑定单路 GPIO 模拟输入 */
class IadcHal
{
public:
  /**
   * @brief 构造函数
   * @param port GPIO 端口
   * @param pin  GPIO 引脚号
   */
  IadcHal(GPIO_Port_TypeDef port, unsigned int pin);

  ~IadcHal();

  /** @brief 初始化 IADC 及 ABUS 模拟总线分配
   *  @return 无 */
  void Init();

  /**
   * @brief 反初始化并将引脚恢复为普通 GPIO
   * @param newMode  新的 GPIO 模式
   * @param outValue 输出初值（输出模式时有效）
   * @return 无
   */
  void DeInit(GPIO_Mode_TypeDef newMode, unsigned int outValue);

  /** @brief 读取原始 ADC 值（0~4095）
   *  @return 原始值，失败返回 -1 */
  int32_t ReadRawResult();

  /** @brief 读取电压（mV）
   *  @return 电压毫伏值 */
  uint32_t ReadVoltageMilliVolts();

  /** @brief 读取芯片 AVDD 供电电压（mV）
   *  @return AVDD 毫伏值 */
  uint32_t ReadChipAvddMilliVolts();

private:
  GPIO_Port_TypeDef m_port;
  unsigned int m_pin;
  const uint32_t m_vRefMv = 3300;

  void CommonConfig(IADC_Init_t& init, IADC_AllConfigs_t& allConfigs);
  void AllocateAnalogBus();
  void FreeAnalogBus();
};

/** @brief 兼容旧测试代码的类型别名 */
using IadcDriver = IadcHal;
