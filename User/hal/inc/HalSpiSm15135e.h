/**
 * @file HalSpiSm15135e.h
 * @brief HAL：SM15135E SPI RGB 驱动
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 封装 SM15135E 上电与 SPI 初始化时序，仅在系统启动阶段调用一次 Init。
 */

#pragma once

#include <cstdint>
#include <cstring>

/**
 * @brief 强类型枚举类：芯片待机/工作模式，指定底层为 uint8_t 极致节省内存
 */
enum class StandbyMode : uint8_t
{
    Normal = 0x00u,
    Sleep  = 0x02u
};

/**
 * @brief 强类型枚举类：5路电流增益定义
 */
enum class CurrentGain : uint8_t
{
    Gain_10_2MA  = 0x00u,
    Gain_20_3MA  = 0x01u,
    Gain_30_4MA  = 0x02u,
    Gain_40_5MA  = 0x03u,
    Gain_50_6MA  = 0x04u,
    Gain_60_7MA  = 0x05u,
    Gain_70_8MA  = 0x06u,
    Gain_80_9MA  = 0x07u,
    Gain_91_0MA  = 0x08u,
    Gain_101_1MA = 0x09u,
    Gain_111_2MA = 0x0Au,
    Gain_121_3MA = 0x0Bu,
    Gain_130_7MA = 0x0Cu,
    Gain_140_6MA = 0x0Du,
    Gain_150_5MA = 0x0Eu,
    Gain_160_2MA = 0x0Fu,
    Gain_170_0MA = 0x10u,
    Gain_179_0MA = 0x11u,
    Gain_188_5MA = 0x12u,
    Gain_198_0MA = 0x13u
};

/** @brief SM15135E SPI RGBWY 驱动 HAL */
class HalSpiSm15135e
{
  public:
    /* 强类型枚举类：芯片待机/工作模式，指定底层为 uint8_t 极致节省内存*/
    enum class StandbyMode : uint8_t
    {
        Normal = 0x00u,
        Sleep  = 0x02u
    };

    /* 强类型枚举类：5路电流增益定义*/
    enum class CurrentGain : uint8_t
    {
        Gain_10_2MA  = 0x00u,
        Gain_20_3MA  = 0x01u,
        Gain_30_4MA  = 0x02u,
        Gain_40_5MA  = 0x03u,
        Gain_50_6MA  = 0x04u,
        Gain_60_7MA  = 0x05u,
        Gain_70_8MA  = 0x06u,
        Gain_80_9MA  = 0x07u,
        Gain_91_0MA  = 0x08u,
        Gain_101_1MA = 0x09u,
        Gain_111_2MA = 0x0Au,
        Gain_121_3MA = 0x0Bu,
        Gain_130_7MA = 0x0Cu,
        Gain_140_6MA = 0x0Du,
        Gain_150_5MA = 0x0Eu,
        Gain_160_2MA = 0x0Fu,
        Gain_170_0MA = 0x10u,
        Gain_179_0MA = 0x11u,
        Gain_188_5MA = 0x12u,
        Gain_198_0MA = 0x13u
    };

  public:
    /* 默认构造函数 */
    HalSpiSm15135e();

    /* 初始化芯片与对应的 SPI/EUSART 硬件总线 */
    void Init();

    /* 反初始化芯片 */
    void DeInit();

    /* 触发 LDMA 引擎异步发送 Reset 信号复位芯片 */
    void SendReset();

    /* 设置 5 通道灰度数据 (RGBWY) */
    void SetRgbwyDuty(uint16_t r, uint16_t g, uint16_t b, uint16_t w, uint16_t y);

    /* 统一设置 5 通道的电流增益大小   */
    void SetAllGain(CurrentGain gain);

    /* 核心打包与高效发送 */
    void SendFrame();

    /* 获取 SM15135E 最大输出 PWM 分辨率位数 */
    uint8_t GetMaxPwmBits() const
    {
        return kMaxPwmBits;
    }
    uint32_t GetMaxPwmValue() const
    {
        return kMaxPwmValue;
    }

  private:
    // --- 隐藏的前置声明，具体定义置于 .cpp 内部，避免污染头文件 ---
    struct PixelStorage;
    union BitExtractor;

    // --- 成员变量（满足小写蛇形 + 单下划线结尾） ---
    PixelStorage* p_pixel_storage_; ///< 不透明指针或内部私有结构体实例（见.cpp）
    uint8_t       old_standby_;

    ///< SM15135E 最大输出PWM
    static constexpr uint8_t  kMaxPwmBits  = 16U;
    static constexpr uint32_t kMaxPwmValue = ((1U << kMaxPwmBits) - 1);

    /* 将两位 SPI 编码查表打包为单字节 */
    uint8_t PackSpiBytePureRaw(uint8_t bh, uint8_t bl) const;

    /* 一次性初始化 SPI 驱动实例 */
    void SpiInitOnceRaw();

    /* 清零像素与增益缓存*/
    void ClearPixelDataRaw();
};
