#include "swimHost.h"
/**
 * @file    SwimHost.cpp
 * @brief   基于 EFR32MG24 的自定义 SWIM 协议主机端数据包发送与接收实现
 * @note    适用于 Gecko SDK v4.x，需确保芯片主频配置正确以保证微秒延时准确
 */

#include "em_gpio.h"
#include "em_cmu.h"
#include "sl_udelay.h"

// ============================================================================
// 1. 硬件配置与宏定义
// ============================================================================
// 假设使用 EFR32MG24 的 PA05 引脚作为单线通信引脚
#define SWIM_PORT gpioPortA
#define SWIM_PIN  5

// 自定义数据包通信指令（省略24位物理地址，直接进行整包操作）
#define CMD_PACKET_WRITE 0x01 // 主机发起写数据包通知
#define CMD_PACKET_READ  0x02 // 主机发起读数据包通知

// 快速引脚控制宏
// EFR32在 gpioModeWiredAnd（开漏）模式下：写1释放总线（靠外部上拉变高），写0强行拉低
#define SWIM_LOW()  GPIO_PinOutClear(SWIM_PORT, SWIM_PIN)
#define SWIM_HIGH() GPIO_PinOutSet(SWIM_PORT, SWIM_PIN)
#define SWIM_READ() GPIO_PinInGet(SWIM_PORT, SWIM_PIN)

// ============================================================================
// 2. 底层驱动层 (私有内部函数)
// ============================================================================

/**
 * @brief  SWIM 发送单个位 (Bit)
 * @note   标准时序：0 表现为长低短高(~1.6us低, ~0.4us高)；1 表现为短低长高(~0.4us低, ~1.6us高)
 */
static void SWIM_SendBit(uint8_t bit)
{
    if (bit == 0)
    {
        SWIM_LOW();
        sl_udelay_wait(1); // 基础微秒延时，78.4MHz下配合硬件开销逼近 1.6us
        // 如下方微调需要，可取消注释并调整循环次数
        // for(volatile int i = 0; i < 10; i++);
        SWIM_HIGH();
        sl_udelay_wait(1);
    }
    else
    {
        SWIM_LOW();
        // 0.4us 极短，在高速 M33 内核下几个汇编空指令(NOP)即可精准控制
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        SWIM_HIGH();
        sl_udelay_wait(2); // 保持高电平以满足单个bit的总周期 (~2.0us)
    }
}

/**
 * @brief  SWIM 读取单个位 (Bit)
 * @return 采样到的电平值 (0 或 1)
 */
static uint8_t SWIM_ReadBit(void)
{
    uint8_t bit = 1;

    // 主机触发读契机：拉低总线约 0.4us
    SWIM_LOW();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // 主机释放总线，变为输入监听状态
    SWIM_HIGH();

    // 等待从机响应，并在当前 Bit 周期中后段 (~1.2us处) 进行电平采样
    sl_udelay_wait(1);
    if (SWIM_READ() == 0)
    {
        bit = 0;
    }

    // 等待本个 Bit 剩余周期结束，恢复总线空闲
    sl_udelay_wait(1);
    return bit;
}

/**
 * @brief  SWIM 发送单字节 (包含：1位起始0 + 8位数据 + 1位偶校验 + 接收方ACK)
 */
static uint8_t SWIM_SendByte(uint8_t byte)
{
    uint8_t parity = 0;

    // 1. 发送起始位 (固定的逻辑 0)
    SWIM_SendBit(0);

    // 2. 发送 8 位数据 (最低位 LSB 先行)
    for (int i = 0; i < 8; i++)
    {
        uint8_t bit = (byte >> i) & 0x01;
        SWIM_SendBit(bit);
        parity ^= bit; // 累加计算偶校验
    }

    // 3. 发送计算出的偶校验位
    SWIM_SendBit(parity);

    // 4. 释放总线并监听从机 B 的应答 (ACK表现为从机强行拉低总线)
    SWIM_HIGH();
    sl_udelay_wait(1);

    uint32_t timeout = 1000;
    while (SWIM_READ() == 1)
    {
        timeout--;
        if (timeout == 0)
            return 1; // 错误：从机未应答超时
    }

    // 等待从机释放总线，重新变回高电平空闲
    timeout = 1000;
    while (SWIM_READ() == 0)
    {
        timeout--;
        if (timeout == 0)
            return 1; // 错误：总线死锁超时
    }

    return 0; // 成功收到应答
}

/**
 * @brief  SWIM 读取单字节 (包含：1位起始0 + 8位数据 + 1位偶校验 + 主机发送ACK)
 */
static uint8_t SWIM_ReadByte(uint8_t *p_byte)
{
    uint8_t data = 0;
    uint8_t parity = 0;
    uint8_t bit = 0;

    // 1. 检测从机输出的起始位
    if (SWIM_ReadBit() != 0)
    {
        return 1; // 帧错误：未检测到合法的起始位
    }

    // 2. 连续读取 8 位数据 (LSB 先行)
    for (int i = 0; i < 8; i++)
    {
        bit = SWIM_ReadBit();
        data |= (bit << i);
        parity ^= bit;
    }

    // 3. 读取从机附带的偶校验位
    uint8_t received_parity = SWIM_ReadBit();

    // 4. 校验比对
    if (parity != received_parity)
    {
        return 1; // 校验失败：数据在传输中受损
    }

    // 5. 校验无误后，主机向从机发送 ACK (逻辑 0) 通知其继续
    SWIM_SendBit(0);

    *p_byte = data;
    return 0;
}

// ============================================================================
// 3. 上层业务应用接口 (对外开放)
// ============================================================================

/**
 * @brief  初始化 EFR32 对应的 SWIM 硬件引脚
 */
void SWIM_Host_Init(void)
{
    // 使能 GPIO 时钟树
    CMU_ClockEnable(cmuClock_GPIO, true);

    // 将引脚配置为开漏输出带输入模式 (Wired-AND with filter)
    // 默认输出 1 处于高阻态，完全依靠外部 1K 欧姆上拉电阻拉高电平
    GPIO_PinModeSet(SWIM_PORT, SWIM_PIN, gpioModeWiredAnd, 1);
}

/**
 * @brief  发送上电复位/唤醒序列，强制令从机 B 进入 SWIM 通信模式
 */
void SWIM_Host_SendResetSequence(void)
{
    SWIM_HIGH();
    sl_udelay_wait(10000); // 稳定总线 10ms

    // 向总线连续发送 4 个宽脉冲（通常为 1kHz 左右，即低 1ms，高 1ms）
    for (int i = 0; i < 4; i++)
    {
        SWIM_LOW();
        sl_udelay_wait(1000);
        SWIM_HIGH();
        sl_udelay_wait(1000);
    }

    SWIM_HIGH();
    sl_udelay_wait(20000); // 等待从机内部重置准备就绪状态 20ms
}

/**
 * @brief  发送完整原始数据包
 * @param  p_data: 要发出的数据数组指针
 * @param  len: 数据长度
 * @return 0: 成功, 1: 失败
 */
uint8_t SWIM_Host_SendPacket(uint8_t *p_data, uint8_t len)
{
    if (p_data == nullptr || len == 0)
        return 1;

    // 发送写包前导指令
    if (SWIM_SendByte(CMD_PACKET_WRITE) != 0)
        return 1;

    // 依次将整包数据逐个字节发出
    for (uint8_t i = 0; i < len; i++)
    {
        if (SWIM_SendByte(p_data[i]) != 0)
        {
            return 1;
        }
        // 给从机留出少量微秒级的平摊处理时间（如压入从机缓冲区）
        sl_udelay_wait(30);
    }
    return 0;
}

/**
 * @brief  读取完整原始数据包
 * @param  p_buf: 存放接收数据的数组指针
 * @param  len: 期望接收的长度
 * @return 0: 成功, 1: 失败
 */
uint8_t SWIM_Host_ReadPacket(uint8_t *p_buf, uint8_t len)
{
    if (p_buf == nullptr || len == 0)
        return 1;

    // 发送读包前导指令
    if (SWIM_SendByte(CMD_PACKET_READ) != 0)
        return 1;

    // 转换为接收监听状态，连续读取从机送上来的字节
    for (uint8_t i = 0; i < len; i++)
    {
        if (SWIM_ReadByte(&p_buf[i]) != 0)
        {
            return 1;
        }
    }
    return 0;
}

//
#define TestSwimHost 0
#if TestSwimHost
void TestSwimHost_APP(void)
{
    uint8_t my_config[6] = {0xAA, 0x12, 0x34, 0x56, 0x78, 0x55};
    uint8_t read_back[6] = {0};

    // 1. 初始化引脚
    SWIM_Host_Init();

    // 2. 激活从机（可在每次上电或需要通信时调用）
    SWIM_Host_SendResetSequence();

    // 3. 扔一包 6 字节的数据给从机B，由从机B自己决定写到它内部的某个 EEPROM 区域
    if (SWIM_Host_SendPacket(my_config, 6) == 0)
    {
        // 发送成功！从机已完整接住了这一包数据
    }

    // 4. 需要时，可以直接向从机B要回一包 6 字节的数据
    if (SWIM_Host_ReadPacket(read_back, 6) == 0)
    {
        // 接收并验证成功！my_config 的内容已经安全的从单片机B读取出来了
    }
}
#endif