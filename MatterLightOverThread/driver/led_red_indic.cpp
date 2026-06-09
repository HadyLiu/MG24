#include "led_red_indic.h"

#include <stdbool.h>
#include <stdint.h>

// =========================================================================
// 1. 模块私有类型定义
// =========================================================================
typedef enum
{
    MODE_NORMAL = 0,     // 普通单一闪烁模式
    MODE_MIXED_BLINK = 1 // 混合单向闪烁模式
} blink_mode_t;

typedef enum
{
    STAGE_MODE1_SPEED = 0, // 模式1速度阶段
    STAGE_MODE2_SPEED = 1  // 模式2速度阶段
} blink_stage_t;

// =========================================================================
// 2. 模块内部私有状态变量 (全部 static 限制作用域)
// =========================================================================
static volatile bool s_blink_normal_flag = false; // 普通闪烁控制标志
static volatile bool s_blink_mixed_flag = false;  // 混合闪烁控制标志（高优先级）

static bool     s_blink_active = false;   // 闪烁激活状态
static bool     s_led_is_on = false;      // LED 当前物理电平状态
static uint16_t s_timer_down_counter = 0; // 当前电平状态维持的递减倒计时
static uint16_t s_cur_reload_ticks = 0;   // 运行时重装载的半周期 Tick 常数
static uint16_t s_rem_blink_count = 0;    // 普通模式：剩余闪烁次数

static blink_mode_t  s_blink_mode = MODE_NORMAL;
static blink_stage_t s_blink_stage = STAGE_MODE1_SPEED;
static uint16_t      s_m1_ticks = 0;     // 模式1速度半周期 Tick 数
static uint16_t      s_m2_ticks = 0;     // 模式2速度半周期 Tick 数
static uint16_t      s_rem_m1_count = 0; // 运行中：模式1剩余闪烁次数
static uint16_t      s_rem_m2_count = 0; // 运行中：模式2剩余闪烁次数

// 模拟硬件接口，请根据实际底层替换
#define INDIC_R_LED_ON()
#define INDIC_R_LED_OFF()

// =========================================================================
// 3. 对外公共 Get/Set 接口
// =========================================================================

/* 普通单速闪烁标志的 Get/Set */
bool Indic_Red_Blink_Normal_Flag_Get(void)
{ return s_blink_normal_flag; }

void Indic_Red_Blink_Normal_Flag_Set(bool enable)
{ s_blink_normal_flag = enable; }

/* 混合双速闪烁标志的 Get/Set */
bool Indic_Red_Blink_Mixed_Flag_Get(void)
{ return s_blink_mixed_flag; }

/**
 * @brief 设置混合双速闪烁标志位的接口函数
 */
void Indic_Red_Blink_Mixed_Flag_Set(bool enable)
{
    s_blink_mixed_flag = enable;

    // 💡 优化体验：如果外部主动设置高优先级标志为 true，
    // 我们可以在接口层直接顺手把低优先级标志清掉，防止冲突。
    if (enable)
    {
        s_blink_normal_flag = false;
    }
}

// =========================================================================
// 4. 内部基础驱动函数
// =========================================================================
void Indic_Red_Blink_Stop(void)
{
    s_blink_active = false;
    INDIC_R_LED_OFF();
}

void Indic_Red_Blink_Start(uint16_t duration_ms, uint16_t blink_count)
{
    s_cur_reload_ticks = (duration_ms / 10) >> 1;
    if (s_cur_reload_ticks == 0)
        s_cur_reload_ticks = 1;

    s_timer_down_counter = s_cur_reload_ticks;
    s_rem_blink_count = blink_count;
    s_blink_mode = MODE_NORMAL;

    s_led_is_on = true;
    INDIC_R_LED_ON();
    s_blink_active = true;
}

void Indic_Red_Mixed_Blink_Start(uint16_t m1_duration_ms, uint16_t m1_count, uint16_t m2_duration_ms, uint16_t m2_count)
{
    s_m1_ticks = (m1_duration_ms / 10) >> 1;
    s_m2_ticks = (m2_duration_ms / 10) >> 1;
    if (s_m1_ticks == 0)
        s_m1_ticks = 1;
    if (s_m2_ticks == 0)
        s_m2_ticks = 1;

    s_blink_mode = MODE_MIXED_BLINK;
    s_blink_stage = STAGE_MODE1_SPEED;
    s_rem_m1_count = m1_count;
    s_rem_m2_count = m2_count;

    s_cur_reload_ticks = s_m1_ticks;
    s_timer_down_counter = s_m1_ticks;

    s_led_is_on = true;
    INDIC_R_LED_ON();
    s_blink_active = true;
}

// =========================================================================
// 5. 控制层分发器（建议在业务主循环 while(1) 中调用）
// =========================================================================
void Indic_Red_Blink_Control_Dispatch(void)
{
    // ----- 优先级 1：混合双速闪烁标志被置位 -----
    if (s_blink_mixed_flag)
    {
        if (!s_blink_active || (s_blink_mode != MODE_MIXED_BLINK))
        {
            s_blink_normal_flag = false;                 // 确保清除低优先级标志
            Indic_Red_Mixed_Blink_Start(500, 3, 200, 2); // 示例参数：慢闪3次，快闪2次
        }
        return;
    }

    // ----- 优先级 2：普通单速闪烁标志被置位 -----
    if (s_blink_normal_flag)
    {
        if (!s_blink_active)
        {
            Indic_Red_Blink_Start(500, 5); // 示例参数：单次闪烁500ms，闪5次
        }
    }
}

// =========================================================================
// 6. 核心融合驱动器（必须被 10ms 定时器严格调用）
// =========================================================================
void Indic_Red_Blink_Poll_10ms(void)
{
    // ------------ 1. 双向联动：外部通过接口清除标志位，内部联动中止动画 ------------
    if (s_blink_active)
    {
        if (s_blink_mode == MODE_NORMAL && !s_blink_normal_flag)
        {
            Indic_Red_Blink_Stop();
            return;
        }
        else if (s_blink_mode == MODE_MIXED_BLINK && !s_blink_mixed_flag)
        {
            Indic_Red_Blink_Stop();
            return;
        }
    }
    else
    {
        return;
    }

    // ------------ 2. 状态机递减与翻转逻辑 ------------
    if (--s_timer_down_counter == 0)
    {
        s_timer_down_counter = s_cur_reload_ticks;

        if (s_led_is_on)
        {
            s_led_is_on = false;
            INDIC_R_LED_OFF(); // 变灭 (结算次数)

            // A. 处理【普通单一模式】
            if (s_blink_mode == MODE_NORMAL)
            {
                if (s_rem_blink_count > 0)
                {
                    if (--s_rem_blink_count == 0)
                    {
                        // 🎯 动画自然结束：关闭动画，并通过内部直接清除标志位
                        Indic_Red_Blink_Stop();
                        s_blink_normal_flag = false;
                        return;
                    }
                }
            }
            // B. 处理【混合双速单向多次模式】
            else
            {
                if (s_blink_stage == STAGE_MODE1_SPEED)
                {
                    if (--s_rem_m1_count == 0)
                    {
                        s_blink_stage = STAGE_MODE2_SPEED;
                        s_cur_reload_ticks = s_m2_ticks;
                        s_timer_down_counter = s_m2_ticks;
                    }
                }
                else
                {
                    if (--s_rem_m2_count == 0)
                    {
                        // 🎯 动画自然结束：关闭动画，并通过内部直接清除标志位
                        Indic_Red_Blink_Stop();
                        s_blink_mixed_flag = false;
                        return;
                    }
                }
            }
        }
        else
        {
            s_led_is_on = true;
            INDIC_R_LED_ON(); // 变亮
        }
    }
}
