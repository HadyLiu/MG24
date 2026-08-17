/**
 * @file IadcHal.cpp
 * @brief IADC 模拟采样 HAL 实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 由 iadc_driver 迁入；支持多通道复用同一 IADC 外设。
 */
#include "HalIadc.h"
#include "sl_udelay.h"

static bool s_iadc_global_initialized = false;

/**
 * @brief 构造函数
 * @param port GPIO 端口
 * @param pin  GPIO 引脚号
 * @return 无
 * @note 仅记录引脚，不访问硬件
 */
HalIadc::HalIadc(uint8_t port, uint8_t pin) : m_port(port), m_pin(pin) {
}

/**
 * @brief 析构函数
 * @return 无
 * @note 关闭 IADC0 时钟
 */
HalIadc::~HalIadc() {
  CMU_ClockEnable(cmuClock_IADC0, false);
}

/**
 * @brief 初始化 IADC 及 ABUS 模拟总线分配
 * @return 无
 * @note 全局仅首次调用时执行 IADC_reset/init
 */
void HalIadc::Init(void) {
  // 1. 开启时钟
  CMU_ClockEnable(cmuClock_IADC0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // 2. 配置引脚为模拟高阻输入模式
  GPIO_PinModeSet(m_port, m_pin, gpioModeDisabled, 0);

  // 3. 分配模拟总线开关（解决读数一直为 0 的绝对核心！）
  AllocateAnalogBus();

  // 4.复位并初始化结构体（仅在全局第一次初始化时进行，避免破坏已有的其他通道初始化）
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

/**
 * @brief 反初始化并将引脚恢复为普通 GPIO
 * @param newMode  新的 GPIO 模式
 * @param outValue 输出初值（输出模式时有效）
 * @return 无
 * @note 释放 ABUS 后重新配置为普通数字 GPIO
 */
void HalIadc::DeInit(GPIO_Mode_TypeDef newMode, HalGpio::GpioPinStateEnum pinState) {
  // 1. 释放内部模拟总线开关，解除引脚与 IADC 模拟总线连接。
  FreeAnalogBus();

  // 2. 重新配置为普通数字 GPIO 模式
  // 例如配置为 gpioModePushPull (普通推挽输出) 或是 gpioModeInputPull
  // (带上拉/下拉输入)
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(m_port, m_pin, newMode, static_cast<uint8_t>(pinState));
}

/**
 * @brief 读取原始 ADC 值（0~4095）
 * @return 原始值，失败返回 -1
 * @note 每次读取前重新配置单通道并等待 FIFO 就绪
 */
int32_t HalIadc::ReadRawResult(void) {
  // 1. 确保 IADC 外设时钟处于启用状态以防此时已经被关闭
  CMU_ClockEnable(cmuClock_IADC0, true);

  // 2. 动态向 IADC 寄存器重新配置本通道，实现共享单个 IADC 外设的多路复用
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;
  singleInput.posInput = IADC_portPinToPosInput(m_port, m_pin);
  singleInput.negInput = iadcNegInputGnd;
  IADC_initSingle(IADC0, &initSingle, &singleInput);

  IADC_command(IADC0, iadcCmdStartSingle);

  sl_udelay_wait(30);  // 等待采样稳定，避免 FIFO 读取为 0

  uint32_t timeout = 100000;
  while ((IADC_getStatus(IADC0) & IADC_STATUS_SINGLEFIFODV) == 0) {
    timeout--;
    if (timeout == 0) return -1;
  }

  IADC_Result_t sample = IADC_readSingleResult(IADC0);
  int32_t rawData = sample.data;
  return (rawData < 0) ? 0 : rawData;
}

/**
 * @brief 读取电压（mV）
 * @return 电压毫伏值
 * @note 使用右移 12 位代替除 4095 以提高效率
 */
uint32_t HalIadc::ReadVoltageMilliVolts(void) {
  int32_t rawData = ReadRawResult();
  if (rawData < 0) return 0;
  // return ((uint32_t)rawData * m_vRefMv) / 4095;
  // 4095 约等于 2^12，使用位移代替除法以提高效率
  return ((uint32_t)rawData * m_vRefMv) >> 12;
}

/**
 * @brief 读取芯片 AVDD 供电电压（mV）
 * @return AVDD 毫伏值
 * @note 临时切换 AVDD 输入，读后恢复本通道；右移 10 位等效乘 4
 */
uint32_t HalIadc::ReadChipAvddMilliVolts(void) {
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
  // return (((uint32_t)rawData * m_vRefMv) / 4095) * 4;
  // 乘以4等同于右移10位，避免乘法运算溢出
  return (((uint32_t)rawData * m_vRefMv) >> 10);
}

/**
 * @brief 分配 Series 2 ABUS 模拟总线至 ADC0
 * @return 无
 * @note 按官方 Joystick 逻辑打通 ABUS；偶数/奇数引脚路由不同
 */
void HalIadc::AllocateAnalogBus(void) {
  if (m_port == gpioPortA) {
    if (0 == m_pin % 2) {  // 偶数引脚 (如 PA08)
      if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AEVEN0_MASK) == GPIO_ABUSALLOC_AEVEN0_TRISTATE) {
        GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AEVEN0_ADC0;
      } else if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AEVEN1_MASK) == GPIO_ABUSALLOC_AEVEN1_TRISTATE) {
        GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AEVEN1_ADC0;
      }
    } else {  // 奇数引脚
      if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AODD0_MASK) == GPIO_ABUSALLOC_AODD0_TRISTATE) {
        GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AODD0_ADC0;
      } else if ((GPIO->ABUSALLOC & _GPIO_ABUSALLOC_AODD1_MASK) == GPIO_ABUSALLOC_AODD1_TRISTATE) {
        GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AODD1_ADC0;
      }
    }
  } else if (m_port == gpioPortB) {
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
  } else if (m_port == gpioPortC || m_port == gpioPortD) {
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

/**
 * @brief 填充 IADC 公共时钟与参考电压配置
 * @param init       输出 IADC 初始化结构
 * @param allConfigs 输出全通道配置结构
 * @return 无
 * @note 供 Init 全局首次初始化使用
 */
void HalIadc::CommonConfig(IADC_Init_t& init, IADC_AllConfigs_t& allConfigs) {
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, 12000000, 0);

  allConfigs.configs[0].reference = iadcCfgReferenceVddx;
  allConfigs.configs[0].vRef = m_vRefMv;

  allConfigs.configs[0].adcClkPrescale =
      IADC_calcAdcClkPrescale(IADC0, 1000000, 0, iadcCfgModeNormal, init.srcClkPrescale);
}

/**
 * @brief 释放 ABUS 模拟总线，恢复 TRISTATE
 * @return 无
 * @note DeInit 时调用，解除引脚与 IADC 连接
 */
void HalIadc::FreeAnalogBus(void) {
  if (m_port == gpioPortA) {
    if (0 == m_pin % 2) {
      // 可能分配在 AEVEN0 或 AEVEN1，统一清理到 TRISTATE。
      GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AEVEN0_MASK;
      GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AEVEN1_MASK;
    } else {
      GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AODD0_MASK;
      GPIO->ABUSALLOC &= ~_GPIO_ABUSALLOC_AODD1_MASK;
    }
  } else if (m_port == gpioPortB) {
    if (0 == m_pin % 2) {
      GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BEVEN0_MASK;
      GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BEVEN1_MASK;
    } else {
      GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BODD0_MASK;
      GPIO->BBUSALLOC &= ~_GPIO_BBUSALLOC_BODD1_MASK;
    }
  } else if (m_port == gpioPortC || m_port == gpioPortD) {
    if (0 == m_pin % 2) {
      GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDEVEN0_MASK;
      GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDEVEN1_MASK;
    } else {
      GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDODD0_MASK;
      GPIO->CDBUSALLOC &= ~_GPIO_CDBUSALLOC_CDODD1_MASK;
    }
  }
}
