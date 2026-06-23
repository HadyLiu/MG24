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
  LowBattery         /**< 极低电量强控（禁止亮灯） */
};

/**
 * @brief 按键语义事件
 * @note 由 ButtonService 从 ButtonMailMsg 翻译，经 entry 投递至
 * LightDecisionCenter。
 */
enum class KeyEventType : uint8_t
{
  ShortPressCycleBrightness = 0, /**< 短按：亮度 255→128→0 循环 */
  DoublePressCycleColor,         /**< 双击：WRGB 调色板循环 */
  LongPressClearNetLighting,     /**< 长按开始：配网灯效开启 */
  LongPressClearNet,             /**< 长按开始：清除配网 + 配网灯效 */
  LongPressStopNet               /**< 长按松开：停止配网 + 恢复默认 */
};

/**
 * @brief 配网控制动作
 * @note 由 LightDecisionCenter 发出，entry 翻译为 MatterBridge 具体操作。
 */
enum class NetControlAction : uint8_t
{
  ClearAndOpen = 0, /**< 软复位网络并打开配网窗 */
  Close             /**< 关闭配网窗 */
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
