/**
 * @file LightDecisionTypes.h
 * @brief 灯光决策层共享类型（无业务逻辑、无跨组件依赖）
 * @author hady
 * @date 2026-06-19
 * @layer Service
 * @note ButtonService / LightDecisionCenter / MatterBridgeServer 共用本头文件，
 *       避免 Service 之间互相 #include 实现类。
 */
#pragma once

#include <stdint.h>

/**
 * @brief 灯光场景状态机
 * @note LightDecisionCenter 内部仲裁；非 Normal 时忽略 Matter 常规下行。
 */
enum class LightSceneState : uint8_t
{
    Normal = 0,        /**< 常规用户/Matter 控制 */
    NetConfiguring,    /**< 配网混合时序进行中 */
    MatterIdentifying, /**< Matter 识别指示时序进行中 */
    LowBattery         /**< 临界电量强控（禁止常亮；可播开灯演示时序） */
};

/**
 * @brief 按键语义事件
 * @note 由 ButtonService 从 ButtonMailMsg 翻译，经 entry 投递至
 * LightDecisionCenter。
 */
enum class KeyEventType : uint8_t
{
    ShortPressCycleBrightness = 0, /**< 短按灯键：亮度循环；未入网时兼触发/刷新配网 */
    DoublePressCycleColor,         /**< 双击：开灯态色库循环（关灯忽略） */
    ShortPressOpenCommissioning,   /**< 短按系统键：未入网时打开/刷新配网窗 */
    LongPressClearNetLighting,     /**< 长按约 8s：主灯/指示灯进入重置预警时序 */
    LongPressClearNet,             /**< 长按约 13s：武装复位（时序完结且已武装才擦除） */
    LongPressStopNet               /**< 松开未满 13s：取消预警（≥13s 松开不取消，等时序完结复位） */
};

/**
 * @brief 配网控制动作
 * @note 由 LightDecisionCenter 发出，entry 翻译为 MatterBridge 具体操作。
 */
enum class NetControlAction : uint8_t
{
    FactoryReset = 0,     /**< 工厂重置：擦除后重启，再进入可配网态 */
    OpenCommissioning,    /**< 打开/刷新基础配网窗（未入网） */
    Close                 /**< 关闭配网窗（预留） */
};

/**
 * @brief 电池电压等级（Service 层抽象，由 PowerServer 从 BSP 映射）
 */
enum class BatteryVoltLevel : uint8_t
{
    Normal = 0,   /**< 电压正常 */
    LowWarning,   /**< 低电量提示（<7.0V，仍可运行） */
    CriticalEmpty /**< 临界电量（<6.5V，强控灭灯） */
};

/**
 * @brief USB 充电状态（Service 层抽象，由 PowerServer 从 BSP 映射）
 */
enum class UsbChargeState : uint8_t
{
    Idle = 0, /**< 未充电 / 初始 */
    Charging, /**< 充电中 */
    Done,     /**< 充满 */
    Fault     /**< 充电故障 */
};

/**
 * @brief 充电综合电池状态（数值越小优先级越高）
 * @note PowerServer 按优先级表仲裁后输出，供指示灯策略扩展。
 */
enum class BatteryChargeStatus : uint8_t
{
    Nobat         = 0, /**< 无电池 */
    ChargeFault   = 1, /**< 充电芯片故障  */
    TempFault     = 2, /**< 电池温度异常 */
    CriticalEmpty = 3, /**< 临界电量，停充保护 */
    Charging      = 4, /**< 充电中（快慢充由 useFastCharge 区分） */
    ChargeDone    = 5, /**< 已充满 */
    LowWarning    = 6, /**< 低电量提示（USB 在位且仍可充电） */
    Idle          = 7  /**< USB 在位但未进入有效充电 */
};

/**
 * @brief 充电指示灯灯效（位标志，白呼吸与红闪可叠加）
 */
enum class ChargeIndicatorEffect : uint8_t
{
    Off                    = 0,                     /**< 熄灭 */
    WhiteBreath            = 1U << 0,               /**< 白灯呼吸：CalcBreath80BytesFactor */
    RedBlink               = 1U << 1,               /**< 红灯闪烁：GetBlink */
    WhiteBreathAndRedBlink = WhiteBreath | RedBlink /**< 充电白呼吸 + 故障红闪 */
};

/**
 * @brief 充电综合状态快照（供 PowerServer 上报、IndicatorServer 仲裁）
 */
struct BatteryChargeSnapshot
{
    BatteryChargeStatus status;        /**< 仲裁后的电池/充电状态 */
    bool                useFastCharge; /**< true=快充，false=慢充 */
    bool                chipValid;     /**< 充电芯片可读 */
    bool                chipCharging;  /**< 芯片报告正在充电（可与故障红闪叠加） */
};

/**
 * @brief 渐变算子业务 ID
 * @note 持久化与 Matter 下行共用；下标映射 LightDecisionCenter::kActionTable。
 */
enum class LightEffectOpId : uint8_t
{
    DirectKeep = 0,  /**< GetKeep */
    LinearLerp,      /**< GetLerp */
    Breath80Bytes,   /**< CalcBreath80BytesFactor */
    Bezier80FadeIn,  /**< GetBezier80BytesFactorFadeIn */
    Bezier40FadeIn,  /**< GetBezier40BytesFactorFadeIn */
    Bezier80FadeOut, /**< GetBezier80BytesFactorFadeOut */
    Bezier40FadeOut, /**< GetBezier40BytesFactorFadeOut */
    Blink,           /**< GetBlink */
    MaxOperators     /**< 算子总数上界（非有效 ID） */
};
