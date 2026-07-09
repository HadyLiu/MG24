/**
 * @file LightMessage.h
 * @brief 灯光系统统一消息契约
 */

#ifndef LIGHT_MESSAGE_H
#define LIGHT_MESSAGE_H

#include "LightDecisionTypes.h" /* 包含先前定义的 LightEffectOpId 枚举 */
#include <stdint.h>

/**
 * @brief 异构输入事件源枚举
 */
enum class LightEvtSrc : uint8_t
{
    Key_Click = 0,        /**< 本地按键短按（Toggle 触发） */
    Key_LongClearNet,     /**< 本地按键长按：清除配网 */
    Key_LongStopNet,      /**< 本地按键长按：停止配网 */
    Matter_Cmd_Control,   /**< Matter 网络：下发常规调光/颜色控制 */
    Battery_Low_Critical, /**< 电池硬中断：低电量强控制（最高优先级） */
    Battery_Normal_Rx     /**< 电池状态恢复恢复 */
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
/**
 * @brief 消息队列传输实体结构体
 * @note 严格遵循 32 位字节对齐，规避跨线程内存碎片与交叉编译对齐隐患。
 */
typedef struct
{
    LightEvtSrc     src;                   /**< 标记消息由谁发出，1 字节 */
    uint8_t         brightness;            /**< 目标全局亮度 (0~255)，1 字节 */
    LightEffectOpId op_id;                 /**< 算子业务枚举 ID，1 字节 */
    uint8_t         reserved1;             /**< 显式边界对齐填充，1 字节 */
    uint16_t        wrgb[4];               /**< 逻辑色彩数组 (W,R,G,B)，8 字节 */
} __attribute__((packed)) LightQueueMsg_T; /* 总体大小固定为 12 字节 */

#endif /* LIGHT_MESSAGE_H */