/**
 * @file LightDecisionCenter.cpp
 * @brief 主灯决策中心实现
 * @author hady
 * @date 2026-06-19
 * @layer Service
 * @note
 * 多源仲裁优先级：极低电量 > 配网/识别时序 > 常规用户/Matter 控制。
 * 记忆灯光：按键/Matter 变更用户目标后经 SafeSaveToStorage 防抖写入 NVM，
 * 冷启动 Init 回读并 Apply；极低电量强控关灯不落盘，避免覆盖用户记忆。
 */
#include "LightDecisionCenter.h"
#include "DebugLog.h"
#include "LightDimmingSpec.h"
#include "sl_sleeptimer.h"
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

namespace {

/** @brief 双击颜色循环调色板（逻辑 WRGB，0~1023） */
static constexpr uint16_t kColorPalette[][4] = {
    {1023U, 0U, 0U, 0U},     /* #5 */
    {409U, 1023U, 0U, 0U},   /* #6  */
    {409U, 0U, 235U, 563U},  /* #7 */
    {327U, 0U, 235U, 1023U}, /* #8 */
    {0U, 1023U, 179U, 0U},   /* #9  */
    {0U, 1023U, 102U, 0U},   /* #10 */
    {0U, 1023U, 0U, 0U},     /* #13 */
    {0U, 1023U, 102U, 59U},  /* #20 */
    {0U, 1023U, 240U, 404U}, /* #25 */
    {0U, 240U, 240U, 1023U}, /* #30 */
    {0U, 522U, 844U, 522U},  /* # */
    {0U, 522U, 1023U, 159U}, /*  */
    {0U, 1023U, 322U, 0U}    /*  */
};

static constexpr uint8_t kColorPaletteCount = static_cast<uint8_t>(sizeof(kColorPalette) / sizeof(kColorPalette[0]));

} // namespace

/** @brief 算子 ID → LightEffectProcessor 函数指针映射表（常驻 Flash） */
const LightDecisionCenter::EffectRenderAction LightDecisionCenter::kActionTable[] = {
    [static_cast<uint8_t>(LightEffectOpId::DirectKeep)]      = &LightEffectProcessor::GetKeep,
    [static_cast<uint8_t>(LightEffectOpId::LinearLerp)]      = &LightEffectProcessor::GetLerp,
    [static_cast<uint8_t>(LightEffectOpId::Breath80Bytes)]   = &LightEffectProcessor::CalcBreath80BytesFactor,
    [static_cast<uint8_t>(LightEffectOpId::Bezier80FadeIn)]  = &LightEffectProcessor::GetBezier80BytesFactorFadeIn,
    [static_cast<uint8_t>(LightEffectOpId::Bezier40FadeIn)]  = &LightEffectProcessor::GetBezier40BytesFactorFadeIn,
    [static_cast<uint8_t>(LightEffectOpId::Bezier80FadeOut)] = &LightEffectProcessor::GetBezier80BytesFactorFadeOut,
    [static_cast<uint8_t>(LightEffectOpId::Bezier40FadeOut)] = &LightEffectProcessor::GetBezier40BytesFactorFadeOut,
    [static_cast<uint8_t>(LightEffectOpId::Blink)]           = &LightEffectProcessor::GetBlink};

/**
 * @brief 初始化决策中心并冷启动回读 NVM
 * @param pSequence 灯效调度器指针
 * @param pStorage  持久化适配器指针
 */
void LightDecisionCenter::Init(LightSequenceScheduler* pSequence, LightStorageProvider* pStorage)
{
    if ((pSequence == nullptr) || (pStorage == nullptr))
    {
        return;
    }

    m_pSequence    = pSequence;
    m_pStorage     = pStorage;
    m_sceneState   = LightSceneState::Normal;
    m_isBatteryLow = false;

    const bool readOk = m_pStorage->Read(reinterpret_cast<uint8_t*>(&m_userTargetParam), sizeof(PersistParam_T));
    if (!readOk || !IsPersistValid(m_userTargetParam))
    {
        LoadDefaults();
        SafeSaveToStorage();
    }

    if (m_userTargetParam.brightness > 0U)
    {
        m_lastValidBrightness = m_userTargetParam.brightness;
    }

    SyncBrightnessCycleIndexFromStoredRaw();

    // 工厂复位前写入的开机灯效标记：播完后清标记；灯光参数已是出厂默认
    if (m_userTargetParam.reserved == kBootEffectFactoryResetDone)
    {
        m_userTargetParam.reserved = kBootEffectNone;
        SafeSaveToStorage();
        m_sceneState = LightSceneState::NetConfiguring;
        StartFactoryResetDoneSequence();
        return;
    }

    ApplyArbitratedResult();
}

/**
 * @brief 注册 Matter 上报回调
 * @param callback entry 注入的上报函数
 */
void LightDecisionCenter::RegisterMatterReporter(MatterReportCallback callback)
{
    m_matterReporter = callback;
}

/**
 * @brief 注册配网控制回调
 * @param callback entry 注入的 MatterBridge 操作
 */
void LightDecisionCenter::RegisterNetControlCallback(NetControlCallback callback)
{
    m_netControl = callback;
}

/**
 * @brief 注册低电量警告指示回调
 * @param callback entry 注入的 IndicatorServer 入口
 */
void LightDecisionCenter::RegisterBatteryWarnIndicatorCallback(BatteryWarnIndicatorCallback callback)
{
    m_batteryWarnIndicator = callback;
}

/**
 * @brief 调用已注册的配网控制回调
 * @param action 清网 / 关窗等动作
 */
void LightDecisionCenter::InvokeNetControlRaw(NetControlAction action)
{
    if (m_netControl != nullptr)
    {
        m_netControl(action);
    }
}

/**
 * @brief 处理按键语义事件
 * @param event KeyEventType 事件类型
 * @note 极低电量时整段丢弃；配网时序由 StartNetConfigSequence 接管渲染。
 */
void LightDecisionCenter::ProcessKeyEvent(KeyEventType event)
{
    if (m_isBatteryLow)
    {
        return;
    }
    LOG_LIGHT_DC("Button: %u", static_cast<uint8_t>(event));
    switch (event)
    {
    case KeyEventType::ShortPressCycleBrightness: {
        m_brightnessCycleIndex++;
        if (m_brightnessCycleIndex >= 3U)
        {
            m_brightnessCycleIndex = 0U;
        }
        const uint8_t brightness = kBrightnessLevels[m_brightnessCycleIndex];
        NotifyBatteryWarnIfNeeded(brightness);
        if (brightness > 0U)
        {
            m_lastValidBrightness = brightness;
        }
        m_userTargetParam.brightness = brightness;
        m_userTargetParam.op_id      = BrightnessIndexToOpId(m_brightnessCycleIndex);
        m_sceneState                 = LightSceneState::Normal;

        ApplyArbitratedResult();
        SafeSaveToStorage();
        ReportToMatterIfRegistered();
        break;
    }
    case KeyEventType::DoublePressCycleColor: {
        m_colorCycleIndex++;
        if (m_colorCycleIndex >= kColorPaletteCount)
        {
            m_colorCycleIndex = 0U;
        }

        memcpy(m_userTargetParam.wrgb, kColorPalette[m_colorCycleIndex], sizeof(m_userTargetParam.wrgb));
        m_userTargetParam.op_id = LightEffectOpId::LinearLerp;
        m_sceneState            = LightSceneState::Normal;
        m_TransitionMs          = 400;
        ApplyArbitratedResult();
        SafeSaveToStorage();
        ReportToMatterIfRegistered();
        break;
    }
    case KeyEventType::LongPressClearNetLighting: {
        m_sceneState = LightSceneState::NetConfiguring;
        StartNetConfigSequence();
        break;
    }
    case KeyEventType::LongPressStopNet: {
        if (m_sceneState == LightSceneState::NetConfiguring)
        {
            m_sceneState = LightSceneState::Normal;
            StopNetConfigAndRestoreRaw();
        }
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 处理 Matter 下行控制
 * @param pWrgbBuffer 逻辑 WRGB 数组
 * @param brightness  目标亮度 0~255
 * @param opId        渐变算子
 * @note NetConfiguring / MatterIdentifying 期间仅更新参数，不 Apply。
 */
void LightDecisionCenter::ProcessMatterCommand(const uint16_t* pWrgbBuffer, uint8_t brightness, LightEffectOpId opId)
{
    if (pWrgbBuffer == nullptr)
    {
        return;
    }

    NotifyBatteryWarnIfNeeded(brightness);
    if (m_isBatteryLow && (brightness > 0U))
    {
        return;
    }

    memcpy(m_userTargetParam.wrgb, pWrgbBuffer, sizeof(m_userTargetParam.wrgb));
    m_userTargetParam.brightness = brightness;
    m_userTargetParam.op_id      = opId;

    // §1.2：网关/Matter 调光 200ms 淡入，关灯 400ms 淡出
    m_TransitionMs = (brightness > 0U) ? LightDimmingSpec::kFadeInMs : LightDimmingSpec::kFadeOutMs;

    // Matter 开灯/调光/改色/色温后：下一次短按直接关灯；Matter 关灯后下次短按 100%
    ArmBrightnessCycleAfterMatterRaw(brightness);

    if (brightness > 0U)
    {
        m_lastValidBrightness = brightness;
    }

    if ((m_sceneState == LightSceneState::Normal) || (m_sceneState == LightSceneState::LowBattery))
    {
        m_sceneState = LightSceneState::Normal;
        ApplyArbitratedResult();
    }

    // 记忆灯光：用户目标（含关灯 brightness=0）落盘，供下次上电恢复
    SafeSaveToStorage();
}

/**
 * @brief 处理极低电量强控
 * @param isLow true=停时序、亮度置 0 并上报关；false=恢复 m_lastValidBrightness
 */
void LightDecisionCenter::ProcessBatteryEvent(bool isLow)
{
    m_isBatteryLow = isLow;

    if (m_isBatteryLow)
    {
        m_sceneState                  = LightSceneState::LowBattery;
        m_isPairSuccessSequenceActive = false;
        m_pSequence->StopSequence();
        m_userTargetParam.brightness = 0U;
        ReportToMatterIfRegistered();
    }
    else if (m_sceneState == LightSceneState::LowBattery)
    {
        m_sceneState                 = LightSceneState::Normal;
        m_userTargetParam.brightness = m_lastValidBrightness;
        SafeSaveToStorage();
        ApplyArbitratedResult();
        ReportToMatterIfRegistered();
    }
}

/**
 * @brief 电池电压等级变化分发
 * @param level CriticalEmpty→强控；LowWarning→仅警告标志；Normal→解除强控
 */
void LightDecisionCenter::ProcessBatteryVoltLevel(BatteryVoltLevel level)
{
    switch (level)
    {
    case BatteryVoltLevel::CriticalEmpty:
        m_isBatteryLowWarning = false;
        ProcessBatteryEvent(true);
        break;
    case BatteryVoltLevel::LowWarning:
        m_isBatteryLowWarning = true;
        break;
    case BatteryVoltLevel::Normal:
    default:
        m_isBatteryLowWarning = false;
        if (m_isBatteryLow)
        {
            ProcessBatteryEvent(false);
        }
        break;
    }
}

/**
 * @brief 用户尝试开灯时，低电/警告态触发指示灯回调
 * @param targetBrightness 目标亮度，0 时不触发
 */
void LightDecisionCenter::NotifyBatteryWarnIfNeeded(uint8_t targetBrightness)
{
    if (targetBrightness == 0U)
    {
        return;
    }

    if ((m_isBatteryLow || m_isBatteryLowWarning) && (m_batteryWarnIndicator != nullptr))
    {
        m_batteryWarnIndicator();
    }
}

/**
 * @brief Matter 配网成功：当前色 60% 快闪×2 后淡入 100%，完结后再落盘并上报
 * @note Matter 栈可能对同一次配网重复投递 kCommissioningComplete，窗口内只播一次灯效。
 */
void LightDecisionCenter::ProcessMatterCommissioningComplete()
{
    if (m_isBatteryLow)
    {
        return;
    }

    if (m_isPairSuccessSequenceActive)
    {
        LOG_LIGHT_DC("PairSuccess sequence already active");
        return;
    }

    const uint32_t nowMs = sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count());
    if ((nowMs - m_lastPairSuccessEffectMs) < kPairSuccessEffectDebounceMs)
    {
        LOG_LIGHT_DC("CommissioningComplete debounced");
        return;
    }
    m_lastPairSuccessEffectMs     = nowMs;
    m_isPairSuccessSequenceActive = true;
    m_sceneState                  = LightSceneState::Normal;

    if (m_pSequence != nullptr)
    {
        m_pSequence->StopSequence();
    }
    StartCommissioningSuccessSequence();
}

/**
 * @brief Matter 识别控制
 * @param active true=进入识别时序；false=停止时序并恢复用户目标
 */
void LightDecisionCenter::ProcessMatterIdentify(bool active)
{
    if (m_isBatteryLow)
    {
        return;
    }

    if (active)
    {
        m_sceneState = LightSceneState::MatterIdentifying;
        StartIdentifySequence();
    }
    else if (m_sceneState == LightSceneState::MatterIdentifying)
    {
        m_sceneState                  = LightSceneState::Normal;
        m_isPairSuccessSequenceActive = false;
        m_pSequence->StopSequence();
        ApplyArbitratedResult();
    }
}

/**
 * @brief 对外触发 Matter 全量上报
 */
void LightDecisionCenter::ReportStateToMatter()
{
    ReportToMatterIfRegistered();
}

/**
 * @brief 回读当前目标亮度
 * @return 0~255
 */
uint8_t LightDecisionCenter::GetCurrentBrightness() const
{
    return m_userTargetParam.brightness;
}

/**
 * @brief 回读当前逻辑 WRGB
 * @param outChannels 输出缓冲
 * @param count       最多拷贝 4 通道
 */
void LightDecisionCenter::GetCurrentWrgb(uint16_t* outChannels, uint8_t count) const
{
    if (outChannels == nullptr)
    {
        return;
    }

    const uint8_t copyCount = (count > 4U) ? 4U : count;
    memcpy(outChannels, m_userTargetParam.wrgb, static_cast<size_t>(copyCount) * sizeof(uint16_t));
}

/**
 * @brief 供电通路就绪后重下发灯效
 * @note 内部走 ApplyArbitratedResult，受场景与低电锁约束。
 */
void LightDecisionCenter::RefreshOutputIfAllowed()
{
    ApplyArbitratedResult();
}

/**
 * @brief 仲裁结果下发至 LightSequenceScheduler
 * @note 仅在 Normal 场景且非极低电量时渲染单步灯效。
 */
void LightDecisionCenter::ApplyArbitratedResult()
{
    if (m_sceneState != LightSceneState::Normal)
    {
        return;
    }

    if (m_isBatteryLow)
    {
        return;
    }

    uint8_t opIndex = static_cast<uint8_t>(m_userTargetParam.op_id);
    if (opIndex >= static_cast<uint8_t>(LightEffectOpId::MaxOperators))
    {
        opIndex = static_cast<uint8_t>(LightEffectOpId::LinearLerp);
    }

    EffectRenderAction pAction = kActionTable[opIndex];
    if (pAction == nullptr)
    {
        return;
    }

    m_pSequence->StartSingleEffect(pAction, m_userTargetParam.wrgb, m_userTargetParam.brightness, m_TransitionMs);
    // 切换到默认
    m_TransitionMs = kTransitionMs;
}

/**
 * @brief 防抖写入 NVM（实际落盘，必须在任务上下文调用）
 * @note memcmp 与上次写入相同则跳过，减少 Flash 擦写。
 */
void LightDecisionCenter::SaveToStorageRaw()
{
    if (m_pStorage == nullptr)
    {
        return;
    }

    static PersistParam_T lastSavedParam = {0};
    m_userTargetParam.magic              = kPersistMagic;

    if (memcmp(&m_userTargetParam, &lastSavedParam, sizeof(PersistParam_T)) != 0)
    {
        if (m_pStorage->Write(reinterpret_cast<const uint8_t*>(&m_userTargetParam), sizeof(PersistParam_T)))
        {
            lastSavedParam = m_userTargetParam;
        }
    }
}

/**
 * @brief FreeRTOS 定时器服务任务回调：在任务上下文执行 NVM 落盘
 */
void LightDecisionCenter::DeferredSaveDispatch(void* param1, uint32_t param2)
{
    (void)param1;
    (void)param2;
    LightDecisionCenter::Instance().SaveToStorageRaw();
}

/**
 * @brief 请求记忆灯光落盘（ISR 安全）
 * @note 按键链跑在 sl_sleeptimer 中断：严禁在中断里 nvm3_write/repack，否则会死机。
 *       中断上下文经 xTimerPendFunctionCallFromISR 转到任务后再写；
 *       任务上下文（Matter 下行等）直接写。
 */
void LightDecisionCenter::SafeSaveToStorage()
{
    if (xPortIsInsideInterrupt() != pdFALSE)
    {
        BaseType_t       higherPriorityTaskWoken = pdFALSE;
        const BaseType_t posted =
            xTimerPendFunctionCallFromISR(DeferredSaveDispatch, nullptr, 0U, &higherPriorityTaskWoken);
        if (posted == pdPASS)
        {
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
        return;
    }

    SaveToStorageRaw();
}

/**
 * @brief 加载出厂默认参数
 * @note 亮度 100%、暖白 WRGB、LinearLerp 算子。
 */
void LightDecisionCenter::LoadDefaults()
{
    m_userTargetParam.magic      = kPersistMagic;
    m_userTargetParam.brightness = kDefaultBrightness;
    m_userTargetParam.op_id      = LightEffectOpId::LinearLerp;
    m_userTargetParam.wrgb[0]    = 1023U;
    m_userTargetParam.wrgb[1]    = 0U;
    m_userTargetParam.wrgb[2]    = 0U;
    m_userTargetParam.wrgb[3]    = 0U;
    m_userTargetParam.reserved   = kBootEffectNone;
    m_lastValidBrightness        = kDefaultBrightness;
    m_brightnessCycleIndex       = 0U;
    m_colorCycleIndex            = 0U;
}

/**
 * @brief 校验 NVM 读回结构
 * @param param 待校验参数
 * @return true=magic 与 op_id 合法
 */
bool LightDecisionCenter::IsPersistValid(const PersistParam_T& param) const
{
    if (param.magic != kPersistMagic)
    {
        return false;
    }

    const uint8_t opIndex = static_cast<uint8_t>(param.op_id);
    return opIndex < static_cast<uint8_t>(LightEffectOpId::MaxOperators);
}

/**
 * @brief 主灯工厂重置预警时序（一次性，4 步）
 * @note Step0 熄灭 400ms → Step1 当前色快闪×3(800ms) → Step2 慢闪×1(2400ms)
 *       → Step3 熄灭 2s；完结后写 NVM 标记并触发工厂复位。
 */
void LightDecisionCenter::StartNetConfigSequence()
{
    if (m_pSequence == nullptr)
    {
        return;
    }

    LightSequenceScheduler::SequenceStep kNetConfigSteps[] = {
        {LightEffectProcessor::GetKeep, {0U, 0U, 0U, 0U}, 0U, 400U, 0U},
        {LightEffectProcessor::GetBlink, {0U, 0U, 0U, 0U}, kFactoryResetWarnBrightness, 800U, 2U},
        {LightEffectProcessor::GetBlink, {0U, 0U, 0U, 0U}, kFactoryResetWarnBrightness, 2400U, 0U},
        {LightEffectProcessor::GetKeep, {0U, 0U, 0U, 0U}, 0U, 2000U, 0U}};
    memcpy(kNetConfigSteps[1].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));
    memcpy(kNetConfigSteps[2].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_pSequence->RegisterSequenceFinishedCallback(OnFactoryResetWarnSequenceFinishedBridge);
    m_pSequence->StartSequence(kNetConfigSteps, 4U, false);
}

/**
 * @brief 重置预警中止：主灯先淡出熄灭，再淡入恢复预警前亮度与颜色
 */
void LightDecisionCenter::StopNetConfigAndRestoreRaw()
{
    if (m_pSequence == nullptr)
    {
        return;
    }

    m_pSequence->RegisterSequenceFinishedCallback(nullptr);

    LightSequenceScheduler::SequenceStep restoreSteps[] = {
        {LightEffectProcessor::GetBezier40BytesFactorFadeOut, {0U, 0U, 0U, 0U}, 0U, kTransitionMs, 0U},
        {LightEffectProcessor::GetBezier40BytesFactorFadeIn,
         {0U, 0U, 0U, 0U},
         m_userTargetParam.brightness,
         kTransitionMs,
         0U}};
    memcpy(restoreSteps[1].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_pSequence->StartSequence(restoreSteps, 2U, false);
}

/**
 * @brief 重置预警完结：落盘出厂默认 + 开机灯效标记，再触发工厂复位
 * @note 仅任务上下文调用（NVM / Matter 不可在 sleeptimer 中断执行）。
 */
void LightDecisionCenter::OnFactoryResetWarnSequenceFinishedRaw()
{
    m_sceneState = LightSceneState::Normal;

    LoadDefaults();
    m_userTargetParam.reserved = kBootEffectFactoryResetDone;
    // 必须同步落盘：随后工厂复位会重启，异步写可能来不及
    SaveToStorageRaw();

    LOG_LIGHT_DC("FactoryReset warn done, persist boot effect + reset");
    InvokeNetControlRaw(NetControlAction::FactoryReset);
}

/**
 * @brief FreeRTOS 定时器服务任务：预警完结落盘与工厂复位
 */
void LightDecisionCenter::DeferredFactoryResetWarnDispatch(void* param1, uint32_t param2)
{
    (void)param1;
    (void)param2;
    LightDecisionCenter::Instance().OnFactoryResetWarnSequenceFinishedRaw();
}

/**
 * @brief 重置预警完结静态桥接（ISR 安全）
 * @note 灯效时序完结回调来自 LightEffectEngine::UpdateTicks（sleeptimer 中断），
 *       严禁在此直接 nvm3_write；须先转到 FreeRTOS 定时器服务任务。
 */
void LightDecisionCenter::OnFactoryResetWarnSequenceFinishedBridge()
{
    if (xPortIsInsideInterrupt() != pdFALSE)
    {
        BaseType_t       higherPriorityTaskWoken = pdFALSE;
        const BaseType_t posted =
            xTimerPendFunctionCallFromISR(DeferredFactoryResetWarnDispatch, nullptr, 0U, &higherPriorityTaskWoken);
        if (posted == pdPASS)
        {
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
        return;
    }

    LightDecisionCenter::Instance().OnFactoryResetWarnSequenceFinishedRaw();
}

/**
 * @brief 工厂复位重启后灯效：出厂色 65% 快闪×2 → 淡入 100%
 */
void LightDecisionCenter::StartFactoryResetDoneSequence()
{
    if (m_pSequence == nullptr)
    {
        m_sceneState = LightSceneState::Normal;
        ApplyArbitratedResult();
        return;
    }

    LightSequenceScheduler::SequenceStep doneSteps[] = {
        {LightEffectProcessor::GetBlink, {0U, 0U, 0U, 0U}, kFactoryResetWarnBrightness, 400U, 1U},
        {LightEffectProcessor::GetBezier40BytesFactorFadeIn, {0U, 0U, 0U, 0U}, kDefaultBrightness, kTransitionMs, 0U}};
    memcpy(doneSteps[0].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));
    memcpy(doneSteps[1].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_pSequence->RegisterSequenceFinishedCallback(OnFactoryResetDoneSequenceFinishedBridge);
    m_pSequence->StartSequence(doneSteps, 2U, false);
}

/**
 * @brief 复位后开机灯效完结：场景回 Normal，亮度已是出厂默认并上报
 */
void LightDecisionCenter::OnFactoryResetDoneSequenceFinishedRaw()
{
    m_sceneState                 = LightSceneState::Normal;
    m_userTargetParam.brightness = kDefaultBrightness;
    m_lastValidBrightness        = kDefaultBrightness;
    m_brightnessCycleIndex       = 0U;
    ReportToMatterIfRegistered();
}

/**
 * @brief 复位后开机灯效完结桥接
 */
void LightDecisionCenter::OnFactoryResetDoneSequenceFinishedBridge()
{
    LightDecisionCenter::Instance().OnFactoryResetDoneSequenceFinishedRaw();
}

/**
 * @brief Matter 识别时序（一次性）
 * @note
 * Step0: kColorPalette[2]、亮度 100%、周期 800ms 闪 2 次（repeatCount=1）；
 * Step1: Bezier 淡入恢复 m_userTargetParam 亮度与颜色。
 */
void LightDecisionCenter::StartIdentifySequence()
{
    if (m_pSequence == nullptr)
    {
        return;
    }

    LightSequenceScheduler::SequenceStep kIdentifySteps[] = {
        {LightEffectProcessor::GetBlink, {0U, 0U, 0U, 0U}, 255U, 800U, 1U},
        {LightEffectProcessor::GetBezier40BytesFactorFadeIn,
         {0U, 0U, 0U, 0U},
         m_userTargetParam.brightness,
         kTransitionMs,
         0U}};
    memcpy(kIdentifySteps[0].targetChannels, kColorPalette[2], sizeof(kColorPalette[2]));
    memcpy(kIdentifySteps[1].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_pSequence->RegisterSequenceFinishedCallback(OnIdentifySequenceFinishedBridge);
    m_pSequence->StartSequence(kIdentifySteps, 2U, false);
}

/**
 * @brief 识别时序自然完结：退出 MatterIdentifying
 */
void LightDecisionCenter::OnIdentifySequenceFinishedRaw()
{
    if (m_sceneState == LightSceneState::MatterIdentifying)
    {
        m_sceneState = LightSceneState::Normal;
    }
}

/**
 * @brief 识别完结静态桥接
 */
void LightDecisionCenter::OnIdentifySequenceFinishedBridge()
{
    LightDecisionCenter::Instance().OnIdentifySequenceFinishedRaw();
}

/**
 * @brief 配对成功时序（一次性，2 步）
 * @note
 * 颜色取自 m_userTargetParam.wrgb（关灯或其它亮度时仍保持用户当前色）；
 * Step0: 60% 亮度、400ms 周期快闪 2 次（repeatCount=1）；
 * Step1: 同色 Bezier 淡入至 100% 亮度（kTransitionMs）。
 */
void LightDecisionCenter::StartCommissioningSuccessSequence()
{
    if (m_pSequence == nullptr)
    {
        m_isPairSuccessSequenceActive = false;
        return;
    }

    static constexpr uint8_t kPairConfirmBrightness = 153U; /**< 255×60% */

    LightSequenceScheduler::SequenceStep successSteps[] = {
        {LightEffectProcessor::GetBlink, {0U, 0U, 0U, 0U}, kPairConfirmBrightness, 400U, 1U},
        {LightEffectProcessor::GetBezier40BytesFactorFadeIn, {0U, 0U, 0U, 0U}, 255U, kTransitionMs, 0U}};
    memcpy(successSteps[0].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));
    memcpy(successSteps[1].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_pSequence->RegisterSequenceFinishedCallback(OnPairSuccessSequenceFinishedBridge);
    m_pSequence->StartSequence(successSteps, 2U, false);
}

/**
 * @brief 配对成功时序完结：目标亮度归 100% 并持久化、上报
 */
void LightDecisionCenter::OnPairSuccessSequenceFinishedRaw()
{
    m_isPairSuccessSequenceActive = false;
    m_userTargetParam.brightness  = 255U;
    m_userTargetParam.op_id       = LightEffectOpId::Bezier40FadeIn;
    m_lastValidBrightness         = 255U;
    m_brightnessCycleIndex        = 0U;
    SafeSaveToStorage();
    ReportToMatterIfRegistered();
}

/**
 * @brief 配对成功完结静态桥接
 */
void LightDecisionCenter::OnPairSuccessSequenceFinishedBridge()
{
    LightDecisionCenter::Instance().OnPairSuccessSequenceFinishedRaw();
}

/**
 * @brief Matter 控灯后武装短按亮度循环
 * @param brightness Matter 下发的目标亮度
 * @note kBrightnessLevels={255,91,0}，短按先 ++ 再取档：
 *       灯亮时 index=1 → 下次短按到 0%；已关时 index=2 → 下次短按到 100%。
 */
void LightDecisionCenter::ArmBrightnessCycleAfterMatterRaw(uint8_t brightness)
{
    if (brightness > 0U)
    {
        // 网关已开灯：下一次短按 index=1 → 再 ++ → OFF
        m_brightnessCycleIndex = 1U;
    }
    else
    {
        // 网关已关灯：下一次短按 index=2 → 再 ++ → 100%
        m_brightnessCycleIndex = 2U;
    }
}

/**
 * @brief 按持久化亮度恢复短按循环索引
 */
void LightDecisionCenter::SyncBrightnessCycleIndexFromStoredRaw()
{
    if (m_userTargetParam.brightness == 0U)
    {
        m_brightnessCycleIndex = 2U;
    }
    else if (m_userTargetParam.brightness <= kBrightnessLevels[1U])
    {
        m_brightnessCycleIndex = 1U;
    }
    else
    {
        m_brightnessCycleIndex = 0U;
    }
}

/**
 * @brief 亮度循环索引 → 渐变算子与 m_TransitionMs
 * @param brightnessIndex 0=100%，1=35%，2=0%
 * @return 对应 LightEffectOpId
 * @note §1.2：开/调光 200ms LinearLerp；关灯 400ms Bezier 淡出。
 */
LightEffectOpId LightDecisionCenter::BrightnessIndexToOpId(uint8_t brightnessIndex)
{
    LightEffectOpId opId = LightEffectOpId::LinearLerp;
    switch (brightnessIndex)
    {
    case 0U:
        m_TransitionMs = LightDimmingSpec::kFadeInMs;
        opId           = LightEffectOpId::LinearLerp;
        break;
    case 1U:
        m_TransitionMs = LightDimmingSpec::kFadeInMs;
        opId           = LightEffectOpId::LinearLerp;
        break;
    case 2U:
    default:
        m_TransitionMs = LightDimmingSpec::kFadeOutMs;
        opId           = LightEffectOpId::Bezier40FadeOut;
        break;
    }
    return opId;
}

/**
 * @brief FreeRTOS 定时器服务任务回调：在任务上下文上报 Matter
 */
void LightDecisionCenter::DeferredReportDispatch(void* param1, uint32_t param2)
{
    (void)param1;
    (void)param2;
    LightDecisionCenter::Instance().ReportToMatterRaw();
}

/**
 * @brief 实际推送当前 on/亮度/WRGB（仅任务上下文）
 */
void LightDecisionCenter::ReportToMatterRaw()
{
    if (m_matterReporter == nullptr)
    {
        return;
    }

    const bool     isOn    = (m_userTargetParam.brightness > 0U) && !m_isBatteryLow;
    const uint16_t wrgb[4] = {m_userTargetParam.wrgb[0], m_userTargetParam.wrgb[1], m_userTargetParam.wrgb[2],
                              m_userTargetParam.wrgb[3]};
    m_matterReporter(isOn, m_userTargetParam.brightness, wrgb);
}

/**
 * @brief 若已注册 Matter 上报回调，则推送当前 on/亮度/WRGB（ISR 安全）
 * @note 按键在 sleeptimer 中断：ScheduleWork 必须先转到任务上下文。
 */
void LightDecisionCenter::ReportToMatterIfRegistered()
{
    if (m_matterReporter == nullptr)
    {
        return;
    }

    if (xPortIsInsideInterrupt() != pdFALSE)
    {
        BaseType_t       higherPriorityTaskWoken = pdFALSE;
        const BaseType_t posted =
            xTimerPendFunctionCallFromISR(DeferredReportDispatch, nullptr, 0U, &higherPriorityTaskWoken);
        if (posted == pdPASS)
        {
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
        return;
    }

    ReportToMatterRaw();
}
