#pragma once

#include <stdint.h>

#include "pinManage.h"

// 模拟硬件接口，请根据实际底层替换
// #define INDIC_R_LED_ON()
// #define INDIC_R_LED_OFF()

bool Indic_Red_Blink_Normal_Flag_Get(void);
void Indic_Red_Blink_Normal_Flag_Set(bool enable);
bool Indic_Red_Blink_Mixed_Flag_Get(void);
void Indic_Red_Blink_Mixed_Flag_Set(bool enable);

void Indic_Red_Blink_Stop(void);
void Indic_Red_Blink_Start(uint16_t duration_ms, uint16_t blink_count);
void Indic_Red_Mixed_Blink_Start(uint16_t m1_duration_ms, uint16_t m1_count, uint16_t m2_duration_ms, uint16_t m2_count);

void Indic_Red_Blink_Control_Dispatch(void);
void Indic_Red_Blink_Poll_10ms(void);
