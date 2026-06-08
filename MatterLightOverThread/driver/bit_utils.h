#pragma once
#include <stdint.h>

/**
 * @brief 对一个 8 位字节进行底层的 Bit 左右镜像翻转 (LSB <-> MSB)
 * @param b 输入的原始单字节
 * @return 翻转后的字节
 */
uint8_t ReverseBits8(uint8_t b);

/**
 * @brief 纠正由于 Bit 端（大小端）反掉导致的 16 位整数错误
 * @param raw_val 从 Matter 属性中拿到的原始 uint16_t 数据
 * @return 修正 Bit 序后的真实 uint16_t 坐标
 */
uint16_t FixBitEndian16(uint16_t raw_val);
