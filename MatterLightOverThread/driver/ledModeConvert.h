#pragma once

#include <stdint.h>

#define LED_HW_MAX 1023

void LedDriver_ConvertHsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);

void Light_Calc_CT_To_WRGB(uint32_t kelvin, uint8_t *out_w, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b);

void MyCalculatedRGB(uint16_t chipX, uint16_t chipY, uint8_t *outR, uint8_t *outG, uint8_t *outB);
