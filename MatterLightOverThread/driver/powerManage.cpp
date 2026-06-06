#include "powerManage.h"

/********************
 * power status
 ******************/
bool eg_PowerStatus, eg_UpPowerStatus;
bool eg_TriggerDetection;
bool eg_ChargingChip_TriggerDetection;

bool g_ChargeState; // 充电状态
/********************
 * battery status
 ******************/
TypedefBatEnum eg_BatStatus;

/********************
 * charge time
 ******************/
unsigned int g_ChargeTimeSec;

/********************
 * ADC value
 ******************/
unsigned int g_ADBatLowVal, g_ADBatHighVal;
// unsigned int g_ADMcuLowVal, g_ADMcuHighVal;
unsigned int g_ADPowerInVal;
unsigned int g_ADCurrentVal;
unsigned int g_ADTemperature;

/****************************************
 * Multiple times the AD value
 **************************************/
unsigned int g_ADBatLowMultipleVal;
// unsigned int g_ADMcuLowMultipleVal;

/**
 * 外部电源累计值
 */
unsigned char g_PowerInOvervoltagesCount;

typedef enum
{
    Error,
    Incharge,
    ChargeFull,
} FM4258_Enum;

FM4258_Enum g_ChargingChip_Status;

unsigned char g_FMTimeout;
unsigned char g_FMCout;

/****************************************
 * Multiple times the AD value
 **************************************/
#ifdef USE_BATTERY_RESISTANCE
#ifdef DEBUGMODE
unsigned int g_BatResVol;
#endif
unsigned int  g_BatResMultipleVal;
unsigned char g_BatResState;
#endif

// 实例化 IADC 驱动对象，绑定到具体的 GPIO 引脚（例如 PA08）
IadcDriver adcA8(gpioPortA, 8); // PA08
IadcDriver adcC4(gpioPortC, 4); // PC04
IadcDriver adcC5(gpioPortC, 5); // PC05

/**
 * @brief 📬 初始化 ADC 模块，准备好后续的电压检测功能
 */
void powerManage_adc_Init(void)
{
    // adcA8.DeInit(gpioModeInput,0);
    adcC4.Init(); // 初始化 IADC 驱动
    adcC5.Init(); // 初始化 IADC 驱动
}

/**
 * @brief 📬 获取外部电源输入的 ADC 电压值
 */
uint16_t get_powerin_adc_vol(void)
{
    uint16_t powerInVol;
    adcA8.Init(); // 确保 ADC 已经初始化
    powerInVol = adcA8.ReadVoltageMilliVolts();
    adcA8.DeInit(gpioModeInput, 0);
    return powerInVol;
}

/**
 * @brief 📬 获取电池电压的 ADC 电压值
 */
uint16_t get_bat_adc_vol(void)
{ return adcC4.ReadVoltageMilliVolts(); }

/**
 * @brief 📬 获取温度传感器的 ADC 电压值
 */
uint16_t get_temp_adc_vol(void)
{ return adcC5.ReadVoltageMilliVolts(); }

/****************************************
 * Init function
 **************************************/
/**
 * @brief 📬 电源管理初始化函数，设置初始状态和参数
 */
void PowerManageInit(void)
{
    eg_TriggerDetection = false; /* 开启一次检测 */
    g_ChargeTimeSec = 0;
    g_ADBatLowMultipleVal = ((BatDisChargeLowWarnVal + 100) << 3);
    g_FMTimeout = FMTimeOut;
    g_FMCout = 0;
    g_ChargingChip_Status = Incharge;
    eg_ChargingChip_TriggerDetection = false;
    BoostDis();
    ChargePwmOff();
    ChargeSwitchOff();
}

/**
 * @brief 📬 电源切换函数，根据当前电源状态进行相应的初始化和状态更新
 */
void PowerSwitchAssignment(void)
{
    if (eg_PowerStatus != eg_UpPowerStatus)
    {
        eg_PowerStatus = eg_UpPowerStatus;
        if (eg_PowerStatus == true) // 外部供电状态
        {
            eg_BatStatus = Bat_ChargeInit;
        }
        else // 电池供电状态
        {
            eg_BatStatus = Bat_DisCharge;
        }
        PowerManageInit(); // 电源管理初始化
        // IndicInit();       // 指示灯初始化
        // led初始化
        // eg_ledData.CloseProtection = false;
        // eg_ledData.HistoryProtection = false;
        // eg_ledData.Protection = false;
    }
}

/**
 * @brief 📬 获取放电时电池状态
 */
void GetDisChargeStatus(void)
{
    do
    {
/* 检测电池是否异常 */
#if (false)
        ChargeSwitchOn();
        SetAdcTempChannel();
        g_ADTemperature = GetAdcValue();
        ChargeSwitchOff();
        if (g_ADTemperature < NTCDetectBat)
        {
            eg_BatStatus = Bat_DisChargeError;
            break;
        }
#endif

#ifdef USE_MCU_LOW_BATTERY
        SetAdcMcuChannel();
        delay_ms(1);
        g_ADMcuLowVal = MoveAverage(&g_ADMcuLowMultipleVal, GetAdcValue());

        if ((eg_BatStatus == Bat_LowVolProt) || (g_ADMcuLowVal > BatDisChargeLowVal))
        {
            eg_BatStatus = Bat_LowVolProt;
            break;
        }
        if ((eg_BatStatus == Bat_LowVolWarn) || (g_ADMcuLowVal > BatDisChargeLowWarnVal))
        {
            eg_BatStatus = Bat_LowVolWarn;
            break;
        }
        eg_BatStatus = Bat_DisCharge;
        break;
#else
        // SetAdcBatChannel();
        g_ADBatLowVal = get_bat_adc_vol();

        if ((eg_BatStatus == Bat_LowVolProt) || (g_ADBatLowVal < BatDisChargeLowVal))
        {
            eg_BatStatus = Bat_LowVolProt;
            break;
        }
        if ((eg_BatStatus == Bat_LowVolWarn) || (g_ADBatLowVal < BatDisChargeLowWarnVal))
        {
            eg_BatStatus = Bat_LowVolWarn;
            break;
        }
        eg_BatStatus = Bat_DisCharge;
        break;

#endif
    } while (0);
}

/*-------------------- charge Time Updata --------------------------*/
/**
 * @brief 📬 充电时间更新函数，10ms调用一次，用于更新充电时间计数器
 */
void ChargeTimeUpdata(void)
{
    static unsigned char timeCount = 0;
    if (++timeCount > 100)
    {
        timeCount = 0;
        if (g_ChargeTimeSec <= ChargeTimeMax && eg_BatStatus >= Bat_Nobat)
        {
            g_ChargeTimeSec++;
        }
    }
}

/* -------------------- 电池内阻检测 --------------------------- */
#ifdef USE_BATTERY_RESISTANCE

void CalcBatRes(void)
{
    unsigned long BatteryResistance;
    if (g_ADCurrentVal > 3)
    {
        // BatteryResistance = g_ADMcuHighVal;
        BatteryResistance *= g_ADBatHighVal;
        BatteryResistance -= g_ADBatLowVal;
        BatteryResistance *= 25;
        BatteryResistance /= g_ADCurrentVal;
        BatteryResistance /= 768;
    }
    else
    {
        BatteryResistance = 0xFFFF;
    }
    if (BatteryResistance <= (RechargeBatR + (RechargeBatR >> 1))) /* 移动平均 */
    {
        BatteryResistance = MoveAverage(&g_BatResMultipleVal, (unsigned int)(0x3FF & BatteryResistance));
    }
    else
    {
        g_BatResMultipleVal = ((RechargeBatR >> 1) << 3);
    }
#ifdef DEBUGMODE
    g_BatResVol = BatteryResistance;
    if (BatteryResistance >= 0xFFFF)
    {
        g_BatResVol = 0xFFFF;
    }
#endif
    if (BatteryResistance > RechargeBatR)
    {
        g_BatResState = false;
    }
    g_BatResState = true;
}
#endif

/**
 * @brief 📬 获取充电芯片状态，判断是否处于充电中或充满状态
 * @return FM4258_Enum 充电芯片状态枚举值
 */
FM4258_Enum FM4258_GetStatus(void)
{
    /* 以 10 ms 周期调用 */
    const uint32_t samplePeriodMs = 10;
    /* 认为 1 Hz 脉冲的边沿间隔应在 0.8–1.2 s 之间 */
    const uint32_t pulseMinMs = 800;
    const uint32_t pulseMaxMs = 1200;

    static uint8_t  lastLevel = 0;   // 上次检测到的电平状态
    static uint32_t msSinceEdge = 0; // 距离上次检测到边沿的时间
    static uint8_t  edgeCount = 0;   // 连续边沿计数

    uint8_t level = FM4258_ReadStatusPin();
    msSinceEdge += samplePeriodMs;

    if (level != lastLevel)
    {
        /* 检测到一个电平跳变，检查跳变间隔是否符合 1 Hz 闪烁 */
        if (msSinceEdge >= pulseMinMs && msSinceEdge <= pulseMaxMs)
        {
            edgeCount++;
        }
        else
        {
            /* 间隔异常，重置脉冲计数 */
            edgeCount = 1;
        }
        msSinceEdge = 0;
        lastLevel = level;
    }

    /* 如果已经检测到连续的 1 Hz 跳变，认为处于错误闪烁状态 */
    if (edgeCount >= 2)
    {
        edgeCount = 2; // 防止计数溢出
        return Error;
    }

    /* 电平长期稳定，重置脉冲计数 */
    if (msSinceEdge > pulseMaxMs * 2)
    {
        edgeCount = 0;
    }

    if (level == 0)
    {
        return Incharge;
    }
    else
    {
        return ChargeFull;
    }
}

bool ChargeDetect(void)
{
    // 错开检测
    bool           status = false;
    static uint8_t IntervalTimer = 0;
    static uint8_t TestingSteps = 0;
    // 400ms检测一次
    if (++IntervalTimer >= 40)
    {
        IntervalTimer = 0;
        if (TestingSteps >= 2)
        {
            TestingSteps = 1;
        }
    }
    if (eg_BatStatus >= Bat_Nobat)
    {
        // 错开检测步骤，第一次检测电池温度，第二次检测USB电源输入电压
        if (TestingSteps == 0)
        {
            BoostEn();
            ChargePwmOff();
            ChargeSwitchOn();
            sl_udelay_wait(30);
            g_ADTemperature = get_temp_adc_vol();
            TestingSteps++;
        }
        else if (TestingSteps == 1)
        {
            //   ChargeSwitchOff();
            sl_udelay_wait(30);
            g_ADPowerInVal = get_powerin_adc_vol();
            TestingSteps = 0;
            status = true;
            TestingSteps++;
        }

        // /* Turn on charging ----------------------- */
        // BoostEn();
        // ChargePwmOff();
        // ChargeSwitchOn();

        // power in
        // SetPowerInAnalog();
        // SetAdcPowerInChannel();
        //  g_ADPowerInVal = get_powerin_adc_vol();
        // ClearPowerInAnalog();

        // Temperature
        // SetAdcTempChannel();
        //  g_ADTemperature = get_temp_adc_vol();

        // #ifdef USE_BATTERY_RESISTANCE
        //         CalcBatRes();
        // #endif
    }
    else
    {
        status = true;
    }
    // 充电芯片状态检测
    g_ChargingChip_Status = FM4258_GetStatus();
    return status;
}

/**
 * @brief 📬 充电逻辑函数，根据当前电池状态和充电芯片状态进行相应的充电控制
 * @param ReceptionStatus 接收状态，表示是否接收到新的充电状态信息
 */
void ChargeLogic(bool ReceptionStatus)
{
    while (ReceptionStatus)
    {
#if (true)
        if (eg_BatStatus <= Bat_InputOverCurt)
        {
            break;
        }
        if (g_ADPowerInVal > PowerInLimitVol)
        {
            eg_BatStatus = Bat_InputOverVol;
            break;
        }
        // if (g_ADCurrentVal > BatLimitI)
        // {
        //     eg_BatStatus = Bat_InputOverCurt;
        //     break;
        // }
#endif

        if (g_ADTemperature > NTCDetectBat)
        {
            g_ChargeTimeSec = 0;
            eg_BatStatus = Bat_Nobat;
            break;
        }

        if (g_ChargeTimeSec < 4)
        {
            eg_BatStatus = Bat_Detect;
            break;
        }
#if (false)
        if (g_ChargeTimeSec < 8)
        {
            if (false)
            {
                eg_BatStatus = Bat_NonRecharge;
                break;
            }
            if (g_ADBatLowVal > BatChargeFullLowVal)
            {
                eg_BatStatus = Bat_ChargeFull;
                break;
            }
            eg_BatStatus = Bat_InCharge;
            break;
        }
#endif
#if (true)
        if (eg_BatStatus < Bat_ChargeInit)
        {
            break;
        }
        // if (g_ADTemperature < NTCOverValTemp)
        // {
        //     eg_BatStatus = Bat_HighTemp;
        //     break;
        // }
        // if (false)
        // {
        //     eg_BatStatus = Bat_NonRecharge;
        //     break;
        // }

        if (g_ChargingChip_Status == ChargeFull || (g_ChargeTimeSec > ChargeTimeMax))
        {
            eg_BatStatus = Bat_ChargeFull;
            break;
        }
        if (g_ChargingChip_Status == Error)
        {
            eg_BatStatus = Bat_ItError;
            break;
        }
        eg_BatStatus = Bat_InCharge;
        break;
#endif
    }
}

void ChargeCurrentCtrlOut(unsigned char Status)
{
    if (eg_BatStatus == Bat_Nobat || eg_BatStatus == Bat_ChargeInit || eg_BatStatus == Bat_Detect || eg_BatStatus == Bat_InCharge)
    {
        if (g_ChargeState == false)
        {
            g_ChargeState = true;
            BoostEn();
            ChargePwmOff();
            ChargeSwitchOn();
        }

        if (Status == false)
        {
            ChargeRateEn();
        }
        else
        {
            ChargeRateDis();
        }
    }
    else
    {
        if (g_ChargeState == true)
        {
            g_ChargeState = false;
            BoostDis();
            ChargePwmOff();
            ChargeSwitchOff();
        }
    }
}
