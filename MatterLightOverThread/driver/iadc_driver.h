#ifndef IADC_DRIVER_H_
#define IADC_DRIVER_H_

#include "em_iadc.h"
#include "em_gpio.h"
#include "em_cmu.h"

class IadcDriver {
public:
    /**
     * @brief 构造函数
     * @param port GPIO端口 (例如: gpioPortA)
     * @param pin  GPIO引脚号 (例如: 8)
     */
    IadcDriver(GPIO_Port_TypeDef port, unsigned int pin);

    ~IadcDriver();

    /**
     * @brief 初始化 IADC 模块以及 ABUS 模拟总线分配
     */
    void Init(void);

    /**
     * @brief 反初始化 IADC 模块并将引脚恢复为普通 GPIO 模式
     * @param newMode 新的 GPIO 模式 (例如: gpioModePushPull)
     * @param outValue 输出值 (如果是输出模式)
     */
    void DeInit(GPIO_Mode_TypeDef newMode, unsigned int outValue);

    /**
     * @brief 读取当前配置引脚的原始 ADC 数值 (0 - 4095)
     */
    int32_t ReadRawResult(void);

    /**
     * @brief 读取当前配置引脚的电压值 (单位: mV)
     */
    uint32_t ReadVoltageMilliVolts(void);

    /**
     * @brief 专门用于读取 EFR32MG24 芯片自身的 AVDD 供电电压
     */
    uint32_t ReadChipAvddMilliVolts(void);

private:
    GPIO_Port_TypeDef m_port;
    unsigned int      m_pin;
    const uint32_t    m_vRefMv = 3300; // 默认参考电压 3300mV (AVDD)
    
    void CommonConfig(IADC_Init_t &init, IADC_AllConfigs_t &allConfigs);
    void AllocateAnalogBus(void);
    void FreeAnalogBus(void);
};

#endif // IADC_DRIVER_H_