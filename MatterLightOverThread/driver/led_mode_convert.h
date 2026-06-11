#pragma once

#include <stdint.h>

#define LED_HW_MAX 1023

void LedDriver_ConvertHsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint16_t *r, uint16_t *g, uint16_t *b);

void Light_Calc_CT_To_WRGB(uint32_t kelvin, uint16_t *out_w, uint16_t *out_r, uint16_t *out_g, uint16_t *out_b);

void Light_Calc_XY_To_RGB(uint16_t currentX, uint16_t currentY, uint16_t *r, uint16_t *g, uint16_t *b);
