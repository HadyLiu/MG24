#pragma once

#include "sl_pwm.h"
#include "sl_pwm_instances.h"
#include <stdint.h>

#include "sm15135e.h"

extern void     my_pwm_set_duty_cycle_10bit_resolution(sl_pwm_instance_t *pwm, uint16_t duty_v1000);
extern uint16_t my_pwm_get_duty_cycle_v1000(sl_pwm_instance_t *pwm);

extern void LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b);