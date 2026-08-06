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
#include "BlinkTimingSpec.h"
#include "DebugLog.h"
#include "LightDimmingSpec.h"
#include "LightEffectEngine.h"
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

namespace {

/**
 * @brief Color Library 循环调色板（逻辑 WRGB 0~1023）
 * @note 顺序对应 PRD §2：#5,#6,#7,#8,#9,#10,#13,#20,#25,#30,#34,#40,#42
 *       百分比 → 通道值：round(pct × 1023 / 100)
 */
static constexpr uint16_t kColorPalette[][4] = {
    {1023U, 0U, 0U, 0U},     /* #5  100/0/0/0   出厂默认 2700K */
    {409U, 1023U, 0U, 0U},   /* #6  40/100/0/0 */
    {409U, 0U, 235U, 563U},  /* #7  40/0/23/55 */
    {327U, 0U, 235U, 1023U}, /* #8  32/0/23/100 */
    {0U, 1023U, 179U, 0U},   /* #9  0/100/17.5/0 */
    {0U, 1023U, 102U, 0U},   /* #10 0/100/10/0 */
    {0U, 1023U, 0U, 0U},     /* #13 0/100/0/0 */
    {0U, 1023U, 102U, 59U},  /* #20 0/100/10/5.8 */
    {0U, 1023U, 240U, 404U}, /* #25 0/100/23.5/39.5 */
    {0U, 240U, 240U, 1023U}, /* #30 0/23.5/23.5/100 */
    {0U, 522U, 844U, 522U},  /* #34 0/51/82.5/51 */
    {0U, 522U, 1023U, 159U}, /* #40 0/51/100/15.5 */
    {0U, 1023U, 322U, 0U}    /* #42 0/100/31.5/0 */
};

static constexpr uint8_t kColorPaletteCount = static_cast<uint8_t>(sizeof(kColorPalette) / sizeof(kColorPalette[0]));

/** @brief 出厂默认色：#5（调色板索引 0） */
static constexpr uint8_t kDefaultColorIndex = 0U;

/** @brief 颜色切换渐变时长 (ms)，PRD §2.1 */
static constexpr uint16_t kColorTransitionMs = 400U;

/**
 * @brief 在调色板中查找与目标 WRGB 完全匹配的索引
 * @param pWrgb 逻辑 WRGB（4 通道）
 * @param outIndex 匹配成功时写回索引
 * @return true=命中色库；false=非色库色（如 Matter 自定义）
 */
bool FindColorPaletteIndexRaw(const uint16_t* pWrgb, uint8_t* outIndex)
{
    if ((pWrgb == nullptr) || (outIndex == nullptr))
    {
        return false;
    }

    for (uint8_t i = 0U; i < kColorPaletteCount; ++i)
    {
        if ((pWrgb[0] == kColorPalette[i][0]) && (pWrgb[1] == kColorPalette[i][1]) &&
            (pWrgb[2] == kColorPalette[i][2]) && (pWrgb[3] == kColorPalette[i][3]))
        {
            *outIndex = i;
            return true;
        }
    }

    return false;
}

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

    m_pSequence                  = pSequence;
    m_pStorage                   = pStorage;
    m_sceneState                 = LightSceneState::Normal;
    m_isBatteryLow               = false;
    m_criticalOpenSequenceActive = false;

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
    SyncColorCycleIndexFromStoredRaw();

    // 工厂复位前写入的开机灯效标记：播完后清标记；灯光参数已是出厂默认
    if ((m_userTargetParam.reserved & kBootEffectMask) == kBootEffectFactoryResetDone)
    {
        m_userTargetParam.reserved =
            static_cast<uint8_t>(m_userTargetParam.reserved & static_cast<uint8_t>(~kBootEffectMask));
        SafeSaveToStorage();
        m_sceneState = LightSceneState::NetConfiguring;
        StartFactoryResetDoneSequence();
        return;
    }

    ApplyArbitratedResult();
}

/**
 * @brief 探测 NVM 是否标记工厂复位后开机灯效（只读，无副作用）
 */
bool LightDecisionCenter::ProbeFactoryResetBootEffect(LightStorageProvider* pStorage)
{
    if (pStorage == nullptr)
    {
        return false;
    }

    PersistParam_T param{};
    const bool readOk = pStorage->Read(reinterpret_cast<uint8_t*>(&param), sizeof(PersistParam_T));
    if (!readOk || !Instance().IsPersistValid(param))
    {
        return false;
    }

    return ((param.reserved & kBootEffectMask) == kBootEffectFactoryResetDone);
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

void LightDecisionCenter::RegisterFabricJoinedQuery(FabricJoinedQuery query)
{
    m_fabricJoinedQuery = query;
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

void LightDecisionCenter::TryOpenCommissioningRaw()
{
    if ((m_fabricJoinedQuery != nullptr) && m_fabricJoinedQuery())
    {
        return;
    }
    InvokeNetControlRaw(NetControlAction::OpenCommissioning);
}

/**
 * @brief 处理按键语义事件
 * @param event KeyEventType 事件类型
 * @note 临界电量：仅响应「尝试开灯」短按（播 §6.2 时序）；其它键丢弃。
 */
void LightDecisionCenter::ProcessKeyEvent(KeyEventType event)
{
    if (m_isBatteryLow)
    {
        if ((event == KeyEventType::ShortPressCycleBrightness) && !m_criticalOpenSequenceActive)
        {
            // §6.2：临界下尝试开灯 → 主灯四段演示；红闪仅开灯边沿（Notify 内判定）
            NotifyBatteryWarnIfNeeded(1U);
            StartCriticalBatteryOpenSequence();
        }
        return;
    }

    // 配对成功确认灯效播放中：按键不打断（清网长按除外，仍允许武装复位）
    if (m_isPairSuccessSequenceActive && (event != KeyEventType::LongPressClearNetLighting) &&
        (event != KeyEventType::LongPressClearNet) && (event != KeyEventType::LongPressStopNet))
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
        // §14/§15：未入网时短按灯键仍走亮度循环，并打开/刷新 15min 配网倒计时
        TryOpenCommissioningRaw();
        break;
    }
    case KeyEventType::ShortPressOpenCommissioning: {
        // §15：未入网时短按系统键手动进入配网；§14 已配网中则刷新倒计时
        TryOpenCommissioningRaw();
        break;
    }
    case KeyEventType::DoublePressCycleColor: {
        // §2.1：仅开灯态允许双击切色
        if (m_userTargetParam.brightness == 0U)
        {
            LOG_LIGHT_DC("DoublePress ignored: light OFF");
            break;
        }

        m_colorCycleIndex++;
        if (m_colorCycleIndex >= kColorPaletteCount)
        {
            m_colorCycleIndex = 0U;
        }

        memcpy(m_userTargetParam.wrgb, kColorPalette[m_colorCycleIndex], sizeof(m_userTargetParam.wrgb));
        m_userTargetParam.op_id = LightEffectOpId::LinearLerp;
        m_sceneState            = LightSceneState::Normal;
        m_TransitionMs          = kColorTransitionMs;
        ApplyArbitratedResult();
        SafeSaveToStorage();
        ReportToMatterIfRegistered();
        break;
    }
    case KeyEventType::LongPressClearNetLighting: {
        // 约 8s：仅播预警灯效；是否复位看后续是否按住满 13s
        m_factoryResetArmed = false;
        m_sceneState        = LightSceneState::NetConfiguring;
        StartNetConfigSequence();
        break;
    }
    case KeyEventType::LongPressClearNet: {
        // 约 13s：武装复位；实际擦除仍等预警时序播完
        if (m_sceneState == LightSceneState::NetConfiguring)
        {
            m_factoryResetArmed = true;
            LOG_LIGHT_DC("FactoryReset armed (>=13s hold)");
        }
        break;
    }
    case KeyEventType::LongPressStopNet: {
        // 未满 13s 松开：取消预警（≥13s 松开由按键层不发本事件）
        if (m_sceneState == LightSceneState::NetConfiguring)
        {
            m_factoryResetArmed = false;
            m_sceneState        = LightSceneState::Normal;
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
 * @note NetConfiguring / MatterIdentifying / 配对成功灯效期间仅更新参数，不 Apply。
 */
void LightDecisionCenter::ProcessMatterCommand(const uint16_t* pWrgbBuffer, uint8_t brightness, LightEffectOpId opId)
{
    if (pWrgbBuffer == nullptr)
    {
        return;
    }

    NotifyBatteryWarnIfNeeded(brightness);
    if (m_isBatteryLow)
    {
        if (brightness > 0U)
        {
            // §6.2：Hub/Matter 尝试开灯 → 临界演示；红闪仅开灯边沿
            if (!m_criticalOpenSequenceActive)
            {
                StartCriticalBatteryOpenSequence();
            }
            return;
        }

        // 强控下仅接受关灯，保持 LowBattery，不点亮
        memcpy(m_userTargetParam.wrgb, pWrgbBuffer, sizeof(m_userTargetParam.wrgb));
        m_userTargetParam.brightness = 0U;
        m_userTargetParam.op_id      = opId;
        ReportToMatterIfRegistered();
        return;
    }

    const bool wasOn =
        (m_userTargetParam.brightness > 0U);
    const bool sameWrgb =
        (memcmp(m_userTargetParam.wrgb, pWrgbBuffer, sizeof(m_userTargetParam.wrgb)) == 0);

    memcpy(m_userTargetParam.wrgb, pWrgbBuffer, sizeof(m_userTargetParam.wrgb));
    m_userTargetParam.brightness = brightness;
    m_userTargetParam.op_id      = opId;

    // Matter 已开灯且仅调亮度：1ms 跟手；开/关/改色仍用 200/400ms
    if ((brightness > 0U) && wasOn && sameWrgb && (m_sceneState == LightSceneState::Normal) &&
        !m_isPairSuccessSequenceActive)
    {
        m_TransitionMs = LightDimmingSpec::kMatterLevelOnlyTransitionMs;
    }
    else
    {
        m_TransitionMs =
            (brightness > 0U) ? LightDimmingSpec::kFadeInMs : LightDimmingSpec::kFadeOutMs;
    }

    // Matter 改色后尽量对齐色库索引，便于后续双击从下一色继续
    SyncColorCycleIndexFromStoredRaw();

    // Matter 开灯/调光/改色/色温后：下一次短按直接关灯；Matter 关灯后下次短按 100%
    ArmBrightnessCycleAfterMatterRaw(brightness);

    if (brightness > 0U)
    {
        m_lastValidBrightness = brightness;
    }

    // 配对成功确认灯效进行中：只记参数，禁止 StartSingleEffect 打断快闪×2
    if ((m_sceneState == LightSceneState::Normal) && !m_isPairSuccessSequenceActive)
    {
        ApplyArbitratedResult();
    }

    // 记忆灯光：用户目标（含关灯 brightness=0）落盘，供下次上电恢复
    SafeSaveToStorage();

    // §3.1 / §14：未入网且灯被开启时自动进入配网（刷新倒计时）
    if ((brightness > 0U) && !m_isPairSuccessSequenceActive)
    {
        TryOpenCommissioningRaw();
    }
}

/**
 * @brief 处理极低电量强控
 * @param isLow true=进入临界强控；false=电压恢复后解除
 * @note §6.2 已亮态跌入临界：播四段演示后强制关灯；未亮则立即关并上报。
 */
void LightDecisionCenter::ProcessBatteryEvent(bool isLow)
{
    if (isLow)
    {
        if (m_isBatteryLow)
        {
            return;
        }

        const bool wasLightOn = (m_userTargetParam.brightness > 0U);
        m_isBatteryLow                = true;
        m_sceneState                  = LightSceneState::LowBattery;
        m_isPairSuccessSequenceActive = false;

        if (wasLightOn)
        {
            if (m_pSequence != nullptr)
            {
                m_pSequence->StopSequence();
            }
            LOG_LIGHT_DC("Critical enter while ON -> play fade sequence");
            StartCriticalBatteryOpenSequence();
            return;
        }

        m_criticalOpenSequenceActive = false;
        if (m_pSequence != nullptr)
        {
            m_pSequence->StopSequence();
        }
        m_userTargetParam.brightness = 0U;
        ReportToMatterIfRegistered();
        return;
    }

    if (!m_isBatteryLow)
    {
        return;
    }

    m_criticalOpenSequenceActive = false;
    m_isBatteryLow               = false;
    m_sceneState                 = LightSceneState::Normal;
    m_userTargetParam.brightness = m_lastValidBrightness;
    SafeSaveToStorage();
    ApplyArbitratedResult();
    ReportToMatterIfRegistered();
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
 * @brief §6.2 临界开灯演示：亮起→渐灭→渐亮→渐灭（亮度用关灯前记忆色）
 */
void LightDecisionCenter::StartCriticalBatteryOpenSequence()
{
    if ((m_pSequence == nullptr) || !m_isBatteryLow)
    {
        return;
    }

    uint8_t peakBrightness = m_lastValidBrightness;
    if (peakBrightness == 0U)
    {
        peakBrightness = kDefaultBrightness;
    }

    LightSequenceScheduler::SequenceStep steps[] = {
        {LightEffectProcessor::GetBezier40BytesFactorFadeIn,
         {0U, 0U, 0U, 0U},
         peakBrightness,
         LightDimmingSpec::kFadeInMs,
         0U},
        {LightEffectProcessor::GetBezier40BytesFactorFadeOut, {0U, 0U, 0U, 0U}, 0U, LightDimmingSpec::kFadeOutMs, 0U},
        {LightEffectProcessor::GetBezier40BytesFactorFadeIn,
         {0U, 0U, 0U, 0U},
         peakBrightness,
         LightDimmingSpec::kFadeInMs,
         0U},
        {LightEffectProcessor::GetBezier40BytesFactorFadeOut, {0U, 0U, 0U, 0U}, 0U, LightDimmingSpec::kFadeOutMs, 0U}};
    memcpy(steps[0].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));
    memcpy(steps[1].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));
    memcpy(steps[2].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));
    memcpy(steps[3].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_criticalOpenSequenceActive = true;
    m_pSequence->RegisterSequenceFinishedCallback(OnCriticalBatteryOpenSequenceFinishedBridge);
    m_pSequence->StartSequence(steps, 4U, false);
    LOG_LIGHT_DC("Critical battery open sequence start, peak=%u", peakBrightness);
}

/**
 * @brief 临界开灯时序完结：维持强控关灯并上报 Matter
 */
void LightDecisionCenter::OnCriticalBatteryOpenSequenceFinishedRaw()
{
    m_criticalOpenSequenceActive = false;
    m_userTargetParam.brightness = 0U;
    if (m_sceneState != LightSceneState::LowBattery)
    {
        m_sceneState = LightSceneState::LowBattery;
    }
    ReportToMatterIfRegistered();
    LOG_LIGHT_DC("Critical battery open sequence done");
}

/**
 * @brief 临界开灯时序完结桥接
 */
void LightDecisionCenter::OnCriticalBatteryOpenSequenceFinishedBridge()
{
    LightDecisionCenter::Instance().OnCriticalBatteryOpenSequenceFinishedRaw();
}

/**
 * @brief 关灯→开灯边沿时触发系统 LED 红灯快闪×2（§6.1 低电 / §6.2 临界）
 * @param targetBrightness 目标亮度，0 时不触发
 * @note 须在更新 m_userTargetParam.brightness 之前调用，以便识别开灯边沿。
 *       已亮态下调光/改色不再重复播红闪。
 */
void LightDecisionCenter::NotifyBatteryWarnIfNeeded(uint8_t targetBrightness)
{
    if (targetBrightness == 0U)
    {
        return;
    }

    if (m_userTargetParam.brightness > 0U)
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
 * @note 每次 kCommissioningComplete 都重播确认灯效（栈可能投递多次，有几次播几次）。
 *       播放期间禁止 Matter Level/Identify 经 Apply 打断（否则只剩“闪一下”）。
 */
void LightDecisionCenter::ProcessMatterCommissioningComplete()
{
    if (m_isBatteryLow)
    {
        return;
    }

    m_isPairSuccessSequenceActive = true;
    m_sceneState                  = LightSceneState::Normal;

    MarkFirstCommissionDone();

    if (m_pSequence != nullptr)
    {
        m_pSequence->StopSequence();
    }
    LOG_LIGHT_DC("PairSuccess: start 60%% blink x2 then fade full");
    StartCommissioningSuccessSequence();
}

/**
 * @brief Matter 识别控制
 * @param active true=进入 #5 持续闪；false=停止并渐变恢复用户目标
 */
void LightDecisionCenter::ProcessMatterIdentify(bool active)
{
    if (m_isBatteryLow)
    {
        return;
    }

    // 配对成功确认优先于 Identify，避免配网尾声 Identify 边沿冲掉快闪×2
    if (m_isPairSuccessSequenceActive)
    {
        LOG_LIGHT_DC("Identify ignored during PairSuccess");
        return;
    }

    if (active)
    {
        m_sceneState = LightSceneState::MatterIdentifying;
        StartIdentifySequence();
    }
    else if (m_sceneState == LightSceneState::MatterIdentifying)
    {
        RestoreAfterIdentifyRaw();
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

bool LightDecisionCenter::HasCompletedFirstCommission() const
{
    return (m_userTargetParam.reserved & kFirstCommissionDoneFlag) != 0U;
}

void LightDecisionCenter::MarkFirstCommissionDone()
{
    if (HasCompletedFirstCommission())
    {
        return;
    }

    m_userTargetParam.reserved = static_cast<uint8_t>(m_userTargetParam.reserved | kFirstCommissionDoneFlag);
    SafeSaveToStorage();
    LOG_LIGHT_DC("First commission marked done");
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
 * @brief 注解10：拔 USB 切电池前先强制熄灭主灯，避免 BAT_EN 切换时 PWM 冲突闪一下
 */
void LightDecisionCenter::PrepareUsbUnplugLightOffRaw()
{
    if (m_pSequence != nullptr)
    {
        m_pSequence->StopSequence(true);
    }
    else
    {
        LightEffectEngine::Instance().StopCurrentEffect(true);
    }
}

/**
 * @brief 注解10：BAT_EN 就绪后从 0 淡入 400ms 至目标亮度/WRGB
 */
void LightDecisionCenter::StartUsbUnplugRestoreSequence()
{
    if (m_pSequence == nullptr)
    {
        return;
    }

    if (m_sceneState != LightSceneState::Normal)
    {
        return;
    }

    if (m_isBatteryLow || m_isPairSuccessSequenceActive)
    {
        return;
    }

    if (m_userTargetParam.brightness == 0U)
    {
        return;
    }

    LightSequenceScheduler::SequenceStep fadeInStep = {LightEffectProcessor::GetBezier40BytesFactorFadeIn,
                                                       {0U, 0U, 0U, 0U},
                                                       m_userTargetParam.brightness,
                                                       LightDimmingSpec::kUsbUnplugFadeInMs,
                                                       0U};
    memcpy(fadeInStep.targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_pSequence->RegisterSequenceFinishedCallback(nullptr);
    m_pSequence->StartSequence(&fadeInStep, 1U, false);
    LOG_LIGHT_DC("USB unplug restore: fade in %u ms", static_cast<unsigned>(LightDimmingSpec::kUsbUnplugFadeInMs));
}

/**
 * @brief 供电通路就绪后重下发灯效
 * @param usbUnplugFadeIn true=注解10：先灭后 400ms 淡入
 */
void LightDecisionCenter::RefreshOutputIfAllowed(bool usbUnplugFadeIn)
{
    if (usbUnplugFadeIn)
    {
        StartUsbUnplugRestoreSequence();
        return;
    }

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

    if (m_isPairSuccessSequenceActive)
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
 * @note 亮度 100%、色库 #5（2700K 暖白）、LinearLerp 算子。
 */
void LightDecisionCenter::LoadDefaults()
{
    m_userTargetParam.magic      = kPersistMagic;
    m_userTargetParam.brightness = kDefaultBrightness;
    m_userTargetParam.op_id      = LightEffectOpId::LinearLerp;
    memcpy(m_userTargetParam.wrgb, kColorPalette[kDefaultColorIndex], sizeof(m_userTargetParam.wrgb));
    m_userTargetParam.reserved = kBootEffectNone;
    m_lastValidBrightness      = kDefaultBrightness;
    m_brightnessCycleIndex     = 0U;
    m_colorCycleIndex          = kDefaultColorIndex;
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
 * @brief 主灯工厂重置预警时序（一次性，3 步）
 * @note 注解：熄灭 400ms → 正常闪×3 → 慢闪×1 → 熄灭（慢闪结束即灭）；
 *       Matter 释放改由 MatterBridge 主动关窗/解 Fail-Safe，无需末尾被动等待 2s；
 *       闪烁亮度=当前亮度，关灯则用关灯前亮度（m_lastValidBrightness）；
 *       完结后仅当已按住≥13s（m_factoryResetArmed）才触发工厂复位。
 */
void LightDecisionCenter::StartNetConfigSequence()
{
    if (m_pSequence == nullptr)
    {
        return;
    }

    // 当前亮度；关灯态回退到关灯前非零亮度
    uint8_t warnBrightness = m_userTargetParam.brightness;
    if (warnBrightness == 0U)
    {
        warnBrightness = m_lastValidBrightness;
    }
    if (warnBrightness == 0U)
    {
        warnBrightness = kDefaultBrightness;
    }

    LightSequenceScheduler::SequenceStep kNetConfigSteps[] = {
        {LightEffectProcessor::GetKeep, {0U, 0U, 0U, 0U}, 0U, BlinkTimingSpec::kResetOffLeadMs, 0U},
        {LightEffectProcessor::GetBlink,
         {0U, 0U, 0U, 0U},
         warnBrightness,
         BlinkTimingSpec::kNormalBlinkCycleMs,
         BlinkTimingSpec::kResetNormalBlinkExtraRepeats},
        {LightEffectProcessor::GetBlink, {0U, 0U, 0U, 0U}, warnBrightness, BlinkTimingSpec::kSlowBlinkCycleMs, 0U}};
    memcpy(kNetConfigSteps[1].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));
    memcpy(kNetConfigSteps[2].targetChannels, m_userTargetParam.wrgb, sizeof(m_userTargetParam.wrgb));

    m_pSequence->RegisterSequenceFinishedCallback(OnFactoryResetWarnSequenceFinishedBridge);
    m_pSequence->StartSequence(kNetConfigSteps, 3U, false);
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
 * @brief 重置预警完结：已武装则落盘并工厂复位；未武装则回到常态
 * @note 仅任务上下文调用（NVM / Matter 不可在 sleeptimer 中断执行）。
 */
void LightDecisionCenter::OnFactoryResetWarnSequenceFinishedRaw()
{
    const bool doReset  = m_factoryResetArmed;
    m_factoryResetArmed = false;
    m_sceneState        = LightSceneState::Normal;

    if (!doReset)
    {
        // 时序播完但未按住满 13s（或中途已取消）：不擦除，恢复用户目标
        LOG_LIGHT_DC("FactoryReset warn done, not armed -> skip reset");
        ApplyArbitratedResult();
        return;
    }

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
 * @note 淡入用 kTransitionMs(400)，缩短到 Normal 的等待。
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
        {LightEffectProcessor::GetBlink,
         {0U, 0U, 0U, 0U},
         kFactoryResetWarnBrightness,
         BlinkTimingSpec::kFastBlinkCycleMs,
         1U},
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
    // 配网窗由 MatterBridge::Init / kServerReady 兜底打开；此处不再 force 刷新，
    // 避免灯效结束时 CHIP 队列正忙导致 ScheduleWork 失败刷屏。
}

/**
 * @brief 复位后开机灯效完结桥接
 */
void LightDecisionCenter::OnFactoryResetDoneSequenceFinishedBridge()
{
    LightDecisionCenter::Instance().OnFactoryResetDoneSequenceFinishedRaw();
}

/**
 * @brief Matter 识别时序：默认色 #5、100% 亮度、正常闪，循环至 Identify 结束
 */
void LightDecisionCenter::StartIdentifySequence()
{
    if (m_pSequence == nullptr)
    {
        return;
    }

    LightSequenceScheduler::SequenceStep kIdentifySteps[] = {{LightEffectProcessor::GetBlink,
                                                              {0U, 0U, 0U, 0U},
                                                              kDefaultBrightness,
                                                              BlinkTimingSpec::kNormalBlinkCycleMs,
                                                              0U}};
    memcpy(kIdentifySteps[0].targetChannels, kColorPalette[kDefaultColorIndex],
           sizeof(kColorPalette[kDefaultColorIndex]));

    // 循环闪烁直至 ProcessMatterIdentify(false) 停止
    m_pSequence->RegisterSequenceFinishedCallback(nullptr);
    m_pSequence->StartSequence(kIdentifySteps, 1U, true);
}

/**
 * @brief 识别结束：保留当前 PWM，渐变回 m_userTargetParam（亮度/颜色）
 */
void LightDecisionCenter::RestoreAfterIdentifyRaw()
{
    m_sceneState = LightSceneState::Normal;

    if (m_pSequence == nullptr)
    {
        return;
    }

    // 不清零硬件，避免识别结束瞬间灭灯再淡入
    m_pSequence->StopSequence(false);

    const LightEffectOpId savedOpId = m_userTargetParam.op_id;
    if (m_userTargetParam.brightness > 0U)
    {
        m_userTargetParam.op_id = LightEffectOpId::Bezier40FadeIn;
        m_TransitionMs          = LightDimmingSpec::kFadeInMs;
    }
    else
    {
        m_userTargetParam.op_id = LightEffectOpId::Bezier40FadeOut;
        m_TransitionMs          = LightDimmingSpec::kFadeOutMs;
    }

    ApplyArbitratedResult();
    m_userTargetParam.op_id = savedOpId;
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
        {LightEffectProcessor::GetBlink,
         {0U, 0U, 0U, 0U},
         kPairConfirmBrightness,
         BlinkTimingSpec::kFastBlinkCycleMs,
         1U},
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
 * @brief 按持久化 WRGB 恢复双击色库索引
 * @note §2.2 颜色记忆落盘的是 wrgb；开机/Matter 改色后由此对齐循环位置。
 *       非色库颜色（APP 自定义）时保持当前索引，下次双击切到 index+1。
 */
void LightDecisionCenter::SyncColorCycleIndexFromStoredRaw()
{
    uint16_t wrgbCopy[4];
    memcpy(wrgbCopy, m_userTargetParam.wrgb, sizeof(wrgbCopy));

    uint8_t matchedIndex = 0U;
    if (FindColorPaletteIndexRaw(wrgbCopy, &matchedIndex))
    {
        m_colorCycleIndex = matchedIndex;
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
