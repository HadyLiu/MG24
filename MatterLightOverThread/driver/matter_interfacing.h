#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 初始化 Matter 属性拦截与同步服务
 * （如果需要做一些全局变量初始化，可以在 AppTask::AppInit 中调用）
 */
void Matter_Interface_Init(void);

/**
 * @brief 状态主动上传接口 API (设备本地状态改变后，调用此函数同步给手机 App)
 * @note  当你通过本地物理按键改变了开关、亮度时，调用此函数能让手机端滑条实时同步
 */
void Matter_UploadLocalStateToPhone(void);
