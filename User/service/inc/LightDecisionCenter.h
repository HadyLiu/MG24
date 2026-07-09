/**
 * @file LightDecisionCenter.h
 * @brief 主灯决策中心：仲裁按键 / Matter / 配网 / 电量等多源输入
 * @author hady
 * @date 2026-06-19
 * @layer Service
 * @note
 * 唯一灯光业务出口；渲染委托 LightSequenceScheduler；
 * 参数持久化经 LightStorageProvider；Matter 上报与配网控制由 entry
 * 注册回调注入。 不直接 #include MatterBridge / ButtonService。
 */
#pragma once

#include "LightDecisionTypes.h"
#include "LightEffectProcessor.h"
#include "LightSequenceScheduler.h"
#include <stdint.h>

/**
 * @brief 灯光参数持久化抽象接口
 * @note 由 LightNvmStorage 实现，委托 HalNvmstorage。
 */
class LightStorageProvider
{
  public:
    virtual ~LightStorageProvider()                        = default;
    virtual bool Read(uint8_t* pDest, uint16_t size)       = 0;
    virtual bool Write(const uint8_t* pSrc, uint16_t size) = 0;
};

/**
 * @class LightDecisionCenter
 * @brief 多源灯光决策与参数记忆核心
 */
class LightDecisionCenter
{
  public:
    using EffectRenderAction = uint32_t (*)(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /** @brief Matter 属性上报（on / 亮度 / WRGB），由 entry 注入 */
    using MatterReportCallback = void (*)(bool on, uint8_t brightness, const uint16_t* pWrgb);
    /** @brief 配网控制动作，由 entry 注入 MatterBridge 操作 */
    using NetControlCallback = void (*)(NetControlAction action);
    /**
     * @brief 获取决策中心单例
     * @return LightDecisionCenter 引用
     */
    static LightDecisionCenter& Instance()
    {
        static LightDecisionCenter instance;
        return instance;
    }

    /**
     * @brief 初始化并冷启动回读 NVM
     * @param pSequence 灯效调度器
     * @param pStorage  持久化适配器
     */
    void Init(LightSequenceScheduler* pSequence, LightStorageProvider* pStorage);

    /** @brief 注册 Matter 上报回调（entry → MatterBridge） */
    void RegisterMatterReporter(MatterReportCallback callback);

    /** @brief 注册配网控制回调（entry → MatterBridge） */
    void RegisterNetControlCallback(NetControlCallback callback);

    /** @brief 按键语义事件入口 */
    void ProcessKeyEvent(KeyEventType event);

    /**
     * @brief Matter 下行控制
     * @param pWrgbBuffer 逻辑 WRGB，长度 >= 4
     * @param brightness  0~255
     * @param opId        渐变算子
     */
    void ProcessMatterCommand(const uint16_t* pWrgbBuffer, uint8_t brightness, LightEffectOpId opId);

    /** @brief 极低电量强控（true=禁止亮灯并上报关） */
    void ProcessBatteryEvent(bool isLow);

    /** @brief 电池电压等级变化（Normal / LowWarning / CriticalEmpty） */
    void ProcessBatteryVoltLevel(BatteryVoltLevel level);

    /** @brief 低电量警告指示回调（entry → 红灯闪烁），在用户尝试开灯时触发 */
    using BatteryWarnIndicatorCallback = void (*)();
    void RegisterBatteryWarnIndicatorCallback(BatteryWarnIndicatorCallback callback);

    /** @brief Matter 配网成功 → 成功时序灯效 + 全量上报 */
    void ProcessMatterCommissioningComplete();

    /** @brief Matter 识别开始/结束 → 识别时序灯效 */
    void ProcessMatterIdentify(bool active);

    /** @brief 触发全量 Matter 上报（入网后由 entry 接线调用） */
    void ReportStateToMatter();

    /** @brief 是否处于极低电量锁死 */
    bool IsBatteryLowLocked() const
    {
        return m_isBatteryLow;
    }

    /** @brief 回读当前目标亮度 */
    uint8_t GetCurrentBrightness() const;

    /** @brief 回读当前逻辑 WRGB */
    void GetCurrentWrgb(uint16_t* outChannels, uint8_t count) const;

    /** @brief 电池通路就绪后恢复主灯输出（非低电且亮度>0 时重下发灯效） */
    void RefreshOutputIfAllowed();

  private:
    LightDecisionCenter()                                      = default;
    ~LightDecisionCenter()                                     = default;
    LightDecisionCenter(const LightDecisionCenter&)            = delete;
    LightDecisionCenter& operator=(const LightDecisionCenter&) = delete;

    /** @brief NVM 持久化结构（亮度 / WRGB / 算子 ID） */
    struct PersistParam_T
    {
        uint8_t         magic;
        uint16_t        wrgb[4];
        uint8_t         brightness;
        LightEffectOpId op_id;
        uint8_t         reserved;
    } __attribute__((packed));

    void            ApplyArbitratedResult();
    void            SafeSaveToStorage();
    void            LoadDefaults();
    bool            IsPersistValid(const PersistParam_T& param) const;
    void            StartNetConfigSequence();
    void            StartIdentifySequence();
    void            StartCommissioningSuccessSequence();
    LightEffectOpId BrightnessIndexToOpId(uint8_t brightnessIndex);
    void            ReportToMatterIfRegistered();
    void            InvokeNetControlRaw(NetControlAction action);
    void            NotifyBatteryWarnIfNeeded(uint8_t targetBrightness);

    LightSequenceScheduler*      m_pSequence{nullptr};
    LightStorageProvider*        m_pStorage{nullptr};
    MatterReportCallback         m_matterReporter{nullptr};
    NetControlCallback           m_netControl{nullptr};
    BatteryWarnIndicatorCallback m_batteryWarnIndicator{nullptr};

    LightSceneState m_sceneState{LightSceneState::Normal};
    bool            m_isBatteryLow{false};
    bool            m_isBatteryLowWarning{false};
    uint8_t         m_lastValidBrightness{255U};
    uint8_t         m_brightnessCycleIndex{0U};
    uint8_t         m_colorCycleIndex{0U};
    uint16_t        m_TransitionMs{400}; ///< 渐变过渡时间（ms）

    PersistParam_T m_userTargetParam{};

    static constexpr uint8_t  kPersistMagic       = 0x5AU;
    static constexpr uint16_t kTransitionMs       = 400U;
    static constexpr uint8_t  kDefaultBrightness  = 255U;
    static constexpr uint8_t  kBrightnessLevels[] = {255U, 89U, 0U}; // 100% -> 35% -> 0%

    static const EffectRenderAction kActionTable[static_cast<uint8_t>(LightEffectOpId::MaxOperators)];
};
