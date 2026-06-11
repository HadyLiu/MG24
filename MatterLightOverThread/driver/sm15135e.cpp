#include "sm15135e.h"
#include "cmsis_os2.h"
#include "sl_spidrv_instances.h"
#include "spidrv.h"
#include <string.h>

// #define SM15135E_SPI_SYMBOL_BITS 4u

// 💡 核心修改：升级为手册指定的 112 Bits
// #define SM15135E_FRAME_BITS   112u
// #define SM15135E_ENCODED_BITS (SM15135E_FRAME_BITS * SM15135E_SPI_SYMBOL_BITS)
// 🎯 (112 * 4 + 7) / 8 = 56 字节静态安全 DMA 缓冲区

//(SM15135E_FRAME_BITS >> 1)
//((SM15135E_ENCODED_BITS + 7u) / 8u) //+7 是为了向上取整到完整字节

#define SM15135E_RESET_BUF_BYTES 128u // 复位发送长度
#define SM15135E_ENCODED_BYTES   56   // 数据发送长度

#define sm15135e_mask5(v) (uint8_t)(v & 0x1Fu)

// spi 查表函数值
static const uint8_t SPI_PACK_LUT[2][2] = {
    // bl = 0, bl = 1
    {0x88, 0x8E}, // bh = 0
    {0xE8, 0xEE}, // bh = 1
};

// 改为查表
#define PACK_SPI_BYTE_PURE(bh, bl) (SPI_PACK_LUT[bh][bl])

typedef union
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
} sm15135e_bit_extractor_t;

// 静态安全缓冲区，规避栈释放导致的 DMA 硬件报错
static uint8_t sm15135e_reset_buf[SM15135E_RESET_BUF_BYTES] = {0};
static uint8_t sm15135e_frame_buf[SM15135E_ENCODED_BYTES];

// 控制器全局单例标志，确保 SPI/EUSART 只初始化一次
static bool sm15135e_spi_inited = false;

/**
 * @brief 初始化spi总线
 */
static void sm15135e_spi_init_once(void)
{
    if (sm15135e_spi_inited)
    {
        return;
    }
    sl_spidrv_init_instances();
    osDelay(10); // 等待 EUSART/SPI 硬件总线时钟稳定
    sm15135e_spi_inited = true;
}

/**
 * @brief 复位sm15135
 */
void sm15135e_send_reset(void)
{
    sm15135e_spi_init_once();
    // memset(sm15135e_reset_buf, 0, sizeof(sm15135e_reset_buf));
    //  🚀 通过 Silicon Labs LDMA 引擎异步发送 Reset 信号
    (void)SPIDRV_MTransmitB(sl_spidrv_eusart_rgb_data_handle, sm15135e_reset_buf, sizeof(sm15135e_reset_buf));
}

/**
 * @brief 初始化sm15135
 **/
void sm15135e_init(void)
{
    sm15135e_spi_init_once();
    // 额外挂起确保引脚处于稳定的 Idle 状态
    osDelay(2);
    sm15135e_send_reset();
}

/**
 * @brief
 */
void sm15135e_send_frame(const sm15135e_pixel_t *p)
{
    if (p == NULL)
    {
        return;
    }
    sm15135e_spi_init_once();

    size_t                   byte_idx = 0;
    sm15135e_bit_extractor_t ext;

    // 1. 灰度 5 通道打包
    uint16_t gray_channels[5] = {p->r, p->g, p->b, p->w, p->y};
    for (uint8_t i = 0; i < 5; ++i)
    {
        ext.val32 = (uint32_t)gray_channels[i];

        // 纯算术级并行压入，没有任何汇编跳转指令（无 BNE/BEQ），流水线跑满
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b15, ext.bits.b14);
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b13, ext.bits.b12);
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b11, ext.bits.b10);
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b9, ext.bits.b8);
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b7, ext.bits.b6);
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b5, ext.bits.b4);
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b3, ext.bits.b2);
        sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b1, ext.bits.b0);
    }

    // 2. 增益+模式复合 32位 打包
    uint32_t pack32 = 0;
    pack32 |= (uint32_t)(p->gain_r & 0x1Fu) << 27u;
    pack32 |= (uint32_t)(p->gain_g & 0x1Fu) << 22u;
    pack32 |= (uint32_t)(p->gain_b & 0x1Fu) << 17u;
    pack32 |= (uint32_t)(p->gain_w & 0x1Fu) << 12u;
    pack32 |= (uint32_t)(p->gain_y & 0x1Fu) << 7u;
    pack32 |= (uint32_t)(p->standby & 0x03u) << 5u;
    pack32 |= (uint32_t)(p->reserve & 0x1Fu);

    ext.val32 = pack32;

    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b31, ext.bits.b30);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b29, ext.bits.b28);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b27, ext.bits.b26);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b25, ext.bits.b24);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b23, ext.bits.b22);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b21, ext.bits.b20);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b19, ext.bits.b18);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b17, ext.bits.b16);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b15, ext.bits.b14);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b13, ext.bits.b12);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b11, ext.bits.b10);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b9, ext.bits.b8);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b7, ext.bits.b6);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b5, ext.bits.b4);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b3, ext.bits.b2);
    sm15135e_frame_buf[byte_idx++] = PACK_SPI_BYTE_PURE(ext.bits.b1, ext.bits.b0);

    // 初始值设为无效状态，确保第一次发送时能正确识别状态变化
    static uint8_t old_standby = 0xFFu;
    uint8_t        timeCounter = 1;
    if (old_standby != (p->standby))
    {
        if (old_standby == SM15135E_STANDBY_SLEEP)
        {
            timeCounter = 2;
        }
        old_standby = (p->standby);
    }
    // 🚀 纯内存到寄存器的算术映射，完工！
    // 状态唤醒时发送两遍，确保芯片完全醒来；正常状态下发送一遍即可
    do
    {
        (void)SPIDRV_MTransmitB(sl_spidrv_eusart_rgb_data_handle, sm15135e_frame_buf, sizeof(sm15135e_frame_buf));
        if (timeCounter > 1)
        {
            osDelay(2); // 状态切换时增加额外延时，确保芯片有足够时间从睡眠状态完全唤醒
        }
    } while (--timeCounter > 0);
}

/**
 * @brief 设定RGBWY的值
 */
void sm15135e_set_rgbwy(sm15135e_pixel_t *p, uint16_t r, uint16_t g, uint16_t b, uint16_t w, uint16_t y)
{
    if (p == NULL)
    {
        return;
    }
    p->r = r;
    p->g = g;
    p->b = b;
    p->w = w;
    p->y = y;
    if (r == 0 && g == 0 && b == 0 && w == 0 && y == 0)
    {
        p->standby = SM15135E_STANDBY_SLEEP;
    }
    else
    {
        p->standby = SM15135E_STANDBY_NORMAL;
    }
    p->reserve = 0x1Fu; // 手册强烈建议全填 1
}

/**
 * @brief 设定电流大小
 */
void sm15135e_set_all_gain(sm15135e_pixel_t *p, uint8_t gain)
{
    if (p == NULL)
    {
        return;
    }
    uint8_t g5 = sm15135e_mask5(gain);
    p->gain_r = g5;
    p->gain_g = g5;
    p->gain_b = g5;
    p->gain_w = g5;
    p->gain_y = g5;
}
