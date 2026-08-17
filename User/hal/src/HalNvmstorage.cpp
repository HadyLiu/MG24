/**
 * @file NvmStorage.cpp
 * @brief HAL：NVM3 读写实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 使用 SDK 默认实例 nvm3_defaultHandle。
 */
#include "HalNvmstorage.h"

/**
 * @brief 获取 HalNvmstorage 单例
 * @return 单例引用
 * @note 函数内静态实例
 */
HalNvmstorage& HalNvmstorage::Instance() {
  static HalNvmstorage s_storage;
  return s_storage;
}

/**
 * @brief 读取 NVM3 对象
 * @param key  NVM3 对象 key
 * @param data 输出缓冲区
 * @param len  期望读取字节数
 * @return ECODE_NVM3_OK 成功，其它为错误码
 * @note 委托 nvm3_readData
 */
Ecode_t HalNvmstorage::ReadData(uint32_t key, void* data, size_t len) {
  return nvm3_readData(nvm3_defaultHandle, key, data, len);
}

/**
 * @brief 写入 NVM3 对象
 * @param key  NVM3 对象 key
 * @param data 待写入数据
 * @param len  写入字节数
 * @return ECODE_NVM3_OK 成功，其它为错误码
 * @note 委托 nvm3_writeData
 */
Ecode_t HalNvmstorage::WriteData(uint32_t key, const void* data, size_t len) {
  return nvm3_writeData(nvm3_defaultHandle, key, data, len);
}

/**
 * @brief 查询 NVM3 对象元信息
 * @param key  NVM3 对象 key
 * @param type 输出对象类型
 * @param len  输出对象长度
 * @return ECODE_NVM3_OK 成功，其它为错误码
 * @note 委托 nvm3_getObjectInfo
 */
Ecode_t HalNvmstorage::GetObjectInfo(uint32_t key, uint32_t* type, size_t* len) {
  return nvm3_getObjectInfo(nvm3_defaultHandle, key, type, len);
}

/**
 * @brief 删除 NVM3 对象
 * @param key NVM3 对象 key
 * @return ECODE_NVM3_OK 成功，其它为错误码
 * @note 委托 nvm3_deleteObject
 */
Ecode_t HalNvmstorage::DeleteObject(uint32_t key) {
  return nvm3_deleteObject(nvm3_defaultHandle, key);
}

/**
 * @brief 必要时执行 NVM3 repack
 * @return true 已执行或无需重整理，false 重整理失败
 * @note 仅在 nvm3_repackNeeded 为真时调用 repack
 */
bool HalNvmstorage::RepackIfNeeded() {
  if (nvm3_repackNeeded(nvm3_defaultHandle)) {
    return nvm3_repack(nvm3_defaultHandle) == ECODE_NVM3_OK;
  }
  return true;
}
