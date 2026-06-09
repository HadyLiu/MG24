#pragma once

#include <stdint.h>

#include "pinManage.h"

// 模拟硬件接口，请根据实际底层替换
// #define INDIC_R_LED_ON()
// #define INDIC_R_LED_OFF()

// 普通单速闪烁配置结构体
typedef struct
{
    uint16_t duration_ms; // 单次闪烁周期(亮+灭的总时间)，单位 ms
    uint16_t blink_count; // 闪烁次数 (0 表示无限闪烁)
} blink_normal_cfg_t;

// 混合双速闪烁配置结构体
typedef struct
{
    uint16_t m1_duration_ms; // 阶段1单次闪烁周期，单位 ms
    uint16_t m1_count;       // 阶段1闪烁次数
    uint16_t m2_duration_ms; // 阶段2单次闪烁周期，单位 ms
    uint16_t m2_count;       // 阶段2闪烁次数
} blink_mixed_cfg_t;

bool Indic_Red_Blink_Normal_Flag_Get(void);
void Indic_Red_Blink_Normal_Flag_Set(bool enable, const blink_normal_cfg_t *cfg);
bool Indic_Red_Blink_Mixed_Flag_Get(void);
void Indic_Red_Blink_Mixed_Flag_Set(bool enable, const blink_mixed_cfg_t *cfg);

void Indic_Red_Blink_Stop(void);
void Indic_Red_Blink_Start(uint16_t duration_ms, uint16_t blink_count);
void Indic_Red_Mixed_Blink_Start(uint16_t m1_duration_ms, uint16_t m1_count, uint16_t m2_duration_ms, uint16_t m2_count);

void Indic_Red_Blink_Control_Dispatch(void);
void Indic_Red_Blink_Poll_10ms(void);
