/**
 * @file NvmStorage.h
 * @brief HAL：NVM3 非易失存储读写封装
 * @layer HAL
 */
#pragma once

#include "nvm3.h"
#include <stddef.h>
#include <stdint.h>

/** @brief NVM3 统一访问接口，供 LightEngine 等模块持久化 */
class NvmStorage
{
public:
    static NvmStorage &instance();

    Ecode_t readData(uint32_t key, void *data, size_t len);
    Ecode_t writeData(uint32_t key, const void *data, size_t len);
    Ecode_t getObjectInfo(uint32_t key, uint32_t *type, size_t *len);
    Ecode_t deleteObject(uint32_t key);
    bool    repackIfNeeded();

private:
    NvmStorage() = default;
};
