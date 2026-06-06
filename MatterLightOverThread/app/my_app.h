
#pragma once

#include "../driver/button.h"
#include "../driver/led_pwm_port.h"
#include "../driver/led_driver.h"
#include "../driver/pinManage.h"
#include "../driver/powerManage.h"
#include "../driver/iadc_driver.h"
#include "../driver/indicLed.h"
#include "../driver/sm15135e.h"

// 你的函数声明
void my_custom_init_app_process(void);

void my_custom_loop_app_process(void);

/**
 * @brief 📬 配网成功动画：当设备成功配对到网络后调用，执行一段特定的灯效动画来提示用户
 */
void TriggerPairingSuccessAnimation(void);
