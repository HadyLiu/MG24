#pragma once

#include "sl_pwm_instances.h"
#include "sl_pwm.h"
#if defined(_SILICON_LABS_32B_SERIES_2)
#include "em_timer.h"
#else
#include "sl_hal_timer.h"
#endif

#include "sm15135e.h"

void     my_pwm_set_duty_cycle_10bit_resolution(sl_pwm_instance_t *pwm, uint16_t duty_v1000);
uint16_t my_pwm_get_duty_cycle_v1000(sl_pwm_instance_t *pwm);

void Indic_SetDuty(uint16_t w_led);

void LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b);
