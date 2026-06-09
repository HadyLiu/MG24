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

// 🎯 新增：用于缓存外部运行时配置的结构体变量
static blink_normal_cfg_t s_normal_cfg = {400, 0};         // 默认值：400ms 持续闪烁
static blink_mixed_cfg_t  s_mixed_cfg = {800, 3, 2400, 1}; // 默认值

// =========================================================================
// 3. 对外公共 Get/Set 接口
// =========================================================================

/* 普通单速闪烁的 Get/Set (支持传入结构体参数) */
bool Indic_Red_Blink_Normal_Flag_Get(void)
{ return s_blink_normal_flag; }

void Indic_Red_Blink_Normal_Flag_Set(bool enable, const blink_normal_cfg_t *cfg)
{
    s_blink_normal_flag = enable;
    if (enable && cfg != NULL)
    {
        s_normal_cfg = *cfg; // 浅拷贝外部配置
    }
}

/* 混合双速闪烁的 Get/Set (支持传入结构体参数) */
bool Indic_Red_Blink_Mixed_Flag_Get(void)
{ return s_blink_mixed_flag; }

void Indic_Red_Blink_Mixed_Flag_Set(bool enable, const blink_mixed_cfg_t *cfg)
{
    s_blink_mixed_flag = enable;
    if (enable && cfg != NULL)
    {
        s_mixed_cfg = *cfg; // 浅拷贝外部配置
    }
}

// =========================================================================
// 4. 内部基础驱动函数 (调整为接收结构体指针)
// =========================================================================
void Indic_Red_Blink_Stop(void)
{
    s_blink_active = false;
    INDIC_R_LED_OFF();
}

static void Indic_Red_Blink_Start(const blink_normal_cfg_t *cfg)
{
    if (cfg == NULL)
        return;

    s_cur_reload_ticks = (cfg->duration_ms / 10) >> 1;
    if (s_cur_reload_ticks == 0)
    {
        s_cur_reload_ticks = 1;
    }
    s_timer_down_counter = s_cur_reload_ticks;
    s_rem_blink_count = cfg->blink_count;
    s_blink_mode = MODE_NORMAL;

    s_led_is_on = true;
    INDIC_R_LED_ON();
    s_blink_active = true;
}

static void Indic_Red_Mixed_Blink_Start(const blink_mixed_cfg_t *cfg)
{
    if (cfg == NULL)
        return;

    s_m1_ticks = (cfg->m1_duration_ms / 10) >> 1;
    s_m2_ticks = (cfg->m2_duration_ms / 10) >> 1;

    if (s_m1_ticks == 0)
        s_m1_ticks = 1;
    if (s_m2_ticks == 0)
        s_m2_ticks = 1;

    s_blink_mode = MODE_MIXED_BLINK;
    s_blink_stage = STAGE_MODE1_SPEED;
    s_rem_m1_count = cfg->m1_count;
    s_rem_m2_count = cfg->m2_count;

    s_cur_reload_ticks = s_m1_ticks;
    s_timer_down_counter = s_m1_ticks;

    s_led_is_on = true;
    INDIC_R_LED_ON();
    s_blink_active = true;
}

// =========================================================================
// 5. 控制层分发器
// =========================================================================
void Indic_Red_Blink_Control_Dispatch(void)
{
    // ----- 优先级 1：混合双速闪烁 -----
    if (s_blink_mixed_flag)
    {
        if (!s_blink_active || (s_blink_mode != MODE_MIXED_BLINK))
        {
            // 🎯 动态使用外部通过 Set 接口传入的配置
            Indic_Red_Mixed_Blink_Start(&s_mixed_cfg);
        }
        return;
    }

    // ----- 优先级 2：普通单速闪烁 -----
    if (s_blink_normal_flag)
    {
        if (!s_blink_active)
        {
            // 🎯 动态使用外部通过 Set 接口传入的配置
            Indic_Red_Blink_Start(&s_normal_cfg);
        }
    }
}

// =========================================================================
// 6. 核心融合驱动器（必须被 10ms 定时器严格调用）
// =========================================================================
void Indic_Red_Blink_Poll_10ms(void)
{
    // ------------ 1. 双向联动 ------------
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
            INDIC_R_LED_OFF();

            // A. 处理【普通单一模式】
            if (s_blink_mode == MODE_NORMAL)
            {
                if (s_rem_blink_count > 0)
                {
                    if (--s_rem_blink_count == 0)
                    {
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
            INDIC_R_LED_ON();
        }
    }
}
