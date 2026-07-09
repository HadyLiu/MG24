/**
 * @file PowerConfig.h
 * @brief 电池/充电阈值与状态枚举
 * @author hady
 * @date 2026-06-12
 * @layer App
 * @note 由 powerManage.h 迁入；Policy/Service 共享，不含 HAL 依赖。
 */
#pragma once

#include <stdint.h>

#define USBInOverCurt (0)     ///< USB 输入过流阈值(0=禁用)
#define NTCVlaueCtrl (0)      ///< NTC 控制模式占位
#define ChargeTimeMax (28800) ///< 最长充电时间(s)
#define FMTimeOut (100)       ///< 工厂模式超时(ms 或 tick)
/** @} */

/** @brief 电池/充电状态机枚举（Policy 与 HAL 共享） */
typedef enum
{
    Bat_DisChargeError = 20, ///< 放电异常
    Bat_LowVolProt     = 21, ///< 低电保护
    Bat_LowVolWarn     = 22, ///< 低电告警
    Bat_DisCharge      = 23, ///< 正常放电

    Bat_InputOverVol  = 30, ///< 输入过压
    Bat_InputOverCurt = 31, ///< 输入过流

    Bat_HighTemp    = 40, ///< 电池高温
    Bat_NonRecharge = 41, ///< 不可再充（保护锁存）
    Bat_ItError     = 42, ///< 充电 IC 故障

    Bat_ChargeFull = 50, ///< 充满
    Bat_Nobat      = 51, ///< 无电池
    Bat_ChargeInit = 52, ///< 充电初始化
    Bat_Detect     = 53, ///< 电池检测中
    Bat_InCharge   = 54, ///< 充电中
} TypedefBatEnum;
