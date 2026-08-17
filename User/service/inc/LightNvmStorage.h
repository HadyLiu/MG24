/**
 * @file LightNvmStorage.h
 * @brief 灯光参数 NVM 持久化适配器
 * @author hady
 * @date 2026-06-19
 * @layer Service
 * @note 实现 LightStorageProvider，委托 HalNvmstorage。
 */
#pragma once

#include "LightDecisionCenter.h"

/**
 * @brief 灯光决策参数 NVM 读写适配
 */
class LightNvmStorage : public LightStorageProvider {
 public:
  bool Read(uint8_t* pDest, uint16_t size) override;
  bool Write(const uint8_t* pSrc, uint16_t size) override;

 private:
  static constexpr uint32_t kLightParamNvmKey = 0xA001U;
};
