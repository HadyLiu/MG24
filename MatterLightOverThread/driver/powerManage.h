#pragma once

#include "pinManage.h"
#include "iadc_driver.h"
#include <stdint.h>
#include <stdbool.h>
#include "sl_udelay.h"

/*---------------------------- IO DEFINE ----------------------------*/
#define GetExternPowerStatus() GET_POWER_IN_DETECT()

#define BatOutEn()  BAT_EN_ON()
#define BatOutDis() BAT_EN_OFF()

#define BoostDis()        CHARGE_EN_OFF()
#define BoostEn()         CHARGE_EN_ON()
#define ChargePwmOff()    (void(0))
#define ChargePwmOn()     (void(0))
#define ChargeSwitchOn()  (void(0))
#define ChargeSwitchOff() (void(0))
#define ChargeRateEn()    CHARGE_SPEED_FAST()
#define ChargeRateDis()   CHARGE_SPEED_SLOW()

#define FM4258_ReadStatusPin() CHARGE_STATUS_READ()

// #define SetPowerInAnalog() (ANSEL1 |= 0x01)
// #define ClearPowerInAnalog() (ANSEL1 &= ~0x01)
//
// #define SetAdcBatChannel() (ADCON0bits.CHS = 0, delay_ms(1))
// #define SetAdcPowerInChannel() (ADCON0bits.CHS = 8, delay_ms(1))
// #define SetAdcTempChannel() (ADCON0bits.CHS = 14, delay_ms(1))

#define ChargeGetStatus() CHARGE_STATUS_READ() //(RA6)

/*------------- BATTERY CONFIG ---------*/
// #define USE_MCU_LOW_BATTERY /* 利用MCU的AD值来代替电池电压 */
// #define USE_BATTERY_RESISTANCE /* 使用电池阻值来判断电池类型 */

/***************** define  ********************/
#define BatDisChargeLowWarnVal (2233) /* y = 6.7 *(1/ 3) = 2233      */
#define BatDisChargeLowVal     (2000) /* y = 6.0 *(1/ 3) = 2000       */

///***   充电设置配置  8190*3/5860=  ***/
// #define BatHighWarnVol (89364230) /* ((4095*4095*(4.55+0.187)*9)/(2.00*4)) = 89364230   提示保护      */
// #define BatFullHighVol (87447715) /* ((4095*4095*(4.45+0.187)*9)/(2.00*4)) = 87447715  电池电压高于  */
// #define BatFullLowVol (83704684)  /* ((4095*2*(4.25+0.187)*9)/(2.00*4)) = 83704684  电池电压低于  */
// #define BatMiddleVol (2900)       /* g= (1024*((6.0+4.0）/2 -0.317+0.02-0.15)/(2*4))*3 = 2017          (计算)    中间点电池电压 */

// 保护
#define PowerInLimitVol               (3000) /*    (6/2)*1000=3000      // (4095*(6/2)/3.3)=3722      (采集)    USB输入过压    */
#define PowerInOvervoltagesLimitCount (10)   /*             */
#define BatLimitI                     (6700) /*     NG      (4095*(3.6/2)/3.3)*3=7445      (采集)    USB输入过压    */
#define NTCDetectBat                  (3092) /*  > 29.884K  (299/(299+20))*3.3*1000 = 3092          NTC检测是否存在电池*/
#define NTCLowValTemp                 (2026) /* > 31.819K   (31.819/(31.819+20))*3.3*1000 = 2026   NTC温度过低 */
#define NTCOverValTemp                (591)  /* < 4.365K    (4.365/(4.365+20))*3.3*1000 = 591       NTC温度过高 */
#define NTCBatUsedOverValTemp         (1349) /* < 4.365K    (4095*2.468/(2.468+20))*3=1349           NTC温度过高 */
#define USBInOverCurt                 (0)    /* 5952   2000/(0.28*1.2)  1.2A   (采集)    USB输入瞬间过流 */
#define NTCVlaueCtrl                  (0)    /* 36+39.497    <1.32V (采集)     NTC温度过高 */

//// 电池类型判断
// #define BatDiffVol (150)   /*  差值过大为无电池   */
// #define NonBatR (2000)     /* 0xFFFFF                        (采集)    大于该值为无电池 */
// #define RechargeBatR (394) /* 294+(544−294)/100⋅40                            (采集)    大于为干电池      */
// #define BatEffTime (60)    /* 60S                             干电池判断检测有效时间 */

// 充电时间控制
#define ChargeTimeMax (28800) /* 3600*8 = 28800                         充电限制时间      */

/********** FM  ****************** */
#define FMTimeOut 100

//// 充电电流控制
// #define HighCurrentCtrl (368550) /* 0.200*1000*0.3*4095*3/2=368550  充电平均电流控制  */
// #define LowCurrentCtrl (368550)  /* 0.200*1000*0.3*4095*3/2=368550  充电平均电流控制  */
//
//// 充电PWM
// #define PwmChargeOffset (0)                                                 /* 0                PWM充电偏移量        */
// #define PwmChargeMin (50 - PwmChargeOffset)                                 /* 100 - 0               充电占空比最小值   */
// #define PwmChargeMax (950 - PwmChargeOffset)                                /* 900 - 0               充电占空比最大值   */
// #define PwmChargeMulit (3)                                                  /* 倍率 */
// #define PwmChargeMinMulit ((PwmChargeMin << PwmChargeMulit) - PwmChargeMin) /* Pwm最低pwm值 */

typedef enum
{
    /* Battery output  */
    Bat_DisChargeError = 20, /* discharge Error  */
    Bat_LowVolProt = 21,     /* low protection */
    Bat_LowVolWarn = 22,     /* low warn */
    Bat_DisCharge = 23,      /* discharge */

    /* power input error protection */
    Bat_InputOverVol = 30,  /* Input overvoltage  6V */
    Bat_InputOverCurt = 31, /* Input overcurrent  1.2A */

    /* bat error protection */
    Bat_HighTemp = 40,    /* battery temperature */
    Bat_NonRecharge = 41, /* Non-rechargeable batteries */
    Bat_ItError = 42,     /* it error */

    /* normal  */
    Bat_ChargeFull = 50, /* charge full */
    Bat_Nobat = 51,      /* No battery */
    Bat_ChargeInit = 52, /* Init */
    Bat_Detect = 53,     /* detetct */
    Bat_InCharge = 54,   /* Incharge  */

} TypedefBatEnum;

/*** */
extern bool           eg_PowerStatus, eg_UpPowerStatus;
extern bool           eg_TriggerDetection;
extern bool           eg_ChargingChip_TriggerDetection;
extern TypedefBatEnum eg_BatStatus; /* 电池状态 */

#define GetExternPowerFlag()              (eg_UpPowerStatus = GetExternPowerStatus())
#define SetTriggerDetection()             (eg_TriggerDetection = true)
#define SetTriggerDetectionChargingChip() (eg_ChargingChip_TriggerDetection = true)

void powerManage_adc_Init(void);
void powerManage_adc_DeInit(void);
bool DetectTemp(void);            // 检测温度
void PowerManageInit(void);       // 初始化
void PowerSwitchAssignment(void); // 电源切换
void ChargeTimeUpdata(void);      // 充电时间更新
void GetDisChargeStatus(void);    // 获取放电时电池状态
// void ChargeFMStatus(void);                      // 充电状态检测
// void USBDetect(void);                           //
bool ChargeDetect(void);                        // 充电检测
void ChargeLogic(bool ReceptionStatus);         // 充电逻辑
void ChargeCurrentCtrlOut(unsigned char value); // 充电控制逻辑输出
void ChargeLedOut(void);                        // 充电指示灯逻辑输出
void PilotInit(void);
// void HL_Interrupt_ChargeIndicatorLed(void);
