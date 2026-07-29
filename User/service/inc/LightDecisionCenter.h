/**
 * @file LightDecisionCenter.h
 * @brief 主灯决策中心：仲裁按键 / Matter / 配网 / 电量等多源输入
 * @author hady
 * @date 2026-06-19
 * @layer Service
 * @note
 * 唯一主灯业务出口；渲染委托 LightSequenceScheduler → LightEffectEngine。
 * 参数持久化经 LightStorageProvider；Matter 上报与配网控制由 entry 注册回调注入。
 * 不直接 #include MatterBridge / ButtonService。
 */
#pragma once

#include "LightDecisionTypes.h"
#include "LightDimmingSpec.h"
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
 * @brief 多源主灯决策与参数记忆核心
 * @note
 * 数据流：各输入源 → 更新 m_userTargetParam / m_sceneState
 *         → ApplyArbitratedResult（Normal 场景）或 Start*Sequence（时序场景）
 *         → LightSequenceScheduler。
 * 仲裁优先级：极低电量强控 > 配网/识别时序 > 常规按键 / Matter 下行。
 * m_userTargetParam 记录用户目标亮度/WRGB/算子，配网时序期间不覆盖，松手后用于恢复。
 * 记忆灯光：按键/Matter 更新目标后 SafeSaveToStorage；Init 冷启动回读并恢复主灯。
 */
class LightDecisionCenter
{
  public:
    /** @brief 灯效算子函数指针（与 LightEffectProcessor 同签名） */
    using EffectRenderAction = uint32_t (*)(uint32_t start, uint32_t end, uint16_t elapsedMs, uint16_t totalMs);

    /** @brief Matter 属性上报（on / 亮度 / WRGB），由 entry 注入 */
    using MatterReportCallback = void (*)(bool on, uint8_t brightness, const uint16_t* pWrgb);

    /** @brief 配网控制动作，由 entry 注入 MatterBridge 操作 */
    using NetControlCallback = void (*)(NetControlAction action);

    /** @brief 低电量警告时触发指示灯，由 entry 注入 IndicatorServer */
    using BatteryWarnIndicatorCallback = void (*)();

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

    /** @brief 注册低电量警告指示回调（entry → IndicatorServer） */
    void RegisterBatteryWarnIndicatorCallback(BatteryWarnIndicatorCallback callback);

    /**
     * @brief 按键语义事件入口
     * @param event 短按亮度 / 双击颜色 / 配网长按等
     */
    void ProcessKeyEvent(KeyEventType event);

    /**
     * @brief Matter 下行控制
     * @param pWrgbBuffer 逻辑 WRGB，长度 >= 4
     * @param brightness  0~255
     * @param opId        渐变算子
     * @note 配网/识别时序进行中不抢占渲染，仅更新目标参数。
     */
    void ProcessMatterCommand(const uint16_t* pWrgbBuffer, uint8_t brightness, LightEffectOpId opId);

    /**
     * @brief 极低电量强控
     * @param isLow true=禁止亮灯并上报关；false=恢复 m_lastValidBrightness
     */
    void ProcessBatteryEvent(bool isLow);

    /**
     * @brief 电池电压等级变化
     * @param level Normal / LowWarning / CriticalEmpty
     */
    void ProcessBatteryVoltLevel(BatteryVoltLevel level);

    /** @brief Matter 配对成功 → 当前色 60% 快闪×2 + 淡入 100% + 完结后上报 */
    void ProcessMatterCommissioningComplete();

    /**
     * @brief Matter 识别开始/结束
     * @param active true=启动识别时序；false=停止并恢复用户目标
     */
    void ProcessMatterIdentify(bool active);

    /** @brief 触发全量 Matter 上报（入网后由 entry 调用） */
    void ReportStateToMatter();

    /** @brief 是否处于极低电量锁死（禁止亮灯） */
    bool IsBatteryLowLocked() const
    {
        return m_isBatteryLow;
    }

    /** @brief 回读当前目标亮度 0~255 */
    uint8_t GetCurrentBrightness() const;

    /** @brief 是否已成功完成过至少一次配网（非首次出厂） */
    bool HasCompletedFirstCommission() const;

    /** @brief 标记已完成首次配网并落盘 */
    void MarkFirstCommissionDone();

    /**
     * @brief 回读当前逻辑 WRGB
     * @param outChannels 输出缓冲
     * @param count       缓冲元素个数（最大拷贝 4）
     */
    void GetCurrentWrgb(uint16_t* outChannels, uint8_t count) const;

    /** @brief 电池通路就绪后重下发灯效（非低电且 Normal 场景） */
    void RefreshOutputIfAllowed();

  private:
    LightDecisionCenter()                                      = default;
    ~LightDecisionCenter()                                     = default;
    LightDecisionCenter(const LightDecisionCenter&)            = delete;
    LightDecisionCenter& operator=(const LightDecisionCenter&) = delete;

    /**
     * @brief NVM 持久化结构（亮度 / WRGB / 算子 ID）
     * @note magic=kPersistMagic 时有效；配网中止恢复依赖此结构未被时序改写。
     */
    struct PersistParam_T
    {
        uint8_t         magic;
        uint16_t        wrgb[4];
        uint8_t         brightness;
        LightEffectOpId op_id;
        uint8_t         reserved; /**< bit0..3 开机灯效；bit4=已完成首次配网 */
    } __attribute__((packed));

    /** @brief Normal 场景：单步灯效下发至调度器 */
    void ApplyArbitratedResult();

    /** @brief 请求落盘（ISR 安全：中断内只挂起，任务上下文再写 NVM） */
    void SafeSaveToStorage();

    /** @brief 实际 NVM 写入（仅任务上下文） */
    void SaveToStorageRaw();

    /** @brief FreeRTOS 定时器服务任务回调，执行 SaveToStorageRaw */
    static void DeferredSaveDispatch(void* param1, uint32_t param2);

    /** @brief 加载出厂默认亮度/WRGB/算子 */
    void LoadDefaults();

    /** @brief 校验 NVM 读回参数合法性 */
    bool IsPersistValid(const PersistParam_T& param) const;

    /** @brief 主灯重置预警时序：熄灭400ms→正常闪×3→慢闪×1→熄灭2s（完结后再判是否复位） */
    void StartNetConfigSequence();

    /** @brief 重置预警中止：淡出熄灭 → 淡入恢复 m_userTargetParam */
    void StopNetConfigAndRestoreRaw();

    /**
     * @brief 重置预警时序完结
     * @note 仅当 m_factoryResetArmed（已按住过 13s）时落盘并工厂复位；否则只恢复常态。
     */
    void OnFactoryResetWarnSequenceFinishedRaw();

    /** @brief 重置预警完结：ISR 安全中转（时序回调在 sleeptimer 中断） */
    static void OnFactoryResetWarnSequenceFinishedBridge();

    /** @brief FreeRTOS 定时器服务任务：执行预警完结落盘与工厂复位 */
    static void DeferredFactoryResetWarnDispatch(void* param1, uint32_t param2);

    /** @brief 工厂复位重启后灯效：快闪×2 → 淡入出厂默认 */
    void StartFactoryResetDoneSequence();

    /** @brief 复位后开机灯效完结：场景回到 Normal */
    void OnFactoryResetDoneSequenceFinishedRaw();

    /** @brief 复位后开机灯效完结桥接 */
    static void OnFactoryResetDoneSequenceFinishedBridge();

    /** @brief Matter 识别时序：#5 @100% 正常闪循环，直至 Identify 结束 */
    void StartIdentifySequence();

    /** @brief 配对成功时序：当前色 60% 快闪×2 → 淡入全亮 */
    void StartCommissioningSuccessSequence();

    /** @brief 配对成功时序完结：落盘 100% 亮度并上报 Matter */
    void OnPairSuccessSequenceFinishedRaw();

    /** @brief 配对成功完结回调桥接（供调度器静态注册） */
    static void OnPairSuccessSequenceFinishedBridge();

    /** @brief 识别循环被外部停止后：渐变恢复用户目标（无自然完结路径） */
    void RestoreAfterIdentifyRaw();

    /**
     * @brief 亮度循环索引 → 渐变算子与过渡时长
     * @param brightnessIndex 0=100%，1=35%，2=0%
     */
    LightEffectOpId BrightnessIndexToOpId(uint8_t brightnessIndex);

    /**
     * @brief Matter 控灯后武装短按循环
     * @note 灯亮：下次短按关灯；已关：下次短按 100%。再按恢复 100%→35%→关 循环。
     */
    void ArmBrightnessCycleAfterMatterRaw(uint8_t brightness);

    /** @brief 按 NVM 亮度恢复短按循环索引 */
    void SyncBrightnessCycleIndexFromStoredRaw();

    /** @brief 按 NVM WRGB 对齐色库循环索引（颜色记忆） */
    void SyncColorCycleIndexFromStoredRaw();

    /** @brief 若已注册则上报当前 on/亮度/WRGB（ISR 安全） */
    void ReportToMatterIfRegistered();

    /** @brief 实际上报（仅任务上下文） */
    void ReportToMatterRaw();

    /** @brief FreeRTOS 定时器服务任务回调，执行 ReportToMatterRaw */
    static void DeferredReportDispatch(void* param1, uint32_t param2);

    /** @brief 调用已注册的配网控制回调 */
    void InvokeNetControlRaw(NetControlAction action);

    /**
     * @brief 用户尝试开灯时触发低电量指示
     * @param targetBrightness 目标亮度，0 时不触发
     */
    void NotifyBatteryWarnIfNeeded(uint8_t targetBrightness);

    LightSequenceScheduler*      m_pSequence{nullptr};
    LightStorageProvider*        m_pStorage{nullptr};
    MatterReportCallback         m_matterReporter{nullptr};
    NetControlCallback           m_netControl{nullptr};
    BatteryWarnIndicatorCallback m_batteryWarnIndicator{nullptr};

    LightSceneState m_sceneState{LightSceneState::Normal}; /**< 场景状态机 */
    bool            m_factoryResetArmed{false};            /**< 已按住≥13s：时序完结后执行工厂复位 */
    bool            m_isBatteryLow{false};                 /**< 极低电量强控锁 */
    bool            m_isBatteryLowWarning{false};          /**< 低电量警告（可开灯） */
    uint8_t         m_lastValidBrightness{255U};           /**< 上次非零亮度，低电恢复用 */
    uint8_t         m_brightnessCycleIndex{0U};            /**< 短按亮度循环索引 */
    uint8_t         m_colorCycleIndex{0U};                 /**< 双击颜色循环索引 */
    uint16_t        m_TransitionMs{400};                   /**< 当前单步渐变时长 (ms) */
    uint32_t        m_lastPairSuccessEffectMs{0U};         /**< 上次配对成功灯效触发时刻 (ms) */
    bool            m_isPairSuccessSequenceActive{false};  /**< 配对成功时序进行中，防重复启动 */

    PersistParam_T m_userTargetParam{}; /**< 用户目标亮度/WRGB/算子（NVM 镜像） */

    static constexpr uint8_t  kPersistMagic                   = 0x5AU;
    static constexpr uint8_t  kBootEffectMask                 = 0x0FU;
    static constexpr uint8_t  kBootEffectNone                 = 0U;
    static constexpr uint8_t  kBootEffectFactoryResetDone     = 1U; /**< reserved 低 4 位：复位后开机播快闪+淡入 */
    static constexpr uint8_t  kFirstCommissionDoneFlag        = 0x10U; /**< reserved bit4：已非首次配网 */
    static constexpr uint16_t kTransitionMs                   = LightDimmingSpec::kFadeOutMs;
    static constexpr uint8_t  kDefaultBrightness              = 255U;
    static constexpr uint8_t  kFactoryResetWarnBrightness     = 166U; /**< ≈65%，仅复位后开机快闪用 */
    static constexpr uint32_t kPairSuccessEffectDebounceMs    = 3500U;
    static constexpr uint8_t  kBrightnessLevels[]             = {255U, 91U, 0U};

    static const EffectRenderAction kActionTable[static_cast<uint8_t>(LightEffectOpId::MaxOperators)];
};
