#include "indicLed.h"

static const uint16_t g_ease_quad_table_4096[81] = {
    0,    1,    5,    11,   20,   32,   46,   62,   81,   103,  128,  154,  184,  216,  251,  288,  327,  370,  414,  462,  512,
    564,  620,  677,  737,  800,  865,  933,  1004, 1077, 1152, 1230, 1311, 1394, 1479, 1567, 1658, 1750, 1845, 1943, 2048, 2153,
    2251, 2346, 2438, 2529, 2617, 2702, 2785, 2866, 2944, 3019, 3092, 3163, 3231, 3296, 3359, 3419, 3476, 3532, 3584, 3634, 3682,
    3726, 3769, 3808, 3845, 3880, 3912, 3942, 3968, 3993, 4015, 4034, 4050, 4064, 4076, 4085, 4091, 4095, 4096};

// 2. 模块私有全局变量
static bool     s_breath_active = false; // 呼吸灯激活标志
static uint16_t s_breath_tick = 0;       // 0 ~ 319 闭环计数器
static uint8_t  s_brightness = 100;      // 最大限制亮度 (0 ~ 100)

// 外部硬件函数声明：请在这里引入你底层专门写这唯一一路指示灯占空比的驱动函数
// 假设原函数是类似于下面这个形式，只要传入 0 ~ 1023 的数值即可

/**
 * @brief 内部私有：单向表逆向复用获取当前 10ms 对应的波形比例
 */
static uint16_t get_breath_scale(uint16_t tick)
{
    if (tick < 80) // 阶段 1：0 ~ 790ms -> 淡入
    {
        return g_ease_quad_table_4096[tick];
    }
    else if (tick < 160) // 阶段 2：800 ~ 1590ms -> 高亮度维持
    {
        return 4096;
    }
    else if (tick < 240) // 阶段 3：1600 ~ 2390ms -> 淡出 (逆向复用)
    {
        return g_ease_quad_table_4096[240 - tick];
    }

    // 阶段 4：2400 ~ 3190ms -> 全灭维持
    return 0;
}

void Indic_W_Breath_Start(uint8_t brightness)
{
    s_brightness = brightness > 100 ? 100 : brightness;
    s_breath_tick = 0;      // 从头起步
    s_breath_active = true; // 激活独立轮询
}

void Indic_W_Breath_Stop(void)
{
    s_breath_active = false;
    Indic_SetDuty(0); // 叫停时立即让指示灯全灭
}

static uint32_t led_map_app_percent_to_output_scale_14bit(uint16_t app_percent)
{
    if (app_percent <= 1)
    {
        return 204; // 2% * 10240
    }
    if (app_percent >= 100)
    {
        return 10240; // 100% * 10240
    }

    // 纯整数算术公式（先乘后除防止丢精度）
    return 204 + ((uint32_t)(app_percent - 1) * 10035) / 99;
}

void Indic_W_Breath_Poll_10ms(void)
{
    // 未激活时，进来判断一下直接秒退，耗时接近 0
    if (!s_breath_active)
    {
        return;
    }

    // 1. 获取当前的呼吸波形缩放因子 (0 ~ 4096)
    uint32_t breath_scale = get_breath_scale(s_breath_tick);

    // 2. 闭环时间步进器 (0~319 循环)
    if (++s_breath_tick >= 320)
    {
        s_breath_tick = 0;
    }

    // 3. 获取 App 全局亮度百分比对应的 14-bit 权重最大值 (0 ~ 10240)
    uint32_t out_scale_14bit = led_map_app_percent_to_output_scale_14bit(s_brightness);

    // 4. 🎯 终极优化一：合并所有多余层级
    // 原理：既然没有颜色干扰，我们只需要让波形比例(0~4096)乘以最大亮度因子(0~10240)，再融入你的硬件最大上限
    // 指示灯最大的 PWM 目标是 1023，我们利用移位来平滑映射：
    // (out_scale_14bit * breath_scale) 的上限是 10240 * 4096 = 41,943,040 (26位，32位硬件乘法单周期搞定)
    // 要把它精确压缩到最大 1023 (10位)，由于 14位 + 12位 = 26位，想要降回 10位，正好需要右移：26 - 10 = 16 位！
    // 配合 +32768 进行半步四舍五入，消除硬件取整时在低暗度下的抖动闪烁：
    uint16_t target_pwm = (uint16_t)(((out_scale_14bit * breath_scale) + 32768) >> 16);

    // 5. 🎯 优化二：一步到位，单条汇编级操作直接灌入硬件 PWM 寄存器
    Indic_SetDuty(target_pwm);
}

/****
 *
 *
 * 红色指示灯逻辑
 *
 *
 *
 *
 */

// 大控制模式定义
typedef enum
{
    MODE_NORMAL = 0,     // 普通单一闪烁模式
    MODE_MIXED_BLINK = 1 // 混合单向闪烁模式
} blink_mode_t;

// 混合闪烁子阶段
typedef enum
{
    STAGE_MODE1_SPEED = 0, // 模式1速度阶段
    STAGE_MODE2_SPEED = 1  // 模式2速度阶段
} blink_stage_t;

// 模块内部私有状态变量
static bool     s_blink_active = false;
static bool     s_led_is_on = false;
static uint16_t s_timer_down_counter = 0; // 当前电平状态维持的递减倒计时
static uint16_t s_cur_reload_ticks = 0;   // 运行时重装载的半周期 Tick 常数
static uint16_t s_rem_blink_count = 0;    // 普通模式：剩余闪烁次数

// 混合双速单向模式运行暂存参数
static blink_mode_t  s_blink_mode = MODE_NORMAL;
static blink_stage_t s_blink_stage = STAGE_MODE1_SPEED;
static uint16_t      s_m1_ticks = 0;     // 模式1速度半周期 Tick 数
static uint16_t      s_m2_ticks = 0;     // 模式2速度半周期 Tick 数
static uint16_t      s_rem_m1_count = 0; // 运行中：模式1剩余闪烁次数
static uint16_t      s_rem_m2_count = 0; // 🎯 运行中：模式2剩余闪烁次数

// ==================== 【功能 1：普通单速模式】 ====================
void Indic_Red_Blink_Start(uint16_t duration_ms, uint16_t blink_count)
{
    s_cur_reload_ticks = (duration_ms / 10) >> 1;
    if (s_cur_reload_ticks == 0)
    {
        s_cur_reload_ticks = 1;
    }

    s_timer_down_counter = s_cur_reload_ticks;
    s_rem_blink_count = blink_count;
    s_blink_mode = MODE_NORMAL;

    s_led_is_on = true;
    INDIC_R_LED_ON();
    s_blink_active = true;
}

// ==================== 【功能 2：双速单向混合多次模式】 ====================
void Indic_Red_Mixed_Blink_Start(uint16_t m1_duration_ms, uint16_t m1_count, uint16_t m2_duration_ms, uint16_t m2_count)
{
    // 转换为半周期 Tick 数
    s_m1_ticks = (m1_duration_ms / 10) >> 1;
    s_m2_ticks = (m2_duration_ms / 10) >> 1;
    if (s_m1_ticks == 0)
        s_m1_ticks = 1;
    if (s_m2_ticks == 0)
        s_m2_ticks = 1;

    s_blink_mode = MODE_MIXED_BLINK;
    s_blink_stage = STAGE_MODE1_SPEED; // 固定从模式1速度起步
    s_rem_m1_count = m1_count;
    s_rem_m2_count = m2_count; // 🎯 初始化模式2的闪烁次数

    s_cur_reload_ticks = s_m1_ticks;
    s_timer_down_counter = s_m1_ticks;

    // 立刻点亮红灯，激活执行器
    s_led_is_on = true;
    INDIC_R_LED_ON();
    s_blink_active = true;
}

void Indic_Red_Blink_Stop(void)
{
    s_blink_active = false;
    INDIC_R_LED_OFF();
}

// ==================== 【核心融合驱动器 (10ms一次)】 ====================
void Indic_Red_Blink_Poll_10ms(void)
{
    if (!s_blink_active)
    {
        return;
    }

    // 1. 递减当前状态的半周期倒计时
    if (--s_timer_down_counter == 0)
    {
        // 2. 倒计时到，无脑重装运行时准备好的 Tick（极致压榨 CPU 性能）
        s_timer_down_counter = s_cur_reload_ticks;

        // 3. 翻转 LED 物理电平
        if (s_led_is_on)
        {
            s_led_is_on = false;
            INDIC_R_LED_OFF(); // 变灭 (一个完整的亮灯半周期结束)

            // 4. 处理【普通单一模式】的次数流控
            if (s_blink_mode == MODE_NORMAL)
            {
                if (s_rem_blink_count > 0)
                {
                    if (--s_rem_blink_count == 0)
                    {
                        s_blink_active = false; // 次数到了，自动关闭
                        return;
                    }
                }
            }
            // 5. 处理【混合双速单向多次模式】
            else
            {
                if (s_blink_stage == STAGE_MODE1_SPEED)
                {
                    if (--s_rem_m1_count == 0)
                    {
                        // 模式1闪完了指定的 X 次，单向切换到模式2
                        s_blink_stage = STAGE_MODE2_SPEED;
                        s_cur_reload_ticks = s_m2_ticks;
                        s_timer_down_counter = s_m2_ticks;
                    }
                }
                else
                {
                    // 🎯 模式2速度也采用递减计数器控制
                    if (--s_rem_m2_count == 0)
                    {
                        // 模式2闪完了指定的 Y 次，整个流水线完美结束，安全注销并退灯
                        s_blink_active = false;
                        return;
                    }
                }
            }
        }
        else
        {
            // 变亮
            s_led_is_on = true;
            INDIC_R_LED_ON();
        }
    }
}
