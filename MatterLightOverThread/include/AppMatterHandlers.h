#pragma once

#include <platform/CHIPDeviceLayer.h>

// 声明配网事件注册函数（供 main.cpp 或 AppTask.cpp 调用）
void RegisterDeviceEventListener(void);

void TriggerNetworkResetWithoutReboot(void);

// 用于抑制“本地上报属性 -> 回调再次触发控制”的回环
bool IsMatterReportBypassEnabled(void);

// 本地状态上报到 Matter 属性库（内部会切换到 Matter 线程执行）
void Upload_Matter_OnOff(bool is_on);
void Upload_Matter_Brightness(uint8_t driver_brightness_percent);

bool IsMatterUnprovisioned(void);
void CommissioningFirstBreath_Stop(void);
