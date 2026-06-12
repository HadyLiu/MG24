#include "test.h"


#if TEST_ADC

#include "../driver/hal/IadcHal.h"

//实例化ADC对象
IadcDriver adcA8(gpioPortA, 8); // PA08

void test_adc_Init(void)
{
    adcA8.Init(); // 初始化 IADC 驱动
}

void test_adc(void)
{
    static uint32_t test_count = 0;
    test_count++;
    if (test_count >= 100) { // 每100次调用测试一次ADC
        test_count = 0;
        uint32_t volA8 = adcA8.ReadVoltageMilliVolts();
        SILABS_LOG("PA08 Voltage: %lu mV", volA8);
    }
}

#endif
