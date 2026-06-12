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

#define BatDisChargeLowWarnVal (2300)
#define BatDisChargeLowVal (2000)

#define PowerInLimitVol (3000)
#define PowerInOvervoltagesLimitCount (10)
#define BatLimitI (6700)
#define NTCDetectBat (3092)
#define NTCLowValTemp (2026)
#define NTCOverValTemp (591)
#define NTCRecoverValTemp (660)
#define NTCBatUsedOverValTemp (1349)
#define USBInOverCurt (0)
#define NTCVlaueCtrl (0)
#define ChargeTimeMax (28800)
#define FMTimeOut (100)

typedef enum
{
  Bat_DisChargeError = 20,
  Bat_LowVolProt     = 21,
  Bat_LowVolWarn     = 22,
  Bat_DisCharge      = 23,

  Bat_InputOverVol  = 30,
  Bat_InputOverCurt = 31,

  Bat_HighTemp    = 40,
  Bat_NonRecharge = 41,
  Bat_ItError     = 42,

  Bat_ChargeFull = 50,
  Bat_Nobat      = 51,
  Bat_ChargeInit = 52,
  Bat_Detect     = 53,
  Bat_InCharge   = 54,

} TypedefBatEnum;
