/**
 * @file HalSpiSm15135e.cpp
 * @brief HAL：SM15135E SPI RGB 驱动实现
 * @author hady
 * @date 2026-06-12
 * @layer HAL
 * @note 仅在系统启动阶段调用一次 Init；帧缓冲与 Reset 缓冲为编译单元静态存储。
 */
#include "HalSpiSm15135e.h"

#include "cmsis_os2.h"
#include "sl_spidrv_instances.h"
#include "spidrv.h"

// --- 1. 常量定义（全面剥离 #define，采用 constexpr 类型安全） ---
static constexpr uint16_t ResetBufBytes = 128u;
static constexpr uint16_t EncodedBytes  = 56u;

// --- 2. 静态常驻安全缓冲区（移出头文件，防止栈释放导致 LDMA 报错） ---
static uint8_t s_reset_buf[ResetBufBytes] = {0};
static uint8_t s_frame_buf[EncodedBytes]  = {0};

// --- 3. 厂商底层的私有结构体与位域映射的具体定义 ---
struct HalSpiSm15135e::PixelStorage
{
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t w;
  uint16_t y;
  uint8_t gain_r;
  uint8_t gain_g;
  uint8_t gain_b;
  uint8_t gain_w;
  uint8_t gain_y;
  uint8_t standby;
  uint8_t reserve;
};

union HalSpiSm15135e::BitExtractor
{
  uint32_t val32;
  struct
  {
    unsigned int b0 : 1;
    unsigned int b1 : 1;
    unsigned int b2 : 1;
    unsigned int b3 : 1;
    unsigned int b4 : 1;
    unsigned int b5 : 1;
    unsigned int b6 : 1;
    unsigned int b7 : 1;
    unsigned int b8 : 1;
    unsigned int b9 : 1;
    unsigned int b10 : 1;
    unsigned int b11 : 1;
    unsigned int b12 : 1;
    unsigned int b13 : 1;
    unsigned int b14 : 1;
    unsigned int b15 : 1;
    unsigned int b16 : 1;
    unsigned int b17 : 1;
    unsigned int b18 : 1;
    unsigned int b19 : 1;
    unsigned int b20 : 1;
    unsigned int b21 : 1;
    unsigned int b22 : 1;
    unsigned int b23 : 1;
    unsigned int b24 : 1;
    unsigned int b25 : 1;
    unsigned int b26 : 1;
    unsigned int b27 : 1;
    unsigned int b28 : 1;
    unsigned int b29 : 1;
    unsigned int b30 : 1;
    unsigned int b31 : 1;
  } bits;
};

// --- 4. 核心成员函数实现 ---

/**
 * @brief 默认构造函数
 * @return 无
 * @note 绑定静态 PixelStorage 并清零像素数据
 */
HalSpiSm15135e::HalSpiSm15135e() : old_standby_(0xFFu)
{
  // 资源静态化：使用常驻在编译单元内的结构体，严禁动态分配 new
  static PixelStorage s_instance_storage;
  p_pixel_storage_ = &s_instance_storage;
  ClearPixelDataRaw();
}

/**
 * @brief 初始化芯片与对应的 SPI/EUSART 硬件总线
 * @return 无
 * @note 延迟后发送 Reset 帧
 */
void HalSpiSm15135e::Init()
{
  SpiInitOnceRaw();
  osDelay(2); // 挂起确保引脚处于稳定的 Idle 状态
  SendReset();
}

/**
 * @brief 反初始化芯片
 * @return 无
 * @note 清零 RGBWY 并发送一帧
 */
void HalSpiSm15135e::DeInit()
{
  SetRgbwyDuty(0, 0, 0, 0, 0);
  SendFrame();
}

/**
 * @brief 触发 LDMA 引擎异步发送 Reset 信号复位芯片
 * @return 无
 * @note 发送 128 字节全零缓冲
 */
void HalSpiSm15135e::SendReset()
{
  SpiInitOnceRaw();
  memset(s_reset_buf, 0, sizeof(s_reset_buf));
  // 🚀 通过 Silicon Labs LDMA 引擎异步发送 Reset 信号
  (void)SPIDRV_MTransmitB(sl_spidrv_eusart_rgb_data_handle, s_reset_buf,
                          sizeof(s_reset_buf));
}

/**
 * @brief 设置 5 通道灰度数据 (RGBWY)
 * @param r 红色灰度
 * @param g 绿色灰度
 * @param b 蓝色灰度
 * @param w 白色灰度
 * @param y 黄色灰度
 * @return 无
 * @note 全零时进入 Sleep；reserve 固定填 0x1F
 */
void HalSpiSm15135e::SetRgbwyDuty(uint16_t r, uint16_t g, uint16_t b,
                                  uint16_t w, uint16_t y)
{
  p_pixel_storage_->r = r;
  p_pixel_storage_->g = g;
  p_pixel_storage_->b = b;
  p_pixel_storage_->w = w;
  p_pixel_storage_->y = y;

  if (r == 0 && g == 0 && b == 0 && w == 0 && y == 0)
  {
    p_pixel_storage_->standby = static_cast<uint8_t>(StandbyMode::Sleep);
  }
  else
  {
    p_pixel_storage_->standby = static_cast<uint8_t>(StandbyMode::Normal);
  }
  p_pixel_storage_->reserve = 0x1Fu; // 手册强烈建议全填 1
}

/**
 * @brief 统一设置 5 通道的电流增益大小
 * @param gain 电流增益枚举
 * @return 无
 * @note 五通道使用相同 gain_code（低 5 位）
 */
void HalSpiSm15135e::SetAllGain(HalSpiSm15135e::CurrentGain gain)
{
  uint8_t gain_code        = static_cast<uint8_t>(gain) & 0x1Fu;
  p_pixel_storage_->gain_r = gain_code;
  p_pixel_storage_->gain_g = gain_code;
  p_pixel_storage_->gain_b = gain_code;
  p_pixel_storage_->gain_w = gain_code;
  p_pixel_storage_->gain_y = gain_code;
}

/**
 * @brief 核心打包与高效发送
 * @return 无
 * @note standby 切换时自动发送两遍以确保唤醒；使用位域提取器与 LUT 打包
 */
void HalSpiSm15135e::SendFrame()
{
  SpiInitOnceRaw();

  size_t byte_idx = 0;
  BitExtractor extractor;

  // 1. 灰度 5 通道算术级并行打包压入（继承原厂无跳转、跑满流水线的高性能设计）
  uint16_t gray_channels[5] = {p_pixel_storage_->r, p_pixel_storage_->g,
                               p_pixel_storage_->b, p_pixel_storage_->w,
                               p_pixel_storage_->y};

  for (uint8_t i = 0; i < 5; ++i)
  {
    extractor.val32 = static_cast<uint32_t>(gray_channels[i]);

    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b15, extractor.bits.b14);
    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b13, extractor.bits.b12);
    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b11, extractor.bits.b10);
    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b9, extractor.bits.b8);
    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b7, extractor.bits.b6);
    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b5, extractor.bits.b4);
    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b3, extractor.bits.b2);
    s_frame_buf[byte_idx++] =
        PackSpiBytePureRaw(extractor.bits.b1, extractor.bits.b0);
  }

  // 2. 增益 + 模式复合 32 位数据打包
  uint32_t pack32 = 0;
  pack32 |= static_cast<uint32_t>(p_pixel_storage_->gain_r & 0x1Fu) << 27u;
  pack32 |= static_cast<uint32_t>(p_pixel_storage_->gain_g & 0x1Fu) << 22u;
  pack32 |= static_cast<uint32_t>(p_pixel_storage_->gain_b & 0x1Fu) << 17u;
  pack32 |= static_cast<uint32_t>(p_pixel_storage_->gain_w & 0x1Fu) << 12u;
  pack32 |= static_cast<uint32_t>(p_pixel_storage_->gain_y & 0x1Fu) << 7u;
  pack32 |= static_cast<uint32_t>(p_pixel_storage_->standby & 0x03u) << 5u;
  pack32 |= static_cast<uint32_t>(p_pixel_storage_->reserve & 0x1Fu);

  extractor.val32 = pack32;

  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b31, extractor.bits.b30);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b29, extractor.bits.b28);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b27, extractor.bits.b26);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b25, extractor.bits.b24);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b23, extractor.bits.b22);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b21, extractor.bits.b20);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b19, extractor.bits.b18);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b17, extractor.bits.b16);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b15, extractor.bits.b14);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b13, extractor.bits.b12);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b11, extractor.bits.b10);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b9, extractor.bits.b8);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b7, extractor.bits.b6);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b5, extractor.bits.b4);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b3, extractor.bits.b2);
  s_frame_buf[byte_idx++] =
      PackSpiBytePureRaw(extractor.bits.b1, extractor.bits.b0);

  uint8_t time_counter = 1;
  if (old_standby_ != p_pixel_storage_->standby)
  {
    if (old_standby_ ==
        static_cast<uint8_t>(HalSpiSm15135e::StandbyMode::Sleep))
    {
      time_counter = 2; // 唤醒时发送两遍，确保芯片完全醒来
    }
    old_standby_ = p_pixel_storage_->standby;
  }

  do
  {
    (void)SPIDRV_MTransmitB(sl_spidrv_eusart_rgb_data_handle, s_frame_buf,
                            sizeof(s_frame_buf));
    if (time_counter > 1)
    {
      osDelay(2); // 状态切换时增加额外延时
    }
  } while (--time_counter > 0);
}

/**
 * @brief 将两位 SPI 编码查表打包为单字节
 * @param bh 高位比特
 * @param bl 低位比特
 * @return 编码后的 SPI 字节
 * @note 使用编译单元内静态 2x2 LUT
 */
uint8_t HalSpiSm15135e::PackSpiBytePureRaw(uint8_t bh, uint8_t bl) const
{
  // 完美的物理封装：原 C 语言全局查表 LUT 收拢为本编译单元内的静态局部常量
  static const uint8_t SpiPackLut[2][2] = {{0x88, 0x8E}, {0xE8, 0xEE}};
  return SpiPackLut[bh][bl];
}

/**
 * @brief 一次性初始化 SPI 驱动实例
 * @return 无
 * @note 调用 sl_spidrv_init_instances 后延迟 10 ms
 */
void HalSpiSm15135e::SpiInitOnceRaw()
{
  static bool s_spiInitialized = false;
  if (s_spiInitialized)
  {
    return;
  }

  sl_spidrv_init_instances();
  osDelay(10);
  s_spiInitialized = true;
}

/**
 * @brief 清零像素与增益缓存
 * @return 无
 * @note p_pixel_storage_ 非空时 memset 整个 PixelStorage
 */
void HalSpiSm15135e::ClearPixelDataRaw()
{
  if (p_pixel_storage_ != nullptr)
  {
    memset(p_pixel_storage_, 0, sizeof(PixelStorage));
  }
}
