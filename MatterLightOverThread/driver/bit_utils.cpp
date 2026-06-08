#include "bit_utils.h"

uint8_t ReverseBits8(uint8_t b)
{
    // 经典的分治算法：4位对调 -> 2位对调 -> 1位对调
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

uint16_t FixBitEndian16(uint16_t raw_val)
{
    // 1. 拆出高低字节
    uint8_t low_byte = (uint8_t)(raw_val & 0x00FF);
    uint8_t high_byte = (uint8_t)((raw_val & 0xFF00) >> 8);

    // 2. 分别对高低字节内部的 Bit 进行物理镜像反转 (LSB <-> MSB)
    uint8_t fixed_low = ReverseBits8(low_byte);
    uint8_t fixed_high = ReverseBits8(high_byte);

    // 3. 🎯 终极拼接平衡（如果字节序大端小端也反了，直接在这里把 fixed_low 和 fixed_high 调换）
    return (uint16_t)((fixed_high << 8) | fixed_low);
}