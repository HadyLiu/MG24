/**
 * @file MatterMailPacket.h
 * @brief Matter 中间层与服务层共享邮箱协议
 * @author hady
 * @date 2026-06-16
 * @layer Middleware
 * @note MatterBridge 投递 MatterMailMsg；MatterBridgeServer 消费并执行 Matter 栈操作。
 */
#pragma once

#include <stdint.h>

/** @brief 上行属性上报 */
static constexpr uint8_t kMatterCategoryUpload = 0x01U;
/** @brief 配网 / 复位 / 事件注册等控制 */
static constexpr uint8_t kMatterCategoryControl = 0x02U;
/** @brief Matter 下行到本地灯光（暂 stub，待 LightEngine 接入） */
static constexpr uint8_t kMatterCategoryDownlink = 0x03U;

/**
 * @brief Matter 邮箱命令码
 */
struct MatterMailCmd
{
  static constexpr uint8_t kUploadOnOff           = 0x01U;
  static constexpr uint8_t kUploadBrightness      = 0x02U;
  static constexpr uint8_t kUploadColorCt         = 0x03U;
  static constexpr uint8_t kUploadColorHsv        = 0x04U;
  static constexpr uint8_t kOpenCommissioning     = 0x10U;
  static constexpr uint8_t kCloseCommissioning      = 0x11U;
  static constexpr uint8_t kSoftNetworkReset        = 0x12U;
  static constexpr uint8_t kRegisterDeviceEvents    = 0x20U;
  static constexpr uint8_t kDownlinkAction          = 0x30U;
  static constexpr uint8_t kDownlinkColor           = 0x31U;
};

/**
 * @brief Matter 邮箱载荷（静态队列元素，无动态分配）
 */
struct alignas(1) MatterMailMsg
{
  uint8_t category; ///< kMatterCategory*
  uint8_t cmd;      ///< MatterMailCmd::k*
  uint8_t byte0;
  uint8_t byte1;
  uint16_t word0;
  uint16_t word1;
};
