#pragma once

#include <platform/CHIPDeviceLayer.h>

// 声明配网事件注册函数（供 main.cpp 或 AppTask.cpp 调用）
void RegisterDeviceEventListener(void);

// 声明数据上报函数（供传感器读取任务调用）
void Upload_Temperature_Data(int16_t local_temp_hundredths);
