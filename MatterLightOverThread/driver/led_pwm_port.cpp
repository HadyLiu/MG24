#include "led_pwm_port.h"

#include "sl_pwm.h"
#if defined(_SILICON_LABS_32B_SERIES_2)
#include "em_timer.h"
#else
#include "sl_hal_timer.h"
#endif

extern sl_pwm_instance_t sl_pwm_w_led0;
extern sl_pwm_instance_t sl_pwm_Indic_led0;

/**
 * @brief 按照千分比(0~1000)设置PWM占空比
 * @param pwm PWM实例指针
 * @param duty_v1000 占空比数值，范围 0 到 1000（500代表50%）
 */
void my_pwm_set_duty_cycle_v1000(sl_pwm_instance_t *pwm, uint16_t duty_v1000)
{
    if (duty_v1000 > 1000) duty_v1000 = 1000; // 限幅防越界

#if defined(_SILICON_LABS_32B_SERIES_2)
    // 适用于 Series 2 芯片 (如 EFR32MG24)
    uint32_t top = TIMER_TopGet(pwm->timer);
    TIMER_CompareBufSet(pwm->timer, pwm->channel, (top * duty_v1000) / 1000);
#else
    // 适用于 Series 3 芯片
    uint32_t top = sl_hal_timer_get_top(pwm->timer);
    sl_hal_timer_channel_set_compare_buffer(pwm->timer, pwm->channel, (top * duty_v1000) / 1000);
#endif
}

/**
 * @brief 获取当前的千分比占空比
 * @return 返回 0 ~ 1000 的数值
 */
uint16_t my_pwm_get_duty_cycle_v1000(sl_pwm_instance_t *pwm)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
    uint32_t top = TIMER_TopGet(pwm->timer);
    uint32_t compare = TIMER_CaptureGet(pwm->timer, pwm->channel);
#else
    uint32_t top = sl_hal_timer_get_top(pwm->timer);
    uint32_t compare = sl_hal_timer_channel_get_compare(pwm->timer, pwm->channel);
#endif
    if (top == 0) return 0;
    return (uint16_t)((compare * 1000) / top);
}

sm15135e_pixel_t my_led;

void LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b)
{
    uint32_t sm_r = 0, sm_g = 0, sm_b = 0;
    if(w == 0)
    {
       sl_pwm_stop(&sl_pwm_w_led0);
    }
    else
    {
        my_pwm_set_duty_cycle_v1000(&sl_pwm_w_led0, w);
        sl_pwm_start(&sl_pwm_w_led0);
    }
    sm_r = (r * 65535) / 1000;
    sm_g = (g * 65535) / 1000;
    sm_b = (b * 65535) / 1000;
    sm15135e_fill_default(&my_led);
    sm15135e_set_rgbwy(&my_led, 
                       sm_r,      // Red (红)
                       sm_g,      // Green (绿)
                       sm_b,      // Blue (蓝)
                       0,      // White (冷白)
                       0       // Yellow/Warm (暖黄)
                      );
    sm15135e_send_frame(&my_led);
}
