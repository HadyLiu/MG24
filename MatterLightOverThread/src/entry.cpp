#include "sl_udelay.h"
#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

#include "AppMatterHandlers.h"

// 引入驱动层的头文件
#include "../driver/button.h"
#include "../driver/led_pwm_port.h"
#include "../driver/led_wrgb.h"
#include "../driver/pinManage.h"
#include "../driver/powerManage.h"
#include "../driver/iadc_driver.h"
#include "../driver/led_red_indic.h"
#include "../driver/led_white_indic.h"
#include "../driver/sm15135e.h"

bool            g_PowerProtect = false; // 电池电量过低标志
static uint16_t g_idleTime10Ms = 0;

void write_led_example(void);
void CheckTickResolution(void);
void ResetMatterNetworkConfiguration(void);
void check_interrupt_injection_status(uint8_t pin);
void PowerSwitchAssignment(void);
void change_led_Indic(uint8_t charge_status);
void CommissioningFirstBreath_Poll(void);

static bool s_first_commission_breath_active = false;

static bool IsCommissioningFirstBreathRequested(void)
{
    if (LED_IsFirstCommissionDone())
    {
        return false;
    }
    if (!IsMatterUnprovisioned())
    {
        return false;
    }
    if (!led_get_status() || g_PowerProtect)
    {
        return false;
    }
    return true;
}

void CommissioningFirstBreath_Stop(void)
{
    if (!s_first_commission_breath_active)
    {
        return;
    }

    s_first_commission_breath_active = false;
    LED_StopEffect();
    SILABS_LOG("[commission] First commissioning white breath stopped");
}

static void CommissioningFirstBreath_Start(void)
{
    led_color_t white = led_get_color_table(0);

    s_first_commission_breath_active = true;
    LED_SetBreath(LED_BRIGHTNESS_MAX, white, 0);
    SILABS_LOG("[commission] First commissioning white breath started");
}

void CommissioningFirstBreath_Poll(void)
{
    if (!IsCommissioningFirstBreathRequested())
    {
        if (s_first_commission_breath_active)
        {
            CommissioningFirstBreath_Stop();
        }
        return;
    }

    if (s_first_commission_breath_active)
    {
        return;
    }

    if (!LED_IsUserEffectIdle())
    {
        return;
    }

    CommissioningFirstBreath_Start();
}

/**
 * @brief 📬 初始化阶段：在这里设置好所有的硬件和软件资源，准备好迎接后续的业务逻辑
 */
void entry_Init(void)
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
    CommissioningFirstBreath_Poll();
}

/**
 * @brief 📬 模块初始化阶段：在这里可以进行一些模块级别的初始化，例如状态变量的设置、定时器的创建等
 */
void module_Init(void)
{
    powerManage_adc_Init(); // 确保 ADC 模块在每次进入正常模式时都被正确初始化，准备好进行电压检测
}

/**
 * @brief 📬 模块反初始化阶段：在这里释放所有的资源，做好系统重置或关闭的准备
 */
void module_Deinit(void)
{
    // 关闭电池输出
    BatOutDis();
    // 🎯 1. 调用统一的 turn_off 接口
    rgb_hardware_enter_low_power();
    powerManage_adc_DeInit();
}

// 🎯 请将这段无依赖的代码放到你的底层轮询或者延时上方：
void dump_power_debug(void)
{
    // 1. 直接读取 ARM Cortex-M33 内核的系统控制寄存器 (SCR)
    // 如果 SLEEPDEEP 位（第 2 位）为 1，说明硬件上内核被允许进深睡 (EM2)
    uint32_t scr = *(volatile uint32_t *)(0xE000ED10);
    bool     sleep_deep_allowed = (scr & (1 << 2)) != 0;

    // 2. 🎯 使用系统的 TaskHandle_t 类型并强转，通过 C++ 严格的类型检查
    TaskHandle_t current_task = (TaskHandle_t)xTaskGetCurrentTaskHandle();
    const char  *task_name = current_task ? pcTaskGetName(current_task) : "Unknown";

    ChipLogProgress(DeviceLayer, "=== [HARDWARE LOG] SCR Reg: 0x%08X (DeepSleep: %s) | Running Task: %s ===", (unsigned int)scr,
                    sleep_deep_allowed ? "YES(EM2)" : "NO(EM1 Locked)", task_name);
}

void entry_clearTimeout(bool *normalAndLow)
{
    // Your timeout clearing code here
    g_idleTime10Ms = 0; // 重置 idle 时间计数器，通常在系统进入低功耗模式前调用，确保下次唤醒时从零开始计时
    if (normalAndLow != nullptr)
    {
        *normalAndLow = true; // 重置正常模式标志
        module_Init();        // 重新初始化模块，恢复到正常工作状态
    }
}

void sleep_Timeout(bool *normalAndLow, bool clearFlag)
{
    // Your idle code here
    ++g_idleTime10Ms; // 这个示例只是简单地增加 idleTimeMs，实际应用中你可以根据需要进行更复杂的处理
    if (g_idleTime10Ms > 300)
    {
        g_idleTime10Ms = 300; // 防止溢出，最大记录 3000ms 的 idle 时间
    }
    if (clearFlag)
    {
        g_idleTime10Ms = 0;
        if (normalAndLow != nullptr)
        {
            *normalAndLow = true; // 如果 clearFlag 为 true，重置状态
        }
    }
    else if (g_idleTime10Ms >= 300) // 无任务 执行超过 3s，进入 idle 状态
    {
        if (normalAndLow != nullptr)
        {
            *normalAndLow = false; // 降低进入次数
        }
        SILABS_LOG("Idle for %lu ms", g_idleTime10Ms);
    }
}

/**
 * @brief 📬 主循环阶段：在这里处理所有的业务逻辑，包括电源管理、按键处理等
 * @param  InterruptWake_up 由外部事件触发的唤醒标志，指示当前循环是否是被外部事件唤醒的
 * @return 返回值控制循环的行为，例如是否继续执行、是否进入低功耗模式等
 */
bool entry_Loop(bool *InterruptWake_up)
{
#define debugLog 1
    static bool     normalAndLow = true; // 当前是否处于正常模式（true）还是低频模式（false）的标志，由电量过低保护逻辑控制
    static uint32_t time_clk = 0;        // 全局时钟计数器，单位为10ms，供整个应用程序使用
    static uint32_t time_detect_bat = 0; // 上次检测电池状态的时间戳，单位为10ms
#if debugLog
    static uint32_t debugTick = 0;
#endif

    // 中断唤醒
    if (*InterruptWake_up)
    {
        *InterruptWake_up = false; // 重置唤醒标志，准备下一次被唤醒
        // 这里可以添加一些针对被外部事件唤醒时需要立即处理的逻辑，例如快速响应某些紧急事件
        SILABS_LOG("====> [entry_Loop] 被外部事件唤醒，执行快速响应逻辑 <====");
        // 例如：检查某个特定的标志位，或者直接调用一个紧急处理函数
        entry_clearTimeout(&normalAndLow);
    }
    // 外部电源唤醒
    if (normalAndLow == false)
    {
        GetExternPowerFlag(); // 获取电源状态，更新全局变量 eg_UpPowerStatus
        // 电源切换回到正常模式的条件判断
        if (eg_PowerStatus != eg_UpPowerStatus)
        {
            entry_clearTimeout(&normalAndLow);
        }
    }
    if (normalAndLow == true)
    {
        // 获取计时器的值
        time_clk++;

        // 获取电源状态
        GetExternPowerFlag();

        // 电源管理 用于电源状态的切换
        PowerSwitchAssignment();

        LED_Tick10ms();                              // LED 驱动的定时器滴答函数，处理渐变效果和状态更新
        LED_SetLowBatteryProtection(g_PowerProtect); // 根据电量过低保护标志设置 LED 的保护状态，物理上锁定或解锁 LED 输出
        CommissioningFirstBreath_Poll();             // 开箱首次配网白光呼吸（与充电 Indic 呼吸独立）
        // 电源模式
        if (eg_PowerStatus == false)
        {
            // 电池模式
            if (time_clk - time_detect_bat > 100) // 1s 检测一次电池状态
            {
                time_detect_bat = time_clk;
                GetDisChargeStatus();
                SILABS_LOG("Battery Status=%d", eg_BatStatus);
                extern unsigned int g_ADBatLowVal;
                SILABS_LOG("BatVol=%d mv", g_ADBatLowVal * 3);
                if (eg_BatStatus == Bat_LowVolProt)
                {
                    g_PowerProtect = true; // 设置电量过低保护标志，禁止所有操作
                }
            }
            BatOutEn();
        }
        else
        {
            // 外部供电模式
            ChargeTimeUpdata(); // 充电时间更新
            BatOutDis();
            ChargeLogic(ChargeDetect());            // 400ms 充电逻辑处理一次，根据充电检测结果调整充电状态
            ChargeCurrentCtrlOut(led_get_status()); // 根据 LED 状态调整充电电流，控制充电速度
            change_led_Indic(eg_BatStatus);         // 根据电池状态调整指示灯显示
        }
        // 指示灯相关逻辑
        Indic_W_Breath_Poll_10ms();
        Indic_Red_Blink_Control_Dispatch();
        Indic_Red_Blink_Poll_10ms();

        // usb供电 不休眠
        // led亮着 不休眠
        sleep_Timeout(&normalAndLow, ((eg_PowerStatus == true) || ((eg_PowerStatus == false) && (led_get_status()) && (g_PowerProtect == false))));
        // 调用 sleep_Timeout 函数，传入当前时钟计数器和 clearFlag 标志
        if (normalAndLow == false) // 如果 IdleTimeout 返回 true，说明满足某些条件，可以进入低功耗模式
        {
            SILABS_LOG("满足进入低功耗模式的条件，执行相关操作...");
            module_Deinit(); // 反初始化模块，释放资源，准备进入低功耗模式
        }

#if debugLog
        // dump_power_debug();
        if (time_clk - debugTick > 200)
        {
            debugTick = time_clk;
            SILABS_LOG("tick=%d", debugTick);
            extern uint16_t g_ChargeTimeSec;
            extern uint16_t g_ADTemperature;
            SILABS_LOG("bat Temperature=%d", g_ADTemperature);
            SILABS_LOG("charge time=%d", g_ChargeTimeSec);
            SILABS_LOG("bat status=%d", eg_BatStatus);
            //  SILABS_LOG("identify=%d", g_Identify);

            //  check_interrupt_injection_status(5);
            //   CheckTickResolution(); // 检查系统 Tick 的时间分辨率，确保定时器逻辑的正确性
            //  my_pwm_set_duty_cycle_v1000(&sl_pwm_w_led0, 500); // 设置占空比为50%
            //  my_pwm_set_duty_cycle_v1000(&sl_pwm_Indic_led0, 250); // 设置占空比为25%
            //  sl_pwm_start(&sl_pwm_w_led0);
            //  sl_pwm_start(&sl_pwm_Indic_led0);

            // write_led_example();
        }
#endif
        // 返回值可以用来控制是否继续执行循环，或者是切换到不同的处理模式
    }
    return normalAndLow;
}

/**
 * @brief 📬 电源切换函数，根据当前电源状态进行相应的初始化和状态更新
 */
void PowerSwitchAssignment(void)
{
    if (eg_PowerStatus != eg_UpPowerStatus)
    {
        eg_PowerStatus = eg_UpPowerStatus;
        if (eg_PowerStatus == true) // 外部供电状态
        {
            eg_BatStatus = Bat_ChargeInit;
        }
        else // 电池供电状态
        {
            eg_BatStatus = Bat_DisCharge;
        }
        g_PowerProtect = false;                       // 切换电源时重置电量过低保护标志，允许正常操作
        PowerManageInit();                            // 电源管理初始化
        Indic_W_Breath_Stop();                        // 切换电源时停止独立指示白色呼吸灯，避免状态混乱
        Indic_Red_Blink_Normal_Flag_Set(false, NULL); // 切换电源时关闭红色错误闪烁，等待新的状态更新来决定是否需要开启

        // IndicInit();       // 指示灯初始化
        // led初始化
        // eg_ledData.CloseProtection = false;
        // eg_ledData.HistoryProtection = false;
        // eg_ledData.Protection = false;
    }
}

void change_led_Indic(uint8_t charge_status)
{
    static uint8_t s_status = 0xFF;

    while (charge_status != s_status)
    {
        s_status = charge_status;
        if (s_status == Bat_InCharge)
        {
            Indic_W_Breath_Start(100);                    // 充满电后呼吸提示，亮度 100%，颜色索引 0，周期 3200ms，持续呼吸
            Indic_Red_Blink_Normal_Flag_Set(false, NULL); // 关闭红色错误闪烁
            break;
        }

        Indic_W_Breath_Stop(); // 其他状态停止呼吸提示
        if (s_status == Bat_Nobat || s_status == Bat_HighTemp)
        {
            blink_normal_cfg_t s_normal_cfg = {400, 0};           // 默认值：400ms 持续闪烁
            Indic_Red_Blink_Normal_Flag_Set(true, &s_normal_cfg); // 无电池状态，红色正常闪烁提示
            break;
        }
    }
}

static void on_reset_effect_end_callback(void)
{
    // 特效完全结束了，手动设置目标状态并触发渐变
    led_set_status(true);
    led_set_color_index(2);
    led_update_normal_state(led_get_status(), led_get_brightness(), led_get_color_table(led_get_color_index()));
    // 触发 400ms 的平滑过渡
    LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);
}

/**
 * @brief 📬 重置matter配网，灯效启动
 * @param  无
 */
void ResetMatterNetworkLightingEffects(void)
{
#define MixedLightingEffectsCount 4
    led_color_t raw_color = led_get_raw_color(); // 获取当前原始颜色，准备在特效中使用

    led_set_color_index(2);                                               // 将当前颜色设置为原始颜色，确保特效从当前颜色开始变化
    led_color_t raw_color_2 = led_get_color_table(led_get_color_index()); // 获取当前原始颜色，准备在特效中使用

    MixedLightingEffects_t MixedLightingEffects_Start_Example[MixedLightingEffectsCount] = {
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 255, .raw_color = raw_color, .fade_ms = 800, .count = 3},
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 255, .raw_color = raw_color, .fade_ms = 2400, .count = 1},
        {.fuction_mode = LED_EFFECT_HOLD, .is_on = false, .brightness = 255, .raw_color = raw_color_2, .fade_ms = 2000, .count = 1},
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 255, .raw_color = raw_color_2, .fade_ms = 400, .count = 2},
    }; // 最后一个 NONE 代表特效序列结束

    memcpy(g_mixed_effects, MixedLightingEffects_Start_Example, sizeof(MixedLightingEffects_Start_Example));

    // 启动混合特效
    Led_MixedLightingEffects_Start(MixedLightingEffectsCount, on_reset_effect_end_callback);

    // 红色指示灯闪烁
    blink_mixed_cfg_t s_mixed_cfg = {800, 3, 2400, 1};
    Indic_Red_Blink_Mixed_Flag_Set(true, &s_mixed_cfg); // 启动混合双速闪烁

    // Your reset logic here
    SILABS_LOG("开启配网等效的灯效提示...");
    // 这里可以添加任何需要在重置时执行的清理代码，例如重置全局变量、清除持久化存储等
}

/**
 * @brief 📬 重置matter配网：当用户长按达到重置条件时调用，执行相关的重置逻辑
 * @param  无
 */
void ResetMatterNetworkConfiguration(void)
{
    // Your reset logic here
    SILABS_LOG("[app]重置Matter网络配置...");
    // 这里可以添加任何需要在重置时执行的清理代码，例如重置全局变量、清除持久化存储等
    // TriggerNetworkResetWithoutReboot(); // 执行在线网络重置的核心逻辑
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
    LED_StopMixedEffects();
    Indic_Red_Blink_Stop(); // 停止红色指示灯闪烁
}

/**
 * @brief 📬 配网成功动画：当设备成功配对到网络后调用，执行一段特定的灯效动画来提示用户
 */
void TriggerPairingSuccessAnimation(void)
{
#define SuccessEffectsCount 1

    led_color_t            raw_color = led_get_raw_color(); // 获取当前原始颜色，准备在特效中使用
    MixedLightingEffects_t Success_Effects[SuccessEffectsCount] = {
        // Step 0: 以 60% 亮度快速闪烁 2 次（亮 200ms，灭 200ms，周期 400ms，次数 2）
        {.fuction_mode = LED_EFFECT_BLINK, .is_on = true, .brightness = 60, .raw_color = raw_color, .fade_ms = 800, .count = 2},
    };

    memcpy(g_mixed_effects, Success_Effects, sizeof(Success_Effects));

    // 启动混合灯效
    Led_MixedLightingEffects_Start(1, NULL);
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

    // 上报当前状态到 Matter 层，确保手机端状态同步
    extern void Upload_Matter_OnOff(bool is_on);
    extern void Upload_Matter_Brightness(uint8_t driver_brightness_percent);

    switch (action)
    {
    case AppButtonEvent::kButtonAction_ShortPress:
    {
        SILABS_LOG(" -> [业务确诊] 按键 %d : 单击触发！", button_idx);
        // 这里写你的单击控制代码
        // 如果当前状态变化来源是远程下发
        if (led_get_change_origin() == LED_ORIGIN_MATTER_APP && led_get_status() == true) // 并且灯当前是开的
        {
            SILABS_LOG("当前LED由远程控制打开，单击事件优先关灯");

            Upload_Matter_OnOff(led_get_status());
            Upload_Matter_Brightness((uint16_t)(led_get_brightness() + 1) * 100 >> 8); // 上报亮度状态

            led_set_status(false);
            led_set_brightness(0);
            led_set_change_origin(LED_ORIGIN_LOCAL_KEY); // 标记状态变化来源为本地按键
            LED_SaveStateToFlash();
            LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);

            break;
        }
        if (eg_BatStatus <= Bat_LowVolWarn)
        {
            SILABS_LOG("当前电池电量过低，红色闪烁提示");
            blink_normal_cfg_t s_normal_cfg = {400, 2};           // 默认值
            Indic_Red_Blink_Normal_Flag_Set(true, &s_normal_cfg); // 红色正常闪烁提示
        }
        if (g_PowerProtect == true) // 额外的电量过低保护逻辑，防止在电池电量过低时误操作导致设备无法正常使用
        {
            break;
        }
        if (led_get_status() == false || led_get_brightness() == 0)
        {
            led_set_status(true);
            led_set_brightness(LED_BRIGHTNESS_MAX); // 恢复到最大亮度
        }
        else if (led_get_status() == true && led_get_brightness() == LED_BRIGHTNESS_MAX)
        {
            led_set_brightness(LED_BRIGHTNESS_MAX >> 1); // 50% 亮度
        }
        else if (led_get_status() == true && led_get_brightness() == (LED_BRIGHTNESS_MAX >> 1))
        {
            led_set_status(false);
            led_set_brightness(0);
        }
        else
        {
            led_set_status(false);
            led_set_brightness(0);
        }

        // 上报当前状态到 Matter 层，确保手机端状态同步
        extern void Upload_Matter_OnOff(bool is_on);
        extern void Upload_Matter_Brightness(uint8_t driver_brightness_percent);

        Upload_Matter_OnOff(led_get_status());
        Upload_Matter_Brightness((uint16_t)(led_get_brightness() + 1) * 100 >> 8); // 上报亮度状态

        SILABS_LOG("is_On=%d, brightness=%d", led_get_status(), led_get_brightness());

        // 传递变化
        led_set_change_origin(LED_ORIGIN_LOCAL_KEY);
        led_update_normal_state(led_get_status(), led_get_brightness(), led_get_raw_color());
        LED_SaveStateToFlash();
        LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);
        break;
    }
    case AppButtonEvent::kButtonAction_DoublePress:
    {
        SILABS_LOG(" -> [业务确诊] 按键 %d : 双击触发！", button_idx);
        if (led_get_status() == false)
        {
            SILABS_LOG("当前LED熄灭！！！双击事件不执行任何操作");
            break;
        }
        if (eg_BatStatus <= Bat_LowVolWarn)
        {
            SILABS_LOG("当前电池电量过低，红色闪烁提示");
            blink_normal_cfg_t s_normal_cfg = {400, 2};           // 默认值
            Indic_Red_Blink_Normal_Flag_Set(true, &s_normal_cfg); // 红色正常闪烁提示
        }
        if (g_PowerProtect == true)
        {
            SILABS_LOG("当前LED处于保护状态！！！双击事件不执行任何操作");
            break;
        }
        // 这里写你的双击控制代码
        uint8_t color_index = led_get_color_index() + 1;
        if (color_index >= LED_COLOR_COUNT)
        {
            color_index = 0;
        }

        Upload_Matter_OnOff(led_get_status());                                     // 上报开关状态
        Upload_Matter_Brightness((uint16_t)(led_get_brightness() + 1) * 100 >> 8); // 上报亮度状态

        SILABS_LOG("color_index=%d", led_get_color_index());

        led_set_color_index(color_index); // 触发颜色表更新，确保颜色正确
        led_set_change_origin(LED_ORIGIN_LOCAL_KEY);
        led_set_color_source(LED_SOURCE_INDEX_TABLE); // 双击切换颜色时，强制使用颜色表作为颜色来源，确保颜色正确更新
        led_update_normal_state(led_get_status(), led_get_brightness(), led_get_color_table(led_get_color_index()));
        LED_SaveStateToFlash();
        LED_Start_Fade_To_Current(LED_FADE_COLOR_SWITCH_MS);

        break;
    }
    case AppButtonEvent::kButtonAction_LongPressStart:
    {
        SILABS_LOG(" -> [业务确诊] 按键 %d : 开始长按！", button_idx);
        break;
    }
    case AppButtonEvent::kButtonAction_LongPressing:
    {
        if (g_PowerProtect) // 额外的电量过低保护逻辑，防止在电池电量过低时误操作导致设备无法正常使用
        {
            break;
        }
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
    }
    case AppButtonEvent::kButtonAction_LongPressRelease:
    {
        if (g_PowerProtect) // 额外的电量过低保护逻辑，防止在电池电量过低时误操作导致设备无法正常使用
        {
            break;
        }
        // 长按时间不足过程不重置配网，但未达到放弃配网的条件
        if (save_history_long_press_count >= 25 && save_history_long_press_count < 50)
        {
            GiveUpMatterNetworkConfiguration(); // 长按过程中触发放弃配网
        }
        SILABS_LOG(" -> [业务确诊] 按键 %d : 长按松开！ 脉冲计数 = %d", button_idx, save_history_long_press_count);
        // 🧼 善后清理：手终于松开了，把重置旗标清空，允许下一次长按重置
        break;
    }

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

#if 0
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
#endif