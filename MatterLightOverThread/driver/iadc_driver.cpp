#include "iadc_driver.h"

static bool s_iadc_global_initialized = false;

IadcDriver::IadcDriver(GPIO_Port_TypeDef port, unsigned int pin)
    : m_port(port), m_pin(pin)
{
}

IadcDriver::~IadcDriver()
{
    CMU_ClockEnable(cmuClock_IADC0, false);
}

// 内部私用：根据官方 Joystick 逻辑打通 Series 2 独有的 ABUS 模拟总线
void IadcDriver::AllocateAnalogBus(void)
{
    if (m_port == gpioPortA) {
        if (0 == m_pin % 2) { // 偶数引脚 (如 PA08)
            if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AEVEN0_MASK) == GPIO_ABUSALLOC_AEVEN0_TRISTATE) {
                GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AEVEN0_ADC0;
            } else if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AEVEN1_MASK) == GPIO_ABUSALLOC_AEVEN1_TRISTATE) {
                GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AEVEN1_ADC0;
            }
        } else { // 奇数引脚
            if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AODD0_MASK) == GPIO_ABUSALLOC_AODD0_TRISTATE) {
                GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AODD0_ADC0;
            } else if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AODD1_MASK) == GPIO_ABUSALLOC_AODD1_TRISTATE) {
                GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AODD1_ADC0;
            }
        }
    } 
    else if (m_port == gpioPortB) {
        if (0 == m_pin % 2) {
            if ((GPIO->BBUSALLOC & _GPIO_BBUSALLOC_BEVEN0_MASK) == GPIO_BBUSALLOC_BEVEN0_TRISTATE) {
                GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BEVEN0_ADC0;
            } else if ((GPIO->BBUSALLOC & _GPIO_BBUSALLOC_BEVEN1_MASK) == GPIO_BBUSALLOC_BEVEN1_TRISTATE) {
                GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BEVEN1_ADC0;
            }
        } else {
            if ((GPIO->BBUSALLOC & _GPIO_BBUSALLOC_BODD0_MASK) == GPIO_BBUSALLOC_BODD0_TRISTATE) {
                GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BODD0_ADC0;
            } else if ((GPIO->BBUSALLOC & _GPIO_BBUSALLOC_BODD1_MASK) == GPIO_BBUSALLOC_BODD1_TRISTATE) {
                GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BODD1_ADC0;
            }
        }
    } 
    else if (m_port == gpioPortC || m_port == gpioPortD) {
        if (0 == m_pin % 2) {
            if ((GPIO->CDBUSALLOC & _GPIO_CDBUSALLOC_CDEVEN0_MASK) == GPIO_CDBUSALLOC_CDEVEN0_TRISTATE) {
                GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDEVEN0_ADC0;
            } else if ((GPIO->CDBUSALLOC & _GPIO_CDBUSALLOC_CDEVEN1_MASK) == GPIO_CDBUSALLOC_CDEVEN1_TRISTATE) {
                GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDEVEN1_ADC0;
            }
        } else {
            if ((GPIO->CDBUSALLOC & _GPIO_CDBUSALLOC_CDODD0_MASK) == GPIO_CDBUSALLOC_CDODD0_TRISTATE) {
                GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDODD0_ADC0;
            } else if ((GPIO->CDBUSALLOC & _GPIO_CDBUSALLOC_CDODD1_MASK) == GPIO_CDBUSALLOC_CDODD1_TRISTATE) {
                GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDODD1_ADC0;
            }
        }
    }
}

void IadcDriver::CommonConfig(IADC_Init_t &init, IADC_AllConfigs_t &allConfigs)
{
    init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, 12000000, 0);

    allConfigs.configs[0].reference = iadcCfgReferenceVddx; 
    allConfigs.configs[0].vRef = m_vRefMv;                 
    
    allConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                  1000000,
                                                                  0,
                                                                  iadcCfgModeNormal,
                                                                  init.srcClkPrescale);
}

void IadcDriver::Init(void)
{
    // 1. 开启时钟
    CMU_ClockEnable(cmuClock_IADC0, true);
    CMU_ClockEnable(cmuClock_GPIO, true);

    // 2. 配置引脚为模拟高阻输入模式
    GPIO_PinModeSet(m_port, m_pin, gpioModeDisabled, 0); 

    // 3. 分配模拟总线开关（解决读数一直为 0 的绝对核心！）
    AllocateAnalogBus();

    // 4. 复位并初始化结构体（仅在全局第一次初始化时进行，避免破坏已有的其他通道初始化）
    if (!s_iadc_global_initialized) {
        IADC_reset(IADC0);

        IADC_Init_t init = IADC_INIT_DEFAULT;
        IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;

        CommonConfig(init, initAllConfigs);

        // 写入全局基础配置
        IADC_init(IADC0, &init, &initAllConfigs);
        s_iadc_global_initialized = true;
    }

    // 5. 将当前通道的转换参数配置进 IADC0 中
    IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
    IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;
    singleInput.posInput = IADC_portPinToPosInput(m_port, m_pin);      
    singleInput.negInput = iadcNegInputGnd; 

    // 6. 写入硬件
    IADC_initSingle(IADC0, &initSingle, &singleInput);
}


// 内部私有：释放之前分配的 ABUS 模拟总线，恢复为 TRISTATE（三态断开）
void IadcDriver::FreeAnalogBus(void)
{
    if (m_port == gpioPortA) {
        if (0 == m_pin % 2) {
            // 可能分配在 AEVEN0 或 AEVEN1，统一清理到 TRISTATE。
            GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AEVEN0_MASK;
            GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AEVEN1_MASK;
        } else {
            GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AODD0_MASK;
            GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AODD1_MASK;
        }
    } 
    else if (m_port == gpioPortB) {
        if (0 == m_pin % 2) {
            GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BEVEN0_MASK;
            GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BEVEN1_MASK;
        } else {
            GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BODD0_MASK;
            GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BODD1_MASK;
        }
    } 
    else if (m_port == gpioPortC || m_port == gpioPortD) {
        if (0 == m_pin % 2) {
            GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDEVEN0_MASK;
            GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDEVEN1_MASK;
        } else {
            GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDODD0_MASK;
            GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDODD1_MASK;
        }
    }
}

// 外部调用：释放引脚与底座的连接并转换为指定普通 IO 模式
void IadcDriver::DeInit(GPIO_Mode_TypeDef newMode, unsigned int outValue)
{
    // 1. 释放内部模拟总线开关，解除引脚与 IADC 模拟总线连接。
    FreeAnalogBus();

    // 2. 重新配置为普通数字 GPIO 模式
    // 例如配置为 gpioModePushPull (普通推挽输出) 或是 gpioModeInputPull (带上拉/下拉输入)
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIO_PinModeSet(m_port, m_pin, newMode, outValue);
}

int32_t IadcDriver::ReadRawResult(void)
{
    // 1. 确保 IADC 外设时钟处于启用状态以防此时已经被关闭
    CMU_ClockEnable(cmuClock_IADC0, true);

    // 2. 动态向 IADC 寄存器重新配置本通道，实现共享单个 IADC 外设的多路复用
    IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
    IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;
    singleInput.posInput = IADC_portPinToPosInput(m_port, m_pin);      
    singleInput.negInput = iadcNegInputGnd; 
    IADC_initSingle(IADC0, &initSingle, &singleInput);

    IADC_command(IADC0, iadcCmdStartSingle);

    uint32_t timeout = 100000;
    while ((IADC_getStatus(IADC0) & IADC_STATUS_SINGLEFIFODV) == 0) {
        timeout--;
        if (timeout == 0) return -1; 
    }

    IADC_Result_t sample = IADC_readSingleResult(IADC0);
    int32_t rawData = sample.data;
    return (rawData < 0) ? 0 : rawData;
}

uint32_t IadcDriver::ReadVoltageMilliVolts(void)
{
    int32_t rawData = ReadRawResult();
    if (rawData < 0) return 0;
    //return ((uint32_t)rawData * m_vRefMv) / 4095;
    return ((uint32_t)rawData * m_vRefMv) >> 12; // 4095 约等于 2^12，使用位移代替除法以提高效率
}

uint32_t IadcDriver::ReadChipAvddMilliVolts(void)
{
    IADC_InitSingle_t avddSingle = IADC_INITSINGLE_DEFAULT;
    IADC_SingleInput_t avddInput = IADC_SINGLEINPUT_DEFAULT;
    
    avddInput.posInput = iadcPosInputAvdd; 
    avddInput.negInput = iadcNegInputGnd;

    IADC_initSingle(IADC0, &avddSingle, &avddInput);
    int32_t rawData = ReadRawResult();

    // 恢复原来的引脚配置
    avddInput.posInput = IADC_portPinToPosInput(m_port, m_pin);
    IADC_initSingle(IADC0, &avddSingle, &avddInput);

    if (rawData < 0) return 0;
    //return (((uint32_t)rawData * m_vRefMv) / 4095) * 4;
    return (((uint32_t)rawData * m_vRefMv) >> 10); // 乘以4等同于右移10位，避免乘法运算溢出
}