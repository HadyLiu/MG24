#include "HalExti.h"
#include "sl_device_gpio.h"

bool HalExti::s_isInitialized = false;

/**
 * @brief 构造函数，配置外部中断的 GPIO 引脚和触发方式
 * @param port GPIO 端口号
 * @param pin GPIO 引脚号
 * @param trigger 触发方式（上升沿、下降沿或双沿）
 */
HalExti::HalExti(sl_gpio_port_t port, uint8_t pin, EdgeTrigger trigger)
    : gpio({port, pin}), m_trigger(trigger), m_assignedIntNo(-1), m_callback(nullptr), m_context(nullptr)
{
}

/**
 * @brief 析构函数，反初始化外部中断
 */
HalExti::~HalExti()
{
    Deinit();
}

/**
 * @brief 初始化外部中断
 * @return true 初始化成功，false 初始化失败（如引脚号非法）
 */
bool HalExti::Init()
{
    if (gpio.pin >= 16)
    {
        return false;
    }

    if (!s_isInitialized)
    {
        GPIOINT_Init();
        s_isInitialized = true;
    }

    sl_gpio_set_pin_mode(&gpio, SL_GPIO_MODE_INPUT_PULL, false);

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

    // 绑定 GPIOINT 分发到 NativeIsrHandler
    GPIOINT_CallbackRegisterExt(gpio.pin, (GPIOINT_IrqCallbackPtrExt_t)HalExti::NativeIsrHandler, this);

    return true;
}

/**
 * @brief 反初始化外部中断
 * @return 无
 */
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

/**
 * @brief 注册外部中断回调函数
 * @param callback 用户定义的回调函数指针，函数签名为 void (*
 * callback)(uint8_t pin, bool pin_state, void* context)，其中 pin_state
 * 表示当前引脚的电平状态（true=高电平，false=低电平）
 * @param context 用户自定义上下文指针，将在中断发生时传回给回调函数
 * @return 无
 */
void HalExti::RegisterCallback(ExtiInstanceCallback_t callback, void* context)
{
    m_callback = callback;
    m_context  = context;
}

/**
 * @brief 启用/禁用外部中断
 * @param enable_flag true=启用中断，false=禁用中断
 * @return 无
 */
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

/**
 * @brief 统一硬件中断分发器（ISR 级别）
 * @param pin 触发中断的引脚号
 * @param ctx 官方注册回调时登记的上下文指针（即当前 HalExti 实例对象指针）
 * @return 无
 * @note 1. 该函数由官方 GPIOINT 模块在中断发生时调用，处于 ISR
 * 上下文，禁止日志/阻塞；2. 通过 ctx
 * 参数获取当前实例对象，读取引脚状态，并调用用户注册的回调函数将
 */
void HalExti::NativeIsrHandler(uint8_t pin, void* ctx)
{
    if (ctx != nullptr)
    {
        // 1. 强转回当前类的实例对象
        HalExti* current_object = static_cast<HalExti*>(ctx);

        // 🌟 2. 核心改动：利用官方标准 API 读取该引脚当前真实的输入电平状态
        bool pin_state = false;
        sl_gpio_get_pin_input(&(current_object->gpio), &pin_state);

        // 3. 将 pin 号、读取到的 io 状态、用户自定义上下文一起打包回传给上层
        if (current_object->m_callback != nullptr)
        {
            current_object->m_callback(pin, pin_state, current_object->m_context);
        }
    }
}
