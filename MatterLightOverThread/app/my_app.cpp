#include "my_app.h"

#include "sl_udelay.h"
#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

uint32_t g_time_clk = 0;
uint32_t g_time_detect_bat = 0;
bool     g_is_operation_in_progress = false; // 耗时过长的操作正在进行中

void write_led_example(void);
void CheckTickResolution(void);
void ResetMatterNetworkConfiguration(void);

void check_interrupt_injection_status(uint8_t pin);
/**
 * @brief 📬 初始化阶段：在这里设置好所有的硬件和软件资源，准备好迎接后续的业务逻辑
 */
void my_custom_init_app_process(void)
{
    // Your initialization code here
    SILABS_LOG("[app]run start");

    gpio_init(); // 初始化 GPIO 引脚
    SILABS_LOG("[app]gpio Init");
    sm15135e_init();
    SILABS_LOG("[app]sm15135e Init");
    MyCustomButtonInit(); // 初始化自定义按键服务 用于创建定时器和设置中断回调
    SILABS_LOG("[app]MyCustomButtonInit");

    powerManage_adc_Init();
    SILABS_LOG("[app]powerManage_adc_Init");
    PowerManageInit(); // 初始化电源管理
    SILABS_LOG("[app]Power completed");

    LED_Init(); // 初始化 LED 状态
    // LED_SetBlink(50, 6, 800, 3); // 上电后闪烁提示，亮度 50%，颜色索引 3，周期 800ms，闪烁 3 次
    //  LED_SetBreath(50, 6, 3200, 0); // 上电后呼吸提示，亮度 50%，颜色索引 6，周期 3200ms，持续呼吸
    // ResetMatterNetworkConfiguration(); // 初始化配网状态，设置好默认的灯效序列
}

/**
 * @brief 📬 主循环阶段：在这里处理所有的业务逻辑，包括电源管理、按键处理等
 */
void my_custom_loop_app_process(void)
{
    static uint32_t tick = 0;
    // 获取计时器的值
    g_time_clk++;

    // 获取电源状态
    GetExternPowerFlag();

    // 电源管理 用于电源状态的切换
    PowerSwitchAssignment();
    // 补上按键 预防中断失效
    // MyCustomButtonInterruptHandler(0, 0); // 模拟按键事件，实际使用中应由硬件中断触发调用
    LED_Tick10ms(); // LED 驱动的定时器滴答函数，处理渐变效果和状态更新

    if (eg_PowerStatus == false)
    {
        // 电池模式
        if (g_time_clk - g_time_detect_bat > 100) // 1s 检测一次电池状态
        {
            g_time_detect_bat = g_time_clk;
            GetDisChargeStatus();
            SILABS_LOG("Battery Status=%d", eg_BatStatus);
            extern unsigned int g_ADBatLowVal;
            SILABS_LOG("BatVol=%d", g_ADBatLowVal);
            g_is_operation_in_progress = false;
        }
        BatOutEn();
    }
    else
    {
        // 外部供电模式
        ChargeTimeUpdata(); // 充电时间更新
        BatOutDis();
        ChargeLogic(ChargeDetect());
        ChargeCurrentCtrlOut(false);
    }
    if (g_time_clk - tick > 200)
    {
        tick = g_time_clk;
        SILABS_LOG("tick=%d", tick);
        // check_interrupt_injection_status(5);
        //  CheckTickResolution(); // 检查系统 Tick 的时间分辨率，确保定时器逻辑的正确性
        // my_pwm_set_duty_cycle_v1000(&sl_pwm_w_led0, 500); // 设置占空比为50%
        // my_pwm_set_duty_cycle_v1000(&sl_pwm_Indic_led0, 250); // 设置占空比为25%
        // sl_pwm_start(&sl_pwm_w_led0);
        // sl_pwm_start(&sl_pwm_Indic_led0);

        // write_led_example();
    }
}

/**
 * @brief 📬 重置matter配网
 * @param  无
 */
void ResetMatterNetworkLightingEffects(void)
{
#define MixedLightingEffectsCount 4
    static const MixedLightingEffects_t MixedLightingEffects_Start_Example[MixedLightingEffectsCount] = {
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 100, .color_index = 6, .fade_ms = 800, .count = 3},
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 100, .color_index = 6, .fade_ms = 2400, .count = 1},
        {.fuction_mode = LED_EFFECT_HOLD, .is_on = false, .brightness = 100, .color_index = 2, .fade_ms = 2000, .count = 1},
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 100, .color_index = 2, .fade_ms = 400, .count = 2},
    }; // 最后一个 NONE 代表特效序列结束

    memcpy(g_mixed_effects, MixedLightingEffects_Start_Example, sizeof(MixedLightingEffects_Start_Example));
    // 启动混合特效
    Led_MixedLightingEffects_Start(MixedLightingEffectsCount, true, 100, 2);

    // Your reset logic here
    SILABS_LOG("开启配网等效的灯效提示...");
    // 这里可以添加任何需要在重置时执行的清理代码，例如重置全局变量、清除持久化存储等
}

void ResetMatterNetworkConfiguration(void)
{
    // Your reset logic here
    SILABS_LOG("[app]重置Matter网络配置...");
    // 这里可以添加任何需要在重置时执行的清理代码，例如重置全局变量、清除持久化存储等
    chip::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
}

/**
 * @brief 📬 放弃配网：当用户选择放弃配网时调用，执行相关的清理和状态更新
 * @param  无
 */
void GiveUpMatterNetworkConfiguration(void)
{
    // Your logic to give up network configuration here
    SILABS_LOG("[app]Giving up on Matter network configuration...");
    // 这里可以添加任何需要在放弃网络配置时执行的代码，例如重置网络状态
    LED_StopMixedEffects(true, 100, 2);
}

/**
 * @brief 📬 配网成功动画：当设备成功配对到网络后调用，执行一段特定的灯效动画来提示用户
 */
void TriggerPairingSuccessAnimation(void)
{
#define SuccessEffectsCount 1
    static const MixedLightingEffects_t Success_Effects[SuccessEffectsCount] = {
        // Step 0: 以 60% 亮度快速闪烁 2 次（亮 200ms，灭 200ms，周期 400ms，次数 2）
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 60, .color_index = 0, .fade_ms = 400, .count = 2},
    };

    memcpy(g_mixed_effects, Success_Effects, sizeof(Success_Effects));

    // 启动混合灯效
    Led_MixedLightingEffects_Start(SuccessEffectsCount, true, 100, 2);
}

/**
 * @brief 📬 异步消费端：成功寄居在其他业务文件中
 */
void MyButtonActionHandler(AppEvent *aEvent)
{
    SILABS_LOG("【MyButtonActionHandler】Enter button event handler");
    static uint16_t save_history_long_press_count = 0; // 用于记录上一次长按事件的计数，帮助区分长按和长按松开
    // 💡 保持强转架构解包法：你怎么强转过去，我就怎么强转回来读，内存完美重合！
    AppButtonEvent *pBtnEvent = reinterpret_cast<AppButtonEvent *>(aEvent);

    uint8_t  button_idx = pBtnEvent->ButtonEvent.ButtonIdx;
    uint8_t  action = pBtnEvent->ButtonEvent.Action;
    uint16_t long_press_count = pBtnEvent->ButtonEvent.LongPressCount;

    switch (action)
    {
    case AppButtonEvent::kButtonAction_ShortPress:
        SILABS_LOG(" -> [业务确诊] 按键 %d : 单击触发！", button_idx);
        // 这里写你的单击控制代码

        if (g_led.is_on == 0 || g_led.brightness == 0)
        {
            g_led.is_on = 1;
            g_led.brightness = LED_BRIGHTNESS_MAX;
        }
        else if (g_led.is_on == 1 && g_led.brightness == LED_BRIGHTNESS_MAX)
        {
            g_led.brightness = (LED_BRIGHTNESS_MAX >> 1); // 50% 亮度
        }
        else if (g_led.is_on == 1 && g_led.brightness == (LED_BRIGHTNESS_MAX >> 1))
        {
            g_led.is_on = 0;
            g_led.brightness = 0;
        }
        else
        {
            g_led.is_on = 0;
            g_led.brightness = 0;
        }
        SILABS_LOG("is_On=%d, brightness=%d", g_led.is_on, g_led.brightness);
        LED_Start_Fade_Color_Index(g_led.is_on, g_led.brightness, g_led.color_index, 400);
        break;

    case AppButtonEvent::kButtonAction_DoublePress:
        SILABS_LOG(" -> [业务确诊] 按键 %d : 双击触发！", button_idx);
        if (g_led.is_on == 0)
        {
            SILABS_LOG("当前LED熄灭！！！双击事件不执行任何操作");
            break;
        }
        // 这里写你的双击控制代码
        g_led.color_index++;
        if (g_led.color_index >= LED_COLOR_COUNT)
        {
            g_led.color_index = 0;
        }
        SILABS_LOG("color_index=%d", g_led.color_index);
        LED_Start_Fade_Color_Index(g_led.is_on, g_led.brightness, g_led.color_index, 400);
        break;

    case AppButtonEvent::kButtonAction_LongPressStart:
    {
        SILABS_LOG(" -> [业务确诊] 按键 %d : 开始长按！", button_idx);
        break;
    }
    case AppButtonEvent::kButtonAction_LongPressing:
        if (long_press_count == 25) // 5000/200=25 代表每200ms触发一次长按事件
        {
            // 开启配网灯效
            ResetMatterNetworkLightingEffects(); // 长按过程中触发重置配网
        }
        if (long_press_count == 50) // 10000/200=50 代表每200ms触发一次长按事件
        {
            // 清除配网信息 开启配网模式
            ResetMatterNetworkConfiguration(); // 长按过程中触发重置配网
        }
        save_history_long_press_count = long_press_count; // 更新历史计数
        SILABS_LOG(" -> [业务确诊] 按键 %d : 长按中！ 脉冲计数 = %d", button_idx, long_press_count);
        break;
    case AppButtonEvent::kButtonAction_LongPressRelease:
        // 长按时间不足过程不重置配网，但未达到放弃配网的条件
        if (save_history_long_press_count >= 25 && save_history_long_press_count < 50)
        {
            GiveUpMatterNetworkConfiguration(); // 长按过程中触发放弃配网
        }
        SILABS_LOG(" -> [业务确诊] 按键 %d : 长按松开！ 脉冲计数 = %d", button_idx, save_history_long_press_count);
        break;

    default: SILABS_LOG(" -> [未捕获] 动作编号: %d", action); break;
    }
}

void CheckTickResolution(void)
{
    // 获取系统内核定时器的频率（每秒有多少个 Tick）
    uint32_t tick_per_second = osKernelGetSysTimerFreq();

    // 计算 1 个 Tick 是多少毫秒
    uint32_t ms_per_tick = 1000 / tick_per_second;

    SILABS_LOG("系统内核每秒 Tick 数: %d Hz", tick_per_second);
    SILABS_LOG("因此 1 个 Tick = %d 毫秒", ms_per_tick);
}

// 测试硬件是否开启了双沿中断
void check_interrupt_injection_status(uint8_t pin)
{
    // EFR32 的硬件外部中断通道号直接等于引脚号 (0 ~ 15)
    uint32_t int_no = pin;
    uint32_t mask = (1u << int_no);

    // 1. 读取硬件寄存器：上升沿使能寄存器 和 下降沿使能寄存器
    // 在旧版 SDK 中对应 GPIO->EXTIRISE / GPIO->EXTIFALL
    // 在新版 SDK 中对应 GPIO->IEN (中断使能) 或直接读取芯片外设基地址
    bool rise_enabled = (GPIO->EXTIRISE & mask) != 0;
    bool fall_enabled = (GPIO->EXTIFALL & mask) != 0;

    SILABS_LOG("\r\n=== 🔍 硬件中断通道 %d 状态体检 ===\r\n", int_no);
    SILABS_LOG("上升沿触发 (Rising Edge): %s\r\n", rise_enabled ? "【已开启 🟢】" : "【未开启 ❌】");
    SILABS_LOG("下降沿触发 (Falling Edge): %s\r\n", fall_enabled ? "【已开启 🟢】" : "【未开启 ❌】");

    // 2. 核心判定逻辑
    if (rise_enabled && fall_enabled)
    {
        SILABS_LOG("结果：🎉 恭喜！双沿中断已【百分之百成功注入】硬件底层！\r\n\r\n");
    }
    else
    {
        SILABS_LOG("结果：⚠️ 警告！硬件仍处于单沿状态（原厂Bug未被覆盖）。\r\n\r\n");
    }
}
