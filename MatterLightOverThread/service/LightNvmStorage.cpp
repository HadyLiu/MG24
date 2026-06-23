/**
 * @file LightNvmStorage.cpp
 * @brief 灯光参数 NVM 持久化实现
 * @author hady
 * @date 2026-06-19
 * @layer Service
 */
#include "LightNvmStorage.h"
#include "HalNvmstorage.h"

/**
 * @brief 从 NVM3 读取灯光参数
 * @param pDest 输出缓冲区
 * @param size  读取字节数
 * @return true 成功
 */
bool LightNvmStorage::Read(uint8_t* pDest, uint16_t size)
{
  if (pDest == nullptr)
  {
    return false;
  }

  return HalNvmstorage::Instance().ReadData(kLightParamNvmKey, pDest,
                                            static_cast<size_t>(size)) ==
         ECODE_NVM3_OK;
}

/**
 * @brief 写入灯光参数至 NVM3
 * @param pSrc 源数据
 * @param size 写入字节数
 * @return true 成功
 */
bool LightNvmStorage::Write(const uint8_t* pSrc, uint16_t size)
{
  if (pSrc == nullptr)
  {
    return false;
  }

  const Ecode_t code = HalNvmstorage::Instance().WriteData(
      kLightParamNvmKey, pSrc, static_cast<size_t>(size));
  if (code != ECODE_NVM3_OK)
  {
    return false;
  }

  (void)HalNvmstorage::Instance().RepackIfNeeded();
  return true;
}
