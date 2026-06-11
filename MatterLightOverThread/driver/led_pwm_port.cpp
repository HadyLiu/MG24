#include "led_pwm_port.h"
#include <stdint.h>

extern sl_pwm_instance_t sl_pwm_w_led0;

/**
 * @brief 按照千分比(0~1000)设置PWM占空比
 * @param pwm PWM实例指针
 * @param duty_val 占空比数值，范围 0 到 1023（512代表50%）
 */
void my_pwm_set_duty_cycle_10bit_resolution(sl_pwm_instance_t *pwm, uint16_t duty_val)
{
    if (duty_val >= 1023)
    {
        duty_val = 1023; // 限幅防越界
    }

#if defined(_SILICON_LABS_32B_SERIES_2)
    // 适用于 Series 2 芯片 (如 EFR32MG24)
    uint32_t top = TIMER_TopGet(pwm->timer);
    TIMER_CompareBufSet(pwm->timer, pwm->channel, (top * duty_val) >> 10);
#else
    // 适用于 Series 3 芯片
    uint32_t top = sl_hal_timer_get_top(pwm->timer);
    // 右移10位相当于除以1024
    sl_hal_timer_channel_set_compare_buffer(pwm->timer, pwm->channel, (top * duty_val) >> 10);
#endif
}

/**
 * @brief 获取当前的千分比占空比
 * @return 返回 0 ~ 1023 的数值
 */
uint16_t my_pwm_get_duty_cycle_10bit_resolution(sl_pwm_instance_t *pwm)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
    uint32_t top = TIMER_TopGet(pwm->timer);
    uint32_t compare = TIMER_CaptureGet(pwm->timer, pwm->channel);
#else
    uint32_t top = sl_hal_timer_get_top(pwm->timer);
    uint32_t compare = sl_hal_timer_channel_get_compare(pwm->timer, pwm->channel);
#endif
    if (top == 0)
    {
        return 0;
    }
    return (uint16_t)((compare * 1023) / top);
}

void LED_W_SetDuty(uint16_t w)
{
    if (w == 0)
    {
        sl_pwm_stop(&sl_pwm_w_led0);
    }
    else
    {
        my_pwm_set_duty_cycle_10bit_resolution(&sl_pwm_w_led0, w);
        sl_pwm_start(&sl_pwm_w_led0);
    }
}

void Indic_SetDuty(uint16_t w_led)
{
    if (w_led == 0)
    {
        sl_pwm_stop(&sl_pwm_Indic_led0);
    }
    else
    {
        my_pwm_set_duty_cycle_10bit_resolution(&sl_pwm_Indic_led0, w_led);
        sl_pwm_start(&sl_pwm_Indic_led0);
    }
}

sm15135e_pixel_t my_led;

/**
 * @brief 将 WRGB 值应用到硬件输出
 * @param w 白光亮度 0~1023
 * @param r 红光亮度 0~1023
 * @param g 绿光亮度 0~1023
 * @param b 蓝光亮度 0~1023
 */
void LED_HW_SetWRGB(uint16_t w, uint16_t r, uint16_t g, uint16_t b)
{
    uint32_t    sm_r = 0, sm_g = 0, sm_b = 0;
    static bool standby = false;

    LED_W_SetDuty(w); // 直接控制白光通道的 PWM 占空比，达到独立调节白光亮度的目的

    // 将10位精度转化为16位精度
    sm_r = r << 6;
    sm_g = g << 6;
    sm_b = b << 6;

    if (r >= 0x3FF)
    {
        sm_r = 0xFFFF;
    }
    if (g >= 0x3FF)
    {
        sm_g = 0xFFFF;
    }
    if (b >= 0x3FF)
    {
        sm_b = 0xFFFF;
    }

    // 输出为0时 芯片进入休眠模式，节省功耗
    sm15135e_set_all_gain(&my_led, SM15135E_GAIN_101_1MA); // 全通道设置为最大电流 198mA，确保亮度输出足够
    sm15135e_set_rgbwy(&my_led,
                       sm_b, // Red (红)
                       sm_g, // Green (绿)
                       sm_r, // Blue (蓝)
                       0,    // White (冷白)
                       0     // Yellow/Warm (暖黄)
    );

    sm15135e_send_frame(&my_led);
}
