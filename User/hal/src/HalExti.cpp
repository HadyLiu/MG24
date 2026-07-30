#include "HalExti.h"
#include "sl_device_gpio.h"

bool HalExti::s_isInitialized = false;

/**
 * @brief 构造函数，配置外部中断的 GPIO 引脚和触发方式
 */
HalExti::HalExti(sl_gpio_port_t port, uint8_t pin, EdgeTrigger trigger)
    : gpio({port, pin}), m_trigger(trigger), m_pull(PullMode::Down), m_assignedIntNo(-1), m_callback(nullptr),
      m_context(nullptr)
{
}

HalExti::~HalExti()
{
    Deinit();
}

/**
 * @brief 初始化外部中断
 */
bool HalExti::Init(PullMode pull)
{
    if (gpio.pin >= 16)
    {
        return false;
    }

    m_pull = pull;

    if (!s_isInitialized)
    {
        GPIOINT_Init();
        s_isInitialized = true;
    }

    if (m_pull == PullMode::None)
    {
        sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT, false);
    }
    else if (m_pull == PullMode::Up)
    {
        sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL, true);
    }
    else
    {
        sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL, false);
    }

    sl_gpio_interrupt_flag_t flags = SL_GPIO_INTERRUPT_RISING_FALLING_EDGE;
    if (m_trigger == EdgeTrigger::RISING)
    {
        flags = SL_GPIO_INTERRUPT_RISING_EDGE;
    }
    else if (m_trigger == EdgeTrigger::FALLING)
    {
        flags = SL_GPIO_INTERRUPT_FALLING_EDGE;
    }

    m_assignedIntNo    = SL_GPIO_INTERRUPT_UNAVAILABLE;
    sl_status_t status = sl_gpio_configure_external_interrupt(&gpio, &m_assignedIntNo, flags, nullptr, nullptr);
    if (status != SL_STATUS_OK)
    {
        return false;
    }

    GPIOINT_CallbackRegisterExt(gpio.pin, (GPIOINT_IrqCallbackPtrExt_t)HalExti::NativeIsrHandler, this);

    return true;
}

void HalExti::Deinit()
{
    if (gpio.pin < 16)
    {
        Enable(false);
        sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_DISABLED, false);

        if (m_assignedIntNo != -1)
        {
            sl_gpio_deconfigure_external_interrupt(m_assignedIntNo);
            m_assignedIntNo = -1;
        }
        GPIOINT_CallbackRegisterExt(gpio.pin, nullptr, nullptr);
    }
}

void HalExti::RegisterCallback(ExtiInstanceCallback_t callback, void* context)
{
    m_callback = callback;
    m_context  = context;
}

void HalExti::Enable(bool enable_flag)
{
    if (enable_flag)
    {
        sl_gpio_enable_interrupts(1 << gpio.pin);
    }
    else
    {
        sl_gpio_disable_interrupts(1 << gpio.pin);
    }
}

HalGpio::GpioPinStateEnum HalExti::GetGpioPinState()
{
    bool pin_state;
    sl_gpio_get_pin_input(&gpio, &pin_state);

    return pin_state ? HalGpio::GpioPinStateEnum::GPIO_PIN_SET : HalGpio::GpioPinStateEnum::GPIO_PIN_RESET;
}

void HalExti::NativeIsrHandler(uint8_t pin, void* ctx)
{
    if (ctx != nullptr)
    {
        HalExti* current_object = static_cast<HalExti*>(ctx);

        bool pin_state = false;
        sl_gpio_get_pin_input(&(current_object->gpio), &pin_state);

        if (current_object->m_callback != nullptr)
        {
            current_object->m_callback(pin, pin_state, current_object->m_context);
        }
    }
}
