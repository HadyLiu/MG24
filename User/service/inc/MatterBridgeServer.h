/**
 * @file MatterBridgeServer.h
 * @brief Matter 业务服务层：邮箱消费 + Matter 栈 ScheduleWork 操作
 * @author hady
 * @date 2026-06-16
 * @layer Service
 * @note
 * 对外仅暴露两个数据接口，数据只能经此二者进出：
 *   - UploadState：唯一上报(输入)，LDC 灯光状态(开关/亮度/WRGB) → Matter；
 *   - RegisterDownlinkHandler：唯一下行(输出)，Matter
 数据(开关/亮度/WRGB/识别)
 * → LDC。 其余邮箱/栈操作/状态查询全部私有；MatterBridge 的投递与查询接线在
 * Init() 内部完成， 上层无需感知。
 */
#pragma once

#include "MatterMailPacket.h"
#include <cstdint>

class MatterBridgeServer
{
  public:
    /**
     * @brief Matter 下行事件类型
     * @note 单一下行回调通过本字段区分语义，避免多个注册回调。
     */
    enum class DownlinkKind : uint8_t
    {
        kLightControl = 0,  /**< 灯光控制：on / brightness / wrgb 均有效 */
        kIdentify,          /**< 识别开始/结束：on 表示 active */
        kCommissioningDone, /**< 配网完成 */
        kNetworkConnected   /**< 入网就绪：请求全量回报 */
    };

    /**
     * @brief Matter 下行统一载荷（→ LightDecisionCenter）
     * @note kLightControl 始终携带补齐后的完整状态(开关 + 亮度 + WRGB)。
     */
    struct DownlinkData
    {
        DownlinkKind kind;       /**< 事件类型 */
        bool         on;         /**< 开关 / 识别 active */
        uint8_t      brightness; /**< 目标亮度 0~255 */
        uint16_t     wrgb[4];    /**< 逻辑 WRGB 0~1023 */
    };

    /** @brief 唯一下行回调：Matter 数据 → LightDecisionCenter（entry 绑定） */
    using DownlinkHandler = void (*)(const DownlinkData& data);

    static MatterBridgeServer& Instance()
    {
        static MatterBridgeServer instance;
        return instance;
    }
    void Init();

    /** @brief 注册唯一下行回调（由 entry 接线至 LightDecisionCenter） */
    void RegisterDownlinkHandler(DownlinkHandler handler);

    // 读取开关/亮度/WRGB 下发给lighting
    void OnMatterDataReceived(MatterDownlinkUploadPayload data);

    /**
     * @brief 本地灯光状态同步到下行缓存（按键/记忆上报时调用）
     * @param on         开关
     * @param brightness 亮度 0~255
     * @param pWrgb      逻辑 WRGB，可为 nullptr 表示不改颜色
     */
    void SyncLocalLightState(bool on, uint8_t brightness, const uint16_t* pWrgb);

  private:
    MatterBridgeServer()  = default;
    ~MatterBridgeServer() = default;

    /** @brief 用缓存的完整状态构造灯控载荷并发出 */
    void EmitLightControlRaw();

    /** @brief 调用已注册的下行回调（空则丢弃） */
    void EmitDownlinkRaw(const DownlinkData& data);

    DownlinkHandler m_downlink{nullptr};

    /** @brief 下行合并缓存：动作不带颜色 / 颜色不带亮度时补齐为完整状态 */
    bool     m_cacheOn{false};
    uint8_t  m_cacheBrightness{255U};
    uint8_t  m_lastOnBrightness{255U}; /**< 上次非零亮度，OnOff 开灯时回退用 */
    uint16_t m_cacheWrgb[4]{1023U, 0U, 0U, 0U};
};
