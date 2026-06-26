/**
 * @file MatterMailPacket.h
 * @brief Matter 中间层与服务层共享邮箱协议
 * @author hady
 * @date 2026-06-16
 * @layer Middleware
 * @note MatterBridge 投递 MatterMailMsg；MatterBridgeServer 消费并执行 Matter
 * 栈操作。
 */
#pragma once

#include <stdint.h>

enum class MatterExecuteElement : uint8_t
{
  kNone = 0x00U, /**< 无效 */
  kClearNetwork  /**< 上行接口清除配网 */
};

enum class MatterDataElement : uint8_t
{
  kNone = 0x00U,      /**< 无效 */
  kOn,                /**< 开关 */
  kBrightness,        /**< 亮度 */
  kHsv,               /**< 上行hsv */
  kCt,                /**< 上行色温 */
  kXy,                /**< 上行xy */
  kIdentify,          /**< 下行标识 */
  kCommissioningDone, /**< 下行配网完成 */
  kNetworkConnected   /**< 下行网络已连接 */
};

/** @brief 下行状态载荷 */
struct MatterDownlinkUploadPayload
{
  bool on;
  uint8_t brightness;
  MatterDataElement element;
  uint8_t reserved; ///< 字节对齐预留

  union
  {
    struct
    {
      uint8_t hue;
      uint8_t saturation;
    } hsv;

    struct
    {
      uint16_t colorTemperature;
    } ct;

    struct
    {
      uint16_t x;
      uint16_t y;
    } xy;
  } color;
};

/** @brief 下行提示载荷 */
enum class MatterDownlinkHintPayload : uint8_t
{
  kIdentify = 0x00U,  /**< 标识 */
  kCommissioningDone, /**< 配网完成 */
  kNetworkConnected   /**< 网络已连接 */
};
