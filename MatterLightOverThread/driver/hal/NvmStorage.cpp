/**
 * @file NvmStorage.cpp
 * @brief HAL：NVM3 读写实现
 * @layer HAL
 */
#include "NvmStorage.h"

/** @brief 获取 NvmStorage 单例 */
NvmStorage &NvmStorage::instance()
{
    static NvmStorage s_storage;
    return s_storage;
}

/** @brief 读取 NVM3 对象 */
Ecode_t NvmStorage::readData(uint32_t key, void *data, size_t len)
{
    return nvm3_readData(nvm3_defaultHandle, key, data, len);
}

/** @brief 写入 NVM3 对象 */
Ecode_t NvmStorage::writeData(uint32_t key, const void *data, size_t len)
{
    return nvm3_writeData(nvm3_defaultHandle, key, data, len);
}

/** @brief 查询 NVM3 对象元信息 */
Ecode_t NvmStorage::getObjectInfo(uint32_t key, uint32_t *type, size_t *len)
{
    return nvm3_getObjectInfo(nvm3_defaultHandle, key, type, len);
}

/** @brief 删除 NVM3 对象 */
Ecode_t NvmStorage::deleteObject(uint32_t key)
{
    return nvm3_deleteObject(nvm3_defaultHandle, key);
}

/** @brief 必要时执行 NVM3 repack */
bool NvmStorage::repackIfNeeded()
{
    if (nvm3_repackNeeded(nvm3_defaultHandle))
    {
        return nvm3_repack(nvm3_defaultHandle) == ECODE_NVM3_OK;
    }
    return true;
}
