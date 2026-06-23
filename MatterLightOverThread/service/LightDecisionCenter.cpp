/**
 * @file LightDecisionCenter.cpp
 * @brief 主灯决策中心实现
 * @author hady
 * @date 2026-06-19
 * @layer Service
 * @note
 * 多源仲裁优先级：极低电量 > 配网/识别时序 > 常规用户/Matter 控制。
 * 每次亮度/WRGB 变更经 SafeSaveToStorage 防抖写入 NVM。
 */
#include "LightDecisionCenter.h"
#include "DebugLog.h"
#include <cstring>

namespace {

static constexpr uint16_t kColorPalette[][4] = {
    {1023U, 0U, 0U, 0U},     {409U, 1023U, 0U, 0U},  {409U, 0U, 235U, 563U},
    {327U, 0U, 235U, 1023U}, {0U, 1023U, 179U, 0U},  {0U, 1023U, 102U, 0U},
    {0U, 1023U, 0U, 0U},     {0U, 1023U, 102U, 59U}, {0U, 1023U, 240U, 404U},
    {0U, 240U, 240U, 1023U}, {0U, 522U, 844U, 522U}, {0U, 522U, 1023U, 159U},
    {0U, 1023U, 322U, 0U}};

static constexpr uint8_t kColorPaletteCount =
    static_cast<uint8_t>(sizeof(kColorPalette) / sizeof(kColorPalette[0]));

} // namespace

/** @brief 算子 ID → LightEffectProcessor 函数指针映射表（常驻 Flash） */
const LightDecisionCenter::EffectRenderAction
    LightDecisionCenter::kActionTable[] = {
        [static_cast<uint8_t>(LightEffectOpId::DirectKeep)] =
            &LightEffectProcessor::GetKeep,
        [static_cast<uint8_t>(LightEffectOpId::LinearLerp)] =
            &LightEffectProcessor::GetLerp,
        [static_cast<uint8_t>(LightEffectOpId::Breath80Bytes)] =
            &LightEffectProcessor::CalcBreath80BytesFactor,
        [static_cast<uint8_t>(LightEffectOpId::Bezier80FadeIn)] =
            &LightEffectProcessor::GetBezier80BytesFactorFadeIn,
        [static_cast<uint8_t>(LightEffectOpId::Bezier40FadeIn)] =
            &LightEffectProcessor::GetBezier40BytesFactorFadeIn,
        [static_cast<uint8_t>(LightEffectOpId::Bezier80FadeOut)] =
            &LightEffectProcessor::GetBezier80BytesFactorFadeOut,
        [static_cast<uint8_t>(LightEffectOpId::Bezier40FadeOut)] =
            &LightEffectProcessor::GetBezier40BytesFactorFadeOut,
        [static_cast<uint8_t>(LightEffectOpId::Blink)] =
            &LightEffectProcessor::GetBlink};

/**
 * @brief 初始化决策中心并冷启动回读 NVM
 * @param pSequence 灯效调度器指针
 * @param pStorage  持久化适配器指针
 * @return 无
 */
void LightDecisionCenter::Init(LightSequenceScheduler *pSequence,
                               LightStorageProvider *pStorage) {
  if ((pSequence == nullptr) || (pStorage == nullptr)) {
    return;
  }

  m_pSequence = pSequence;
  m_pStorage = pStorage;
  m_sceneState = LightSceneState::Normal;
  m_isBatteryLow = false;

  const bool readOk = m_pStorage->Read(
      reinterpret_cast<uint8_t *>(&m_userTargetParam), sizeof(PersistParam_T));
  if (!readOk || !IsPersistValid(m_userTargetParam)) {
    LoadDefaults();
    SafeSaveToStorage();
  }

  if (m_userTargetParam.brightness > 0U) {
    m_lastValidBrightness = m_userTargetParam.brightness;
  }

  ApplyArbitratedResult();
}

void LightDecisionCenter::RegisterMatterReporter(
    MatterReportCallback callback) {
  m_matterReporter = callback;
}

void LightDecisionCenter::RegisterNetControlCallback(
    NetControlCallback callback) {
  m_netControl = callback;
}

void LightDecisionCenter::InvokeNetControlRaw(NetControlAction action) {
  if (m_netControl != nullptr) {
    m_netControl(action);
  }
}

/**
 * @brief 处理按键语义事件
 * @param event KeyEventType 事件类型
 * @return 无
 * @note 低电量时直接丢弃；变更后保存 NVM 并可选上报 Matter。
 */
void LightDecisionCenter::ProcessKeyEvent(KeyEventType event) {
  if (m_isBatteryLow) {
    return;
  }

  switch (event) {
  case KeyEventType::ShortPressCycleBrightness: {
    const uint8_t brightness = kBrightnessLevels[m_brightnessCycleIndex % 3U];
    m_userTargetParam.brightness = brightness;
    m_userTargetParam.op_id =
        BrightnessIndexToOpId(m_brightnessCycleIndex % 3U);
    m_sceneState = LightSceneState::Normal;

    if (brightness > 0U) {
      m_lastValidBrightness = brightness;
    }

    m_brightnessCycleIndex++;
    if (m_brightnessCycleIndex >= 3U) {
      m_brightnessCycleIndex = 0U;
    }

    SafeSaveToStorage();
    ApplyArbitratedResult();
    ReportToMatterIfRegistered();
    break;
  }

  case KeyEventType::DoublePressCycleColor: {
    m_colorCycleIndex++;
    if (m_colorCycleIndex >= kColorPaletteCount) {
      m_colorCycleIndex = 0U;
    }

    memcpy(m_userTargetParam.wrgb, kColorPalette[m_colorCycleIndex],
           sizeof(m_userTargetParam.wrgb));
    m_userTargetParam.op_id = LightEffectOpId::LinearLerp;
    m_sceneState = LightSceneState::Normal;

    SafeSaveToStorage();
    ApplyArbitratedResult();
    ReportToMatterIfRegistered();
    break;
  }

  case KeyEventType::LongPressClearNet: {
    m_sceneState = LightSceneState::NetConfiguring;
    InvokeNetControlRaw(NetControlAction::ClearAndOpen);
    StartNetConfigSequence();
    break;
  }

  case KeyEventType::LongPressStopNet: {
    if (m_sceneState == LightSceneState::NetConfiguring) {
      m_sceneState = LightSceneState::Normal;
      InvokeNetControlRaw(NetControlAction::Close);
      m_pSequence->StopSequence();
      LoadDefaults();
      SafeSaveToStorage();
      ApplyArbitratedResult();
      ReportToMatterIfRegistered();
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
 * @return 无
 */
void LightDecisionCenter::ProcessMatterCommand(const uint16_t *pWrgbBuffer,
                                               uint8_t brightness,
                                               LightEffectOpId opId) {
  if (pWrgbBuffer == nullptr) {
    return;
  }

  if (m_isBatteryLow && (brightness > 0U)) {
    return;
  }

  memcpy(m_userTargetParam.wrgb, pWrgbBuffer, sizeof(m_userTargetParam.wrgb));
  m_userTargetParam.brightness = brightness;
  m_userTargetParam.op_id = opId;

  if (brightness > 0U) {
    m_lastValidBrightness = brightness;
  }

  SafeSaveToStorage();

  if ((m_sceneState == LightSceneState::Normal) ||
      (m_sceneState == LightSceneState::LowBattery)) {
    m_sceneState = LightSceneState::Normal;
    ApplyArbitratedResult();
  }
}

/**
 * @brief 处理极低电量事件
 * @param isLow true=进入强控灭灯；false=恢复上次亮度
 * @return 无
 */
void LightDecisionCenter::ProcessBatteryEvent(bool isLow) {
  m_isBatteryLow = isLow;

  if (m_isBatteryLow) {
    m_sceneState = LightSceneState::LowBattery;
    m_pSequence->StopSequence();
    m_userTargetParam.brightness = 0U;
    ReportToMatterIfRegistered();
  } else if (m_sceneState == LightSceneState::LowBattery) {
    m_sceneState = LightSceneState::Normal;
    m_userTargetParam.brightness = m_lastValidBrightness;
    SafeSaveToStorage();
    ApplyArbitratedResult();
    ReportToMatterIfRegistered();
  }
}

void LightDecisionCenter::ProcessMatterCommissioningComplete() {
  if (m_isBatteryLow) {
    return;
  }

  m_sceneState = LightSceneState::Normal;
  StartCommissioningSuccessSequence();
  ReportStateToMatter();
}

void LightDecisionCenter::ProcessMatterIdentify(bool active) {
  if (m_isBatteryLow) {
    return;
  }

  if (active) {
    m_sceneState = LightSceneState::MatterIdentifying;
    StartIdentifySequence();
  } else if (m_sceneState == LightSceneState::MatterIdentifying) {
    m_sceneState = LightSceneState::Normal;
    m_pSequence->StopSequence();
    ApplyArbitratedResult();
  }
}

void LightDecisionCenter::ReportStateToMatter() {
  ReportToMatterIfRegistered();
}

uint8_t LightDecisionCenter::GetCurrentBrightness() const {
  return m_userTargetParam.brightness;
}

void LightDecisionCenter::GetCurrentWrgb(uint16_t *outChannels,
                                         uint8_t count) const {
  if (outChannels == nullptr) {
    return;
  }

  const uint8_t copyCount = (count > 4U) ? 4U : count;
  memcpy(outChannels, m_userTargetParam.wrgb,
         static_cast<size_t>(copyCount) * sizeof(uint16_t));
}

/**
 * @brief 仲裁结果下发至 LightSequenceScheduler
 * @return 无
 * @note 仅在 Normal 场景且非低电量时渲染。
 */
void LightDecisionCenter::ApplyArbitratedResult() {
  if (m_sceneState != LightSceneState::Normal) {
    return;
  }

  if (m_isBatteryLow) {
    return;
  }

  uint8_t opIndex = static_cast<uint8_t>(m_userTargetParam.op_id);
  if (opIndex >= static_cast<uint8_t>(LightEffectOpId::MaxOperators)) {
    opIndex = static_cast<uint8_t>(LightEffectOpId::LinearLerp);
  }

  EffectRenderAction pAction = kActionTable[opIndex];
  if (pAction == nullptr) {
    return;
  }

  m_pSequence->StartSingleEffect(pAction, m_userTargetParam.wrgb,
                                 m_userTargetParam.brightness, kTransitionMs);
}

/**
 * @brief 防抖写入 NVM（memcmp 无变化则跳过擦写）
 * @return 无
 */
void LightDecisionCenter::SafeSaveToStorage() {
  if (m_pStorage == nullptr) {
    return;
  }

  static PersistParam_T lastSavedParam = {0};
  m_userTargetParam.magic = kPersistMagic;

  if (memcmp(&m_userTargetParam, &lastSavedParam, sizeof(PersistParam_T)) !=
      0) {
    if (m_pStorage->Write(reinterpret_cast<const uint8_t *>(&m_userTargetParam),
                          sizeof(PersistParam_T))) {
      lastSavedParam = m_userTargetParam;
    }
  }
}

void LightDecisionCenter::LoadDefaults() {
  m_userTargetParam.magic = kPersistMagic;
  m_userTargetParam.brightness = kDefaultBrightness;
  m_userTargetParam.op_id = LightEffectOpId::LinearLerp;
  m_userTargetParam.wrgb[0] = 1023U;
  m_userTargetParam.wrgb[1] = 0U;
  m_userTargetParam.wrgb[2] = 0U;
  m_userTargetParam.wrgb[3] = 0U;
  m_userTargetParam.reserved = 0U;
  m_lastValidBrightness = kDefaultBrightness;
  m_brightnessCycleIndex = 0U;
  m_colorCycleIndex = 0U;
}

bool LightDecisionCenter::IsPersistValid(const PersistParam_T &param) const {
  if (param.magic != kPersistMagic) {
    return false;
  }

  const uint8_t opIndex = static_cast<uint8_t>(param.op_id);
  return opIndex < static_cast<uint8_t>(LightEffectOpId::MaxOperators);
}

void LightDecisionCenter::StartNetConfigSequence() {
  static const LightSequenceScheduler::SequenceStep kNetConfigSteps[] = {
      {LightEffectProcessor::CalcBreath80BytesFactor,
       {1023U, 0U, 0U, 0U},
       255U,
       1600U,
       0U},
      {LightEffectProcessor::GetBlink, {1023U, 0U, 0U, 0U}, 255U, 800U, 0U}};

  m_pSequence->StartSequence(kNetConfigSteps, 2U, true);
}

void LightDecisionCenter::StartIdentifySequence() {
  static const LightSequenceScheduler::SequenceStep kIdentifySteps[] = {
      {LightEffectProcessor::CalcBreath80BytesFactor,
       {0U, 1023U, 0U, 0U},
       255U,
       1600U,
       0U},
      {LightEffectProcessor::CalcBreath80BytesFactor,
       {0U, 0U, 1023U, 0U},
       255U,
       1600U,
       0U}};

  m_pSequence->StartSequence(kIdentifySteps, 2U, true);
}

void LightDecisionCenter::StartCommissioningSuccessSequence() {
  static const LightSequenceScheduler::SequenceStep kSuccessSteps[] = {
      {LightEffectProcessor::GetBezier40BytesFactorFadeIn,
       {1023U, 0U, 0U, 0U},
       255U,
       800U,
       0U},
      {LightEffectProcessor::GetBezier40BytesFactorFadeOut,
       {1023U, 0U, 0U, 0U},
       255U,
       800U,
       0U}};

  m_pSequence->StartSequence(kSuccessSteps, 2U, false);
}

LightEffectOpId
LightDecisionCenter::BrightnessIndexToOpId(uint8_t brightnessIndex) const {
  switch (brightnessIndex) {
  case 0U:
    return LightEffectOpId::Bezier40FadeIn;
  case 1U:
    return LightEffectOpId::LinearLerp;
  case 2U:
  default:
    return LightEffectOpId::Bezier40FadeOut;
  }
}

void LightDecisionCenter::ReportToMatterIfRegistered() {
  if (m_matterReporter == nullptr) {
    return;
  }

  const bool isOn = (m_userTargetParam.brightness > 0U) && !m_isBatteryLow;
  const uint16_t wrgb[4] = {
      m_userTargetParam.wrgb[0], m_userTargetParam.wrgb[1],
      m_userTargetParam.wrgb[2], m_userTargetParam.wrgb[3]};
  m_matterReporter(isOn, m_userTargetParam.brightness, wrgb);
}
