/**
 * @file NvmStorage.h
 * @brief HAL：NVM3 非易失存储读写封装
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 使用 SDK 默认 NVM3 实例
 * nvm3_defaultHandle；业务层通过本类持久化灯光记忆等。
 */
#pragma once

#include "nvm3.h"
#include <stddef.h>
#include <stdint.h>

/** @brief NVM3 统一访问接口，供 LightEngine 等模块持久化 */
class NvmStorage
{
public:
  /** @brief 获取 NvmStorage 单例
   *  @return 单例引用 */
  static NvmStorage& instance();

  /**
   * @brief 读取指定 key 的数据对象
   * @param key  NVM3 对象 key
   * @param data 输出缓冲区
   * @param len  期望读取字节数
   * @return ECODE_NVM3_OK 成功，其它为错误码
   */
  Ecode_t readData(uint32_t key, void* data, size_t len);

  /**
   * @brief 写入指定 key 的数据对象
   * @param key  NVM3 对象 key
   * @param data 待写入数据
   * @param len  写入字节数
   * @return ECODE_NVM3_OK 成功，其它为错误码
   */
  Ecode_t writeData(uint32_t key, const void* data, size_t len);

  /**
   * @brief 查询对象类型与长度
   * @param key  NVM3 对象 key
   * @param type 输出对象类型
   * @param len  输出对象长度
   * @return ECODE_NVM3_OK 成功，其它为错误码
   */
  Ecode_t getObjectInfo(uint32_t key, uint32_t* type, size_t* len);

  /**
   * @brief 删除指定 key 的对象
   * @param key NVM3 对象 key
   * @return ECODE_NVM3_OK 成功，其它为错误码
   */
  Ecode_t deleteObject(uint32_t key);

  /**
   * @brief 必要时执行 NVM3 重整理（repack）以回收空间
   * @return true 已执行或无需重整理，false 重整理失败
   */
  bool repackIfNeeded();

private:
  NvmStorage() = default;
};
