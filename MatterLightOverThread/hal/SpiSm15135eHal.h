/**
 * @file SpiRgbToSm15135eHal.h
 * @brief HAL：SM15135E SPI RGB 驱动初始化
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 封装 SM15135E 上电与 SPI 初始化时序，仅在系统启动阶段调用一次。
 */

#pragma once
#include <stdint.h>
#include <string.h>

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

class SpiSm15135eHal
{
public:
  /**
   * @brief 默认构造函数
   */
  SpiSm15135eHal();

  /**
   * @brief 初始化芯片与对应的 SPI/EUSART 硬件总线
   */
  void Init();

  /**
   * @brief 反初始化芯片
   */
  void Deinit();

  /**
   * @brief 触发 LDMA 引擎异步发送 Reset 信号复位芯片
   */
  void SendReset();

  /**
   * @brief 设置 5 通道灰度数据 (RGBWY)
   */
  void SetRgbwy(uint16_t r, uint16_t g, uint16_t b, uint16_t w, uint16_t y);

  /**
   * @brief 统一设置 5 通道的电流增益大小
   */
  void SetAllGain(CurrentGain gain);

  /**
   * @brief 核心打包与高效发送
   */
  void SendFrame();

  /**
   * @brief 查询模块是否完成初始化
   */
  bool IsInitialized() const
  {
    return is_initialized_;
  }

private:
  // --- 隐藏的前置声明，具体定义置于 .cpp 内部，避免污染头文件 ---
  struct PixelStorage;
  union BitExtractor;

  // --- 内部静态私有函数 (以 _ 开头) ---
  uint8_t _pack_spi_byte_pure(uint8_t bh, uint8_t bl) const;
  void _spi_init_once();
  void _clear_pixel_data();

private:
  // --- 成员变量（满足小写蛇形 + 单下划线结尾） ---
  PixelStorage* p_pixel_storage_; ///< 不透明指针或内部私有结构体实例（见.cpp）
  bool is_initialized_;
  uint8_t old_standby_;
};
