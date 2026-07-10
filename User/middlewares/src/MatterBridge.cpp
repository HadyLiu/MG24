/**
 * @file MatterBridge.cpp
 * @brief Matter 集成中间层：组包并投递邮件至 MatterBridgeServer
 * @author hady
 * @date 2026-06-12
 * @layer Middleware
 */
#include "MatterBridge.h"
#include "DebugLog.h"
#include "sl_sleeptimer.h"

// 用于把中断上下文的清网请求安全转投到任务上下文（xTimerPendFunctionCallFromISR）
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <app/FailSafeContext.h>
#include <app/server/Dnssd.h>
#include <crypto/OperationalKeystore.h>
#include <lib/core/DataModelTypes.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/silabs/ConfigurationManagerImpl.h>
#endif
#include <platform/silabs/KeyValueStoreManagerImpl.h>

using namespace chip;
using namespace chip::DeviceLayer;

bool MatterBridge::g_bypass_zcl_callback = false;

/**
 * @brief 配网完成灯效一次性锁存
 * @note Matter 栈同一次配网会在"会话建立"与"运维完成"两个阶段先后投递
 *       kCommissioningComplete，两者可相隔 10s 以上，时间窗去重无法覆盖。
 *       改用锁存：本次配网只放一次灯效，直到下一次清网(软重置)才复位。
 */
static bool s_commissioningDoneEffectLatched = false;

/** @brief 软重置进行中标志，防止并发重置导致配网状态机错乱 */
static bool s_networkResetInFlight = false;

/** @brief 阶段 2 是否已完成清网（避免打开配网窗重试时重复擦除） */
static bool s_networkDataCleared = false;

/** @brief 等待 Fail-Safe 释放后清网并打开配网窗的重试计数 */
static uint8_t s_openCommissioningWindowRetryCount = 0U;
static constexpr uint8_t  kMaxOpenCommissioningWindowRetries = 50U;
static constexpr uint32_t kOpenCommissioningWindowRetryMs    = 100U;
static constexpr uint32_t kSoftResetPhase2DelayMs            = 300U;

/**
 * @brief 清网与"打开配网窗"之间的间隔
 * @note 把两件重活拆到不同的事件循环调度周期，避免单次 dispatch 过长
 *       （日志曾出现 "Long dispatch time: 194 ms"）扰乱 BLE / Thread 时序。
 */
static constexpr uint32_t kSoftResetOpenWindowDelayMs = 120U;

/**
 * @brief 工厂重置前的缓冲延时：给指示灯展示与日志刷出留时间，随后擦除并重启
 * @note 重启后设备为未配网态，SDK 自动开启 BLE + 可配网广播，可直接扫码重配。
 */
static constexpr uint32_t kFactoryResetRebootDelayMs = 500U;

/**
 * @brief 无 Fabric 时确保基础配网窗与 PASE 监听已打开
 * @note  必须在 Matter 线程且已持有 ChipStack 锁时调用（事件回调 / ScheduleWork）。
 *        工厂重置重启后若仅 BLE 可连但未 ListenForPASE，手机会卡在 Char Write 之前。
 */
static void EnsureCommissioningWindowOpenRaw()
{
    chip::Server& server = chip::Server::GetInstance();
    const uint8_t fabricCount = server.GetFabricTable().FabricCount();

    if (fabricCount != 0U)
    {
        LOG_MATTER("[Commission] Skip open window, fabricCount=%u", fabricCount);
        return;
    }

    chip::CommissioningWindowManager& windowMgr = server.GetCommissioningWindowManager();
    const bool windowOpen = windowMgr.IsCommissioningWindowOpen();
    const chip::Dnssd::CommissioningMode mode = windowMgr.GetCommissioningMode();
    const bool listeningForPase = (mode != chip::Dnssd::CommissioningMode::kDisabled);

    LOG_MATTER("[Commission] fabric=0 windowOpen=%u listening=%u",
               windowOpen ? 1U : 0U, listeningForPase ? 1U : 0U);

    // 窗已开且正在听 PASE：无需动作
    if (windowOpen && listeningForPase)
    {
        return;
    }

    chip::app::FailSafeContext& failSafeContext = server.GetFailSafeContext();
    if (!failSafeContext.IsFailSafeFullyDisarmed())
    {
        LOG_MATTER("[Commission] Fail-Safe armed, skip opening commissioning window");
        return;
    }

    // 窗开着但不听 PASE：先关再开，重建 AdvertiseAndListenForPASE
    if (windowOpen)
    {
        LOG_MATTER("[Commission] Window open but not listening, reopen");
        windowMgr.CloseCommissioningWindow();
    }

    const CHIP_ERROR openErr = windowMgr.OpenBasicCommissioningWindow(
        System::Clock::Seconds32(CHIP_DEVICE_CONFIG_DISCOVERY_TIMEOUT_SECS),
        chip::CommissioningWindowAdvertisement::kAllSupported);
    if (openErr != CHIP_NO_ERROR)
    {
        LOG_MATTER("[Commission] OpenBasicCommissioningWindow failed: %" CHIP_ERROR_FORMAT,
                   openErr.Format());
    }
    else
    {
        LOG_MATTER("[Commission] Basic commissioning window opened, PASE listener ready");
    }
}

/**
 * @brief 启动后兜底打开配网窗（ScheduleWork 回调，ChipStack 已锁定）
 * @note  entry_Init / MatterBridge::Init 晚于 Server::Init，会错过 kServerReady。
 */
static void EnsureCommissioningWindowOnBootHandler(intptr_t arg)
{
    (void)arg;
    EnsureCommissioningWindowOpenRaw();
}

/**
 * @brief 在 Matter 线程启动 Identify 轮询定时器
 */
void MatterBridge::StartIdentifyMonitorHandler(intptr_t arg)
{
    (void)arg;
    MatterBridge::Instance().StartIdentifyMonitorRaw();
}

/** @brief 初始化 Matter 桥并注册配网完成等设备事件监听 */
void MatterBridge::Init()
{
    RegisterDeviceEventListener();

    // Identify 轮询必须在 Matter 线程启动 SystemLayer 定时器（需持 Chip 锁）
    CHIP_ERROR identifyErr =
        chip::DeviceLayer::PlatformMgr().ScheduleWork(StartIdentifyMonitorHandler, 0);
    if (identifyErr != CHIP_NO_ERROR)
    {
        LOG_MATTER("Schedule Identify monitor failed: %" CHIP_ERROR_FORMAT, identifyErr.Format());
    }

    // Server 已初始化完毕，主动兜底开窗，不依赖可能已错过的 kServerReady
    CHIP_ERROR err =
        chip::DeviceLayer::PlatformMgr().ScheduleWork(EnsureCommissioningWindowOnBootHandler, 0);
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("[Commission] Schedule boot window ensure failed: %" CHIP_ERROR_FORMAT,
                   err.Format());
    }
}

/**
 * @brief 注册matter下行数据
 * @return 无
 */
void MatterBridge::MatterDownlinkLocalRegister(MatterDownlinkCallback callback)
{
    m_matterDownlinkCallback = callback;
}

/**
 * @brief 本地上报
 * @param muc 上报数据
 * @return 无
 */
void MatterBridge::MatterUploadLocalReport(MatterDownlinkUploadPayload muc)
{
    switch (muc.element)
    {
        // 上报开关/亮度/颜色数据
    case MatterDataElement::kOn:
        MatterUploadSwitch(muc.on);
        break;
    case MatterDataElement::kBrightness:
        MatterUploadBrightness(muc.brightness);
        break;
    case MatterDataElement::kHsv:
        MatterUploadHsv(muc.color.hsv.hue, muc.color.hsv.saturation);
        break;
    case MatterDataElement::kCt:
        MatterUploadCt(muc.color.ct.colorTemperature);
        break;
    case MatterDataElement::kXy:
        MatterUploadXy(muc.color.xy.x, muc.color.xy.y);
        break;
    default:
        // 未知
        break;
    }
}

/**
 * @brief 执行Matter命令
 * @param executeElement 执行命令类型
 * @return 无
 */
void MatterBridge::MatterExecuteCmd(MatterExecuteElement executeElement)
{
    switch (executeElement)
    {
    case MatterExecuteElement::kClearNetwork:
        MatterClearNetwork();
        break;
    default:
        // 未知
        break;
    }
}

void MatterBridge::MatterIdentifyBridge(bool active)
{
    m_matterDownlinkUploadPayload.on      = active;
    m_matterDownlinkUploadPayload.element = MatterDataElement::kIdentify;
    LOG_MATTER("Matter Identify bridge: %s", active ? "start" : "stop");
    if (m_matterDownlinkCallback != nullptr)
    {
        m_matterDownlinkCallback(m_matterDownlinkUploadPayload);
    }
}

void MatterBridge::SetOn(bool isOn)
{
    m_matterDownlinkUploadPayload.on      = isOn;
    m_matterDownlinkUploadPayload.element = MatterDataElement::kOn;
}

void MatterBridge::SetBrightness(uint8_t brightness)
{
    m_matterDownlinkUploadPayload.brightness = brightness;
    m_matterDownlinkUploadPayload.element    = MatterDataElement::kBrightness;
}

void MatterBridge::SetHsv(uint8_t hue, uint8_t saturation)
{
    m_matterDownlinkUploadPayload.color.hsv.hue        = hue;
    m_matterDownlinkUploadPayload.color.hsv.saturation = saturation;
    m_matterDownlinkUploadPayload.element              = MatterDataElement::kHsv;
}

void MatterBridge::SetCt(uint16_t colorTemperature)
{
    m_matterDownlinkUploadPayload.color.ct.colorTemperature = colorTemperature;
    m_matterDownlinkUploadPayload.element                   = MatterDataElement::kCt;
}

void MatterBridge::SetXy(uint16_t x, uint16_t y)
{
    m_matterDownlinkUploadPayload.color.xy.x = x;
    m_matterDownlinkUploadPayload.color.xy.y = y;
    m_matterDownlinkUploadPayload.element    = MatterDataElement::kXy;
}
// ####################################################
//   连接到 Matter 底层接口
// ####################################################

/**
 * @brief 清除会话 / Fabric / 残留运维密钥 / Thread 数据（不重启）
 * @note 必须在 Fail-Safe 完全释放后调用，避免与异步清理竞态。
 */
static void ClearNetworkPersistentDataRaw()
{
    chip::Server& server = chip::Server::GetInstance();

    // 先清会话，避免旧 CASE/PASE 占用资源
    server.GetSecureSessionManager().ExpireAllSecureSessions();
    server.GetSecureSessionManager().ExpireAllPASESessions();

    for (const auto& fabricInfo : server.GetFabricTable())
    {
        if (fabricInfo.GetFabricIndex() != chip::kUndefinedFabricIndex)
        {
            LOG_MATTER("[SoftReset] Clearing Fabric Index: 0x%X", fabricInfo.GetFabricIndex());
        }
    }

    // 始终调用：同时 RevertPendingFabricData，清理未提交的配网半成品
    server.GetFabricTable().DeleteAllFabrics();
    LOG_MATTER("[SoftReset] Matter Fabrics / pending data cleared");

    // 兜底清理残留 PSA 运维密钥（避免 CSR 时出现 PSA key recycled）
    chip::Crypto::OperationalKeystore* pKeystore =
        const_cast<chip::Crypto::OperationalKeystore*>(server.GetFabricTable().GetOperationalKeystore());
    if (pKeystore != nullptr)
    {
        for (chip::FabricIndex fabricIndex = chip::kMinValidFabricIndex; fabricIndex <= CHIP_CONFIG_MAX_FABRICS; ++fabricIndex)
        {
            (void)pKeystore->RemoveOpKeypairForFabric(fabricIndex);
        }
        pKeystore->RevertPendingKeypair();
        LOG_MATTER("[SoftReset] Operational keystore slots cleaned");
    }

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    chip::DeviceLayer::ConfigurationManagerImpl::GetDefaultInstance().ClearThreadStack();
    LOG_MATTER("[SoftReset] Thread persistent data cleared");
#endif

    chip::DeviceLayer::PersistedStorage::KeyValueStoreMgrImpl().ForceKeyMapSave();

    // 注意：DNS-SD/BLE 的重新广播统一交给随后的 OpenBasicCommissioningWindow 处理，
    // 这里不再重复调用 DnssdServer::StartServer()，避免 "Updating services" 重复churn
    // 以及与配网窗打开时的广播产生竞态。
}

/**
 * @brief 投递在线软重置到 Matter 主线程
 */
void MatterBridge::MatterClearNetwork()
{
    TriggerNetworkResetWithoutReboot();
}

/**
 * @brief 读取开关与亮度
 * @param aAction 动作类型
 * @param aValue 动作值
 * @return 无
 */
void MatterBridge::MatterOnBrightnessBridge(int aAction, uint8_t* aValue)
{
    bool change = false;
    //  判断是否是开关
    if (aAction == LightingManager::ON_ACTION || aAction == LightingManager::OFF_ACTION)
    {
        if (aAction == LightingManager::ON_ACTION)
        {
            SetOn(true);
            change = true;
        }
        else if (aAction == LightingManager::OFF_ACTION)
        {
            SetOn(false);
            change = true;
        }
    }
    // 1. 判断是否是亮度
    if (aAction == LightingManager::LEVEL_ACTION) //&& g_led.is_on == true)
    {
        if (aValue != nullptr)
        {
            // Toggle/Off 后常紧跟 CurrentLevel=MinLevel(1)。若 OnOff 属性已是关，
            // 先同步 Off 再处理 Level，避免仍按“开灯调光”把主灯点亮。
            bool matterOn = true;
            (void)chip::app::Clusters::OnOff::Attributes::OnOff::Get(1, &matterOn);
            if (!matterOn)
            {
                SetOn(false);
                if (m_matterDownlinkCallback != nullptr)
                {
                    m_matterDownlinkCallback(m_matterDownlinkUploadPayload);
                }
            }

            SetBrightness(*aValue);
            change = true;
        }
    }
    // 有变化触发回调
    if (change)
    {
        LOG_MATTER("OnOff/brightness read from Matter OK");

        if (m_matterDownlinkCallback != nullptr)
        {
            m_matterDownlinkCallback(m_matterDownlinkUploadPayload);
        }
    }
}

/**
 * @brief 处理颜色相关的 Matter 下行数据
 * @param action 动作类型
 * @param valueData 动作值
 * @return 无
 */
void MatterBridge::MatterColorBridge(uint8_t action, void* valueData)
{
    if (valueData == nullptr)
    {
        return;
    }
    bool change = false;
    // 将泛型指针强转为官方标准颜色数据结构体
    auto* colorData = reinterpret_cast<RGBLEDWidget::ColorData_t*>(valueData);

    // 显式将 uint8_t 强转为官方的 Action_t 枚举，提高代码可读性
    switch (static_cast<LightingManager::Action_t>(action))
    {

    case LightingManager::COLOR_ACTION_HSV: {
        // 投递HSV色彩数据
        // 色调 (0-254)
        SetHsv(colorData->hsv.h, colorData->hsv.s);
        change = true;
        break;
    }

    case LightingManager::COLOR_ACTION_CT: {
        // 投递色温数据
        SetCt(colorData->ct.ctMireds);
        change = true;
        break;
    }

    case LightingManager::COLOR_ACTION_XY: {
        // 投递XY色彩数据
        SetXy(colorData->xy.x, colorData->xy.y);
        change = true;
        break;
    }

    default:
        // 未知
        break;
    }
    // 有变化触发回调
    if (change)
    {
        LOG_MATTER("Color read from Matter OK");
        if (m_matterDownlinkCallback != nullptr)
        {
            m_matterDownlinkCallback(m_matterDownlinkUploadPayload);
        }
    }
}

/**
 * @brief 本地上报回环阻断标志：true
 * 表示当前属性写入来自本地同步，不应再反向触发控制链
 * @return true 如果启用，否则 false
 */
bool MatterBridge::IsMatterReportBypassEnabled()
{
    return g_bypass_zcl_callback;
}

/**
 * @brief 在 Matter 线程执行 OnOff 属性写入
 */
void MatterBridge::Safe_Upload_OnOff_Callback(intptr_t context)
{
    const bool       is_on               = (context != 0);
    chip::EndpointId targetEndpoint      = 1;
    static uint8_t   last_reported_onoff = 0xFF;
    const uint8_t    onoff_value         = is_on ? 1 : 0;

    if (onoff_value == last_reported_onoff)
    {
        return;
    }

    g_bypass_zcl_callback = true;
    chip::Protocols::InteractionModel::Status status =
        chip::app::Clusters::OnOff::Attributes::OnOff::Set(targetEndpoint, is_on);
    g_bypass_zcl_callback = false;

    if (status == chip::Protocols::InteractionModel::Status::Success)
    {
        last_reported_onoff = onoff_value;
        // 属性与 LightingManager 状态机对齐，避免手机 Toggle 方向反了
        LightMgr().SyncCompletedState(is_on);
        LOG_MATTER("OnOff report OK: %s \n", is_on ? "ON" : "OFF");
    }
    else
    {
        LOG_MATTER("OnOff report failed, status 0x%02X \n", static_cast<uint8_t>(status));
    }
}

/**
 * @brief 供驱动层调用的公开接口：上报开关状态
 */
void MatterBridge::MatterUploadSwitch(bool is_on)
{
    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(Safe_Upload_OnOff_Callback, is_on ? 1 : 0);
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("Schedule OnOff report failed: 0x%" CHIP_ERROR_FORMAT " \n", err.Format());
    }
}

/**
 * @brief 上报亮度百分比
 * @param driver_brightness_percent 亮度百分比（0-100）
 * @return 无
 */
void MatterBridge::Safe_Upload_Brightness_Callback(intptr_t context)
{
    uint8_t          driver_brightness_percent = static_cast<uint8_t>(context);
    chip::EndpointId targetEndpoint            = 1;

    if (driver_brightness_percent > 100)
    {
        driver_brightness_percent = 100;
    }

    // 转换成 Matter 的 0~254 标准值
    uint8_t matter_level = (uint8_t)((uint16_t)driver_brightness_percent * 254 / 100);

    // 线程本地静态变量防重复轰炸
    static uint8_t last_reported_level = 0xFF;
    if (matter_level == last_reported_level)
    {
        return;
    }

    // 🎯 2. 此时这里也可以正确识别它，不再报错
    g_bypass_zcl_callback = true;

    // 写入属性数据库
    chip::Protocols::InteractionModel::Status status =
        chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Set(targetEndpoint, matter_level);

    // 解除阻断
    g_bypass_zcl_callback = false;

    if (status == chip::Protocols::InteractionModel::Status::Success)
    {
        LOG_MATTER("Level report OK (no LEVEL_ACTION): %d%% \n", driver_brightness_percent);
        last_reported_level = matter_level;
    }
}

/**
 * @brief 供驱动层调用的公开接口：上报亮度百分比
 * @param driver_brightness_percent 亮度百分比（0-100）
 * @return 无
 */
void MatterBridge::MatterUploadBrightness(uint8_t driver_brightness_percent)
{
    if (driver_brightness_percent > 100)
    {
        driver_brightness_percent = 100;
    }
    // 投递到 Matter 线程
    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(Safe_Upload_Brightness_Callback,
                                                                   static_cast<intptr_t>(driver_brightness_percent));
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("Schedule brightness report failed: 0x%" CHIP_ERROR_FORMAT " \n", err.Format());
    }
}

void MatterBridge::MatterUploadHsv(uint8_t hue, uint8_t saturation)
{
    // 打包：低 8 位 hue，次 8 位 saturation
    const intptr_t packed = static_cast<intptr_t>(hue) | (static_cast<intptr_t>(saturation) << 8);
    CHIP_ERROR     err    = chip::DeviceLayer::PlatformMgr().ScheduleWork(Safe_Upload_Hsv_Callback, packed);
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("Schedule HSV report failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
}

/**
 * @brief 上报色温（Matter mireds）
 */
void MatterBridge::MatterUploadCt(uint16_t colorTemperatureMireds)
{
    CHIP_ERROR err =
        chip::DeviceLayer::PlatformMgr().ScheduleWork(Safe_Upload_Ct_Callback,
                                                      static_cast<intptr_t>(colorTemperatureMireds));
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("Schedule CT report failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
}

void MatterBridge::MatterUploadXy(uint16_t x, uint16_t y)
{
    // 打包：低 16 位 X，高 16 位 Y
    const intptr_t packed = static_cast<intptr_t>(x) | (static_cast<intptr_t>(y) << 16);
    CHIP_ERROR     err    = chip::DeviceLayer::PlatformMgr().ScheduleWork(Safe_Upload_Xy_Callback, packed);
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("Schedule XY report failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
}

/**
 * @brief Matter 线程：写入 HSV + ColorMode
 */
void MatterBridge::Safe_Upload_Hsv_Callback(intptr_t context)
{
    const uint8_t    hue            = static_cast<uint8_t>(context & 0xFF);
    const uint8_t    saturation     = static_cast<uint8_t>((context >> 8) & 0xFF);
    chip::EndpointId targetEndpoint = 1;

    g_bypass_zcl_callback = true;
    (void)chip::app::Clusters::ColorControl::Attributes::ColorMode::Set(
        targetEndpoint,
        chip::app::Clusters::ColorControl::ColorModeEnum::kCurrentHueAndCurrentSaturation);
    (void)chip::app::Clusters::ColorControl::Attributes::EnhancedColorMode::Set(
        targetEndpoint,
        chip::app::Clusters::ColorControl::EnhancedColorModeEnum::kCurrentHueAndCurrentSaturation);
    const auto hueStatus =
        chip::app::Clusters::ColorControl::Attributes::CurrentHue::Set(targetEndpoint, hue);
    const auto satStatus =
        chip::app::Clusters::ColorControl::Attributes::CurrentSaturation::Set(targetEndpoint, saturation);
    g_bypass_zcl_callback = false;

    if ((hueStatus == chip::Protocols::InteractionModel::Status::Success) &&
        (satStatus == chip::Protocols::InteractionModel::Status::Success))
    {
        LOG_MATTER("HSV report OK: H=%u S=%u", hue, saturation);
    }
    else
    {
        LOG_MATTER("HSV report failed: hue=0x%02X sat=0x%02X",
                   static_cast<uint8_t>(hueStatus),
                   static_cast<uint8_t>(satStatus));
    }
}

/**
 * @brief Matter 线程：写入色温 mireds + ColorMode
 */
void MatterBridge::Safe_Upload_Ct_Callback(intptr_t context)
{
    const uint16_t   mireds         = static_cast<uint16_t>(context);
    chip::EndpointId targetEndpoint = 1;

    g_bypass_zcl_callback = true;
    (void)chip::app::Clusters::ColorControl::Attributes::ColorMode::Set(
        targetEndpoint,
        chip::app::Clusters::ColorControl::ColorModeEnum::kColorTemperatureMireds);
    (void)chip::app::Clusters::ColorControl::Attributes::EnhancedColorMode::Set(
        targetEndpoint,
        chip::app::Clusters::ColorControl::EnhancedColorModeEnum::kColorTemperatureMireds);
    const auto status =
        chip::app::Clusters::ColorControl::Attributes::ColorTemperatureMireds::Set(targetEndpoint, mireds);
    g_bypass_zcl_callback = false;

    if (status == chip::Protocols::InteractionModel::Status::Success)
    {
        LOG_MATTER("CT report OK: %u mireds", mireds);
    }
    else
    {
        LOG_MATTER("CT report failed: 0x%02X", static_cast<uint8_t>(status));
    }
}

/**
 * @brief Matter 线程：写入 XY + ColorMode
 */
void MatterBridge::Safe_Upload_Xy_Callback(intptr_t context)
{
    const uint16_t   x              = static_cast<uint16_t>(context & 0xFFFF);
    const uint16_t   y              = static_cast<uint16_t>((context >> 16) & 0xFFFF);
    chip::EndpointId targetEndpoint = 1;

    g_bypass_zcl_callback = true;
    (void)chip::app::Clusters::ColorControl::Attributes::ColorMode::Set(
        targetEndpoint,
        chip::app::Clusters::ColorControl::ColorModeEnum::kCurrentXAndCurrentY);
    (void)chip::app::Clusters::ColorControl::Attributes::EnhancedColorMode::Set(
        targetEndpoint,
        chip::app::Clusters::ColorControl::EnhancedColorModeEnum::kCurrentXAndCurrentY);
    const auto xStatus =
        chip::app::Clusters::ColorControl::Attributes::CurrentX::Set(targetEndpoint, x);
    const auto yStatus =
        chip::app::Clusters::ColorControl::Attributes::CurrentY::Set(targetEndpoint, y);
    g_bypass_zcl_callback = false;

    if ((xStatus == chip::Protocols::InteractionModel::Status::Success) &&
        (yStatus == chip::Protocols::InteractionModel::Status::Success))
    {
        LOG_MATTER("XY report OK: X=%u Y=%u", x, y);
    }
    else
    {
        LOG_MATTER("XY report failed: x=0x%02X y=0x%02X",
                   static_cast<uint8_t>(xStatus),
                   static_cast<uint8_t>(yStatus));
    }
}

/**
 * @brief Matter 设备事件回调：处理配网完成、BLE连接等事件
 * @param event 设备事件指针
 * @param arg   上下文参数（未使用）
 * @return 无
 */
void MatterBridge::OnMatterDeviceEvent(const ChipDeviceEvent* event, intptr_t arg)
{
    MatterBridge& self = MatterBridge::Instance();
    switch (event->Type)
    {
    // 🎯 核心事件：配网完成（手机成功将设备加入家庭网络）
    case DeviceEventType::kCommissioningComplete:
    {
        // 一次性锁存：同一次配网多次投递只放一次灯效，直到下次清网复位
        if (s_commissioningDoneEffectLatched)
        {
            LOG_MATTER("CommissioningComplete latched, ignore duplicate");
            break;
        }
        s_commissioningDoneEffectLatched = true;

        LOG_MATTER("Commissioning complete!");

        // 运行配对成功特效
        self.m_matterDownlinkUploadPayload.element = MatterDataElement::kCommissioningDone;
        if (self.m_matterDownlinkCallback != nullptr)
        {
            self.m_matterDownlinkCallback(self.m_matterDownlinkUploadPayload);
        }
        break;
    }

    // 辅助判定事件：手机通过蓝牙与设备建立安全会话连接（处于扫码配对中间状态）
    case DeviceEventType::kCHIPoBLEConnectionEstablished:
    {
        LOG_MATTER("BLE connection established");
        // 事件回调已持有 ChipStack 锁，勿再 Lock/Unlock
        EnsureCommissioningWindowOpenRaw();
        break;
    }

    // 服务器就绪 / DNS-SD 就绪：无 Fabric 时兜底打开配网窗（工厂重置重启后常见）
    case DeviceEventType::kServerReady:
    case DeviceEventType::kDnssdInitialized:
    {
        EnsureCommissioningWindowOpenRaw();
        break;
    }

    // 配网 Fail-Safe 超时：重新打开配网窗，便于再次扫码
    case DeviceEventType::kFailSafeTimerExpired:
    {
        EnsureCommissioningWindowOpenRaw();
        break;
    }

    default:
        break;
    }
}

/**
 * @brief 注册 Matter 设备事件监听器
 * @return 无
 */
void MatterBridge::RegisterDeviceEventListener(void)
{
    static bool s_listenerRegistered = false;
    if (s_listenerRegistered)
    {
        return;
    }
    s_listenerRegistered = true;

    PlatformMgr().AddEventHandler(MatterBridge::OnMatterDeviceEvent, reinterpret_cast<intptr_t>(nullptr));
}

/**
 * @brief 重置网络的外部接口函数，供按键长按回调等调用
 * @note 此函数是线程安全的，可以在任意上下文（如 FreeRTOS 其他任务）中调用
 * ，判断Fabric是否存在，避免空转崩溃重启
 * @note 该函数会将重置请求投递到 Matter
 * 的主事件循环线程中异步执行，确保线程安全
 * @note 配网失败的防御机制：在核心重置函数中增加了对 Fail-Safe
 * 状态的强制解除和配网状态机的重置，确保即使处于配网失败的中间态也能安全退出，避免死锁和重启
 */
/**
 * @brief 阶段 2：Fail-Safe 完全释放后仅清网，随后把"打开配网窗"拆到下一调度周期
 * @note OpenBasicCommissioningWindow 要求 IsFailSafeFullyDisarmed()==true。
 *       清网与打开配网窗分两次 dispatch，避免单次事件循环占用过久（曾出现
 *       "Long dispatch time: 194 ms"）导致 BLE/Thread 时序被扰乱、手机连上却无法 PASE。
 */
void MatterBridge::FinishSoftNetworkResetHandler(intptr_t arg)
{
    (void)arg;

    PlatformMgr().LockChipStack();

    chip::app::FailSafeContext& failSafeContext = chip::Server::GetInstance().GetFailSafeContext();
    if (!failSafeContext.IsFailSafeFullyDisarmed())
    {
        PlatformMgr().UnlockChipStack();

        s_openCommissioningWindowRetryCount++;
        if (s_openCommissioningWindowRetryCount < kMaxOpenCommissioningWindowRetries)
        {
            DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(kOpenCommissioningWindowRetryMs),
                                                  FinishSoftNetworkResetTimerCallback, nullptr);
            return;
        }

        LOG_MATTER("[SoftReset] Fail-Safe disarm timeout, continue network clear");
        PlatformMgr().LockChipStack();
    }

    // Fail-Safe 已释放（或超时兜底）：只清网一次
    if (!s_networkDataCleared)
    {
        ClearNetworkPersistentDataRaw();
        s_networkDataCleared = true;
    }

    PlatformMgr().UnlockChipStack();

    // 清网完成后释放事件循环，隔一小段时间再单独打开配网窗（重置重试计数）
    s_openCommissioningWindowRetryCount = 0U;
    DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(kSoftResetOpenWindowDelayMs),
                                          OpenCommissioningWindowTimerCallback, nullptr);
}

/**
 * @brief 阶段 2 延迟重试定时器回调
 */
void MatterBridge::FinishSoftNetworkResetTimerCallback(chip::System::Layer* layer, void* appState)
{
    (void)layer;
    (void)appState;
    PlatformMgr().ScheduleWork(FinishSoftNetworkResetHandler, 0);
}

/**
 * @brief 阶段 3：清网后单独打开基础配网窗口，并显式恢复 BLE 快速广播
 * @note 与清网分开 dispatch，缩短单次事件循环占用；打开失败按节奏重试。
 *       BLE/DNS-SD 的广播由 OpenBasicCommissioningWindow 统一恢复。
 */
void MatterBridge::OpenCommissioningWindowHandler(intptr_t arg)
{
    (void)arg;

    PlatformMgr().LockChipStack();

    const CHIP_ERROR openErr =
        chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow(
            System::Clock::Seconds32(CHIP_DEVICE_CONFIG_DISCOVERY_TIMEOUT_SECS),
            chip::CommissioningWindowAdvertisement::kAllSupported);
    if (openErr != CHIP_NO_ERROR)
    {
        PlatformMgr().UnlockChipStack();

        s_openCommissioningWindowRetryCount++;
        if (s_openCommissioningWindowRetryCount < kMaxOpenCommissioningWindowRetries)
        {
            LOG_MATTER("[SoftReset] OpenBasicCommissioningWindow failed: %" CHIP_ERROR_FORMAT ", retrying",
                       openErr.Format());
            DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(kOpenCommissioningWindowRetryMs),
                                                  OpenCommissioningWindowTimerCallback, nullptr);
            return;
        }

        LOG_MATTER("[SoftReset] OpenBasicCommissioningWindow failed permanently: %" CHIP_ERROR_FORMAT, openErr.Format());
        s_networkResetInFlight = false;
        return;
    }

    PlatformMgr().UnlockChipStack();

    s_networkResetInFlight              = false;
    s_openCommissioningWindowRetryCount = 0U;

    LOG_MATTER("[SoftReset] Basic commissioning window opened, ready to pair");
    LOG_MATTER("=============================================");
    LOG_MATTER("[SoftReset] Online network reset done (no reboot)");
    LOG_MATTER("=============================================");
}

/**
 * @brief 阶段 3 延迟 / 重试定时器回调
 */
void MatterBridge::OpenCommissioningWindowTimerCallback(chip::System::Layer* layer, void* appState)
{
    (void)layer;
    (void)appState;
    PlatformMgr().ScheduleWork(OpenCommissioningWindowHandler, 0);
}

/**
 * @brief 阶段 1：关闭配网窗 / 断 BLE / 解 Fail-Safe，再延迟进入清网阶段
 */
void MatterBridge::DoSoftNetworkResetHandler(intptr_t arg)
{
    (void)arg;

    if (s_networkResetInFlight)
    {
        LOG_MATTER("[SoftReset] Reset in progress, ignore duplicate request");
        return;
    }
    s_networkResetInFlight              = true;
    s_networkDataCleared                = false;
    s_openCommissioningWindowRetryCount = 0U;
    s_commissioningDoneEffectLatched    = false;

    LOG_MATTER("=============================================");
    LOG_MATTER("[SoftReset] Starting online network clear (no reboot)...");
    LOG_MATTER("=============================================");

    PlatformMgr().LockChipStack();

    chip::Server& server = chip::Server::GetInstance();

    // 1. 关闭配网窗并切断残留会话 / BLE，避免旧连接拖慢下次配网
    server.GetCommissioningWindowManager().CloseCommissioningWindow();
    server.GetSecureSessionManager().ExpireAllSecureSessions();
    server.GetSecureSessionManager().ExpireAllPASESessions();
    if (server.GetBleLayerObject() != nullptr)
    {
        server.GetBleLayerObject()->CloseAllBleConnections();
    }
    ConnectivityMgr().SetBLEAdvertisingEnabled(false);

    // 2. 若 Fail-Safe 已武装，触发异步清理；清 Fabric 放到阶段 2
    chip::app::FailSafeContext& failSafeContext = server.GetFailSafeContext();
    if (failSafeContext.IsFailSafeArmed())
    {
        LOG_MATTER("[SoftReset] Force Fail-Safe timer expiry");
        failSafeContext.ForceFailSafeTimerExpiry();
    }

    PlatformMgr().UnlockChipStack();

    // 3. 稍等 BLE 断开与 Fail-Safe 异步清理，再进入阶段 2
    DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(kSoftResetPhase2DelayMs),
                                          FinishSoftNetworkResetTimerCallback, nullptr);
}

/**
 * @brief 工厂重置阶段 1：刷 KVS 映射并延时缓冲，随后擦除并重启
 * @note  延时用于让"清网中"指示灯展示；ForceKeyMapSave 避免 Silabs 平台
 *        复位前 nvm3 shadow key 泄漏导致重启后 KVS 异常。
 */
void MatterBridge::DoFactoryResetHandler(intptr_t arg)
{
    (void)arg;

    LOG_MATTER("=============================================");
    LOG_MATTER("[FactoryReset] Starting factory reset, erase and reboot in %lu ms...",
               static_cast<unsigned long>(kFactoryResetRebootDelayMs));
    LOG_MATTER("=============================================");

    PlatformMgr().LockChipStack();
    chip::DeviceLayer::PersistedStorage::KeyValueStoreMgrImpl().ForceKeyMapSave();
    PlatformMgr().UnlockChipStack();

    DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(kFactoryResetRebootDelayMs),
                                          FactoryResetRebootTimerCallback, nullptr);
}

/**
 * @brief 工厂重置阶段 2：Silabs 系统工厂复位（整段擦除 NVM3/KVS/Thread 后重启）
 * @note 等价于 BaseApplication::ScheduleFactoryReset()（该类方法为 protected，无法直接调用）。
 *       复位前先清 PSA 运维密钥：该密钥在 Opaque PSA 区，不在 NVM3 KVS 内，
 *       仅 ErasePartition 清不掉，会导致下次配网出现 "PSA key recycled" 并拖慢 CSR。
 */
void MatterBridge::FactoryResetRebootTimerCallback(chip::System::Layer* layer, void* appState)
{
    (void)layer;
    (void)appState;

    LOG_MATTER("[FactoryReset] Calling system factory reset (erase + reboot)");

    PlatformMgr().ScheduleWork([](intptr_t) {
        chip::Server& server = chip::Server::GetInstance();

        // PSA Opaque 运维密钥不在 KVS 分区内，必须在整段擦除前显式删除
        chip::Crypto::OperationalKeystore* pKeystore = const_cast<chip::Crypto::OperationalKeystore*>(
            server.GetFabricTable().GetOperationalKeystore());
        if (pKeystore != nullptr)
        {
            for (chip::FabricIndex fabricIndex = chip::kMinValidFabricIndex;
                 fabricIndex <= CHIP_CONFIG_MAX_FABRICS;
                 ++fabricIndex)
            {
                (void)pKeystore->RemoveOpKeypairForFabric(fabricIndex);
            }
            pKeystore->RevertPendingKeypair();
            LOG_MATTER("[FactoryReset] PSA operational keys cleared");
        }

        // 与 BaseApplication::ScheduleFactoryReset 相同的系统复位路径
        PlatformMgr().HandleServerShuttingDown();
        ConfigurationMgr().InitiateFactoryReset();
    });
}

/**
 * @brief 在任务上下文把重置请求投递到 Matter 主事件循环线程
 * @note  由 xTimerPendFunctionCall(FromISR) 转到 FreeRTOS 定时器服务任务后调用，
 *        此时处于普通任务上下文，可安全操作 CHIP 事件队列。
 * @param param1 未使用
 * @param param2 未使用
 */
void MatterBridge::DeferredNetworkResetDispatch(void* param1, uint32_t param2)
{
    (void)param1;
    (void)param2;

    // 系统工厂复位：BaseApplication::ScheduleFactoryReset → DoFactoryReset → 重启
    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(DoFactoryResetHandler, 0);
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("Schedule reset task failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
    else
    {
        LOG_MATTER("Reset request posted to Matter thread");
    }
}

/**
 * @brief 供外部（如按键长按回调）调用的接口函数
 * @note 按键状态机跑在 sl_sleeptimer 中断上下文，严禁在中断里直接操作 CHIP
 *       事件队列（会出现 "Failed to post event to CHIP Platform event queue"，
 *       导致清网请求被静默丢弃、无法重新配网）。
 *       因此：中断上下文先经 FreeRTOS 定时器服务任务中转到任务上下文，再
 *       ScheduleWork 到 Matter 主线程；任务上下文则直接投递。
 */
void MatterBridge::TriggerNetworkResetWithoutReboot(void)
{
    if (xPortIsInsideInterrupt() != pdFALSE)
    {
        // 中断上下文：只做 ISR 安全的延迟投递，不触碰 CHIP 栈
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        const BaseType_t posted =
            xTimerPendFunctionCallFromISR(DeferredNetworkResetDispatch, nullptr, 0U, &higherPriorityTaskWoken);
        if (posted != pdPASS)
        {
            // 定时器命令队列已满，本次请求丢弃（极少发生，用户可再次长按重试）
            return;
        }
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
        return;
    }

    // 已在任务上下文：直接投递到 Matter 主线程
    DeferredNetworkResetDispatch(nullptr, 0U);
}

namespace {

/** @brief IdentifyTime 边沿：上一拍是否处于识别中 */
bool sIdentifyWasActive = false;

/** @brief Identify 轮询周期 (ms) */
constexpr uint32_t kIdentifyPollIntervalMs = 100U;

} // namespace

/**
 * @brief 启动 IdentifyTime 边沿轮询
 * @note 官方 BaseApplication 已占用 emberAfIdentifyClusterInitCallback；
 *       此处只读官方 Identify 集群状态，不抢符号、不改 SDK。
 */
void MatterBridge::StartIdentifyMonitorRaw()
{
    CHIP_ERROR err = DeviceLayer::SystemLayer().StartTimer(
        System::Clock::Milliseconds32(kIdentifyPollIntervalMs), IdentifyMonitorTimerCallback, nullptr);
    if (err != CHIP_NO_ERROR)
    {
        LOG_MATTER("Identify monitor timer start failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
}

/**
 * @brief Identify 轮询定时器：边沿检测后再次武装
 */
void MatterBridge::IdentifyMonitorTimerCallback(chip::System::Layer* layer, void* appState)
{
    (void)layer;
    (void)appState;

    MatterBridge::Instance().PollIdentifyTimeRaw();

    (void)DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(kIdentifyPollIntervalMs),
                                                IdentifyMonitorTimerCallback, nullptr);
}

/**
 * @brief 轮询 IdentifyTime：上升沿启动识别灯效，下降沿停止
 */
void MatterBridge::PollIdentifyTimeRaw()
{
    chip::app::Clusters::IdentifyCluster* pCluster = FindIdentifyClusterOnEndpoint(LIGHT_ENDPOINT);
    if (pCluster == nullptr)
    {
        return;
    }

    const bool isActive = (pCluster->GetIdentifyTime() > 0U);
    if (isActive == sIdentifyWasActive)
    {
        return;
    }

    sIdentifyWasActive = isActive;
    MatterIdentifyBridge(isActive);
}
