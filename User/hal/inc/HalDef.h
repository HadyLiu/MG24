/**
 * @file hal_def.h
 * @brief HAL 层公共定义
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note
 */
#pragma once

enum class HalStateEnum : uint8_t
{
  HAL_OK            = 0x00u, /* 操作成功 */
  HAL_ERROR         = 0x01u, /* 操作发生错误 */
  HAL_BUSY          = 0x02u, /* 操作正在进行中 */
  HAL_TIMEOUT       = 0x03u, /* 操作超时 */
  HAL_CRC_ERROR     = 0x04u, /* CRC（循环冗余校验）错误 */
  HAL_DMA_ERROR     = 0x05u, /* DMA（直接内存访问）错误 */
  HAL_INVALID_PARAM = 0x06u
};
