/**
 * @file MatterBridge.h
 * @brief Matter 集成中间层：对外 API，经邮箱投递至 MatterBridgeServer
 * @author hady
 * @date 2026-06-12
 * @layer Middleware
 * @note 不直接调用 Matter 栈；MailPoster 与状态查询由 entry 注册。
 */
#pragma once

#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
#include "LightingManager.h"
#include "RGBLEDWidget.h"
#endif

#include "MatterMailPacket.h"
#include <cstdint>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/color-control-server/color-control-server.h>
#include <app/clusters/identify-server/identify-server.h>

#include "AppConfig.h"

// 引入官方的结构体定义，用于解析指针
#include "LightingManager.h"
#include "RGBLEDWidget.h"

// 重置网络的核心函数声明
#include <app/server/Server.h>
#include <app/server/Server.h> // 必须明确包含此头文件，解决 Server 未定义问题
#include <platform/CHIPDeviceLayer.h>
#include <setup_payload/SetupPayload.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/OpenThread/GenericThreadStackManagerImpl_OpenThread.h>
#endif

// 引入 OpenThread 原生 API 所需的头文件
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <platform/OpenThread/OpenThreadUtils.h>
#endif

/**
 * @brief Matter 集成桥：仅负责组包并投递邮件
 */
class MatterBridge
{
  public:
    /* 本地 Matter 下发回调 */
    using MatterDownlinkCallback = void (*)(const MatterDownlinkUploadPayload& mdc);

    static MatterBridge& Instance()
    {
        static MatterBridge matterBridge;
        return matterBridge;
    }

    /* @brief 初始化 Matter 桥 */
    void Init();

    /* Matter 下发回调*/
    void MatterDownlinkLocalRegister(MatterDownlinkCallback callback);

    /* 本地上报 */
    void MatterUploadLocalReport(MatterDownlinkUploadPayload muc);

    /* 执行Matter命令 */
    void MatterExecuteCmd(MatterExecuteElement executeElement);

    // ##################
    //  连接到 Matter 底层接口
    //  ##################

    /* 读取开关与亮度 */
    void MatterOnBrightnessBridge(int aAction, uint8_t* aValue);
    /* 读取颜色相关 */
    void MatterColorBridge(uint8_t action, void* valueData);
    /* Matter Identify 开始/停止 → 下行 kIdentify */
    void MatterIdentifyBridge(bool active);

    /* 抑制matter下发后回传 */
    bool IsMatterReportBypassEnabled();

    /**
     * @brief 请求打开基础配网窗（可从任意任务上下文调用）
     * @param forceRestartTimer true=已开窗也先关再开，以重置 15 分钟倒计时
     * @note 仅在 FabricCount==0 时生效；实际开窗在 Matter 线程执行。
     */
    void RequestOpenCommissioningWindow(bool forceRestartTimer);

    /**
     * @brief 用户本地操作（按键等）时延长 ICD Active 窗口
     * @note 可从任意任务上下文调用；内部投递至 Matter 线程。
     */
    void NotifyUserInteraction();

    /**
     * @brief 注册「主灯是否开启」查询（供自动配网门禁）
     * @param query 返回 true=亮度>0；nullptr=不检查灯态
     */
    void RegisterLightOnQuery(bool (*query)(void));

    /**
     * @brief 注册首次配网 UI 回调（窗打开 / 配网完成）
     * @note entry 据此驱动系统 LED 白呼吸起停。
     */
    using CommissioningUiCallback = void (*)(bool windowOpenedForFirstPair, bool commissioningDone);
    void RegisterCommissioningUiCallback(CommissioningUiCallback callback);

    /**
     * @brief 是否尚未成功配网过（首次出厂 / 工厂复位后）
     * @note 由 entry 结合 LDC NVM 标志维护后回写；MatterBridge 只缓存供开窗回调使用。
     */
    void SetFirstCommissionPending(bool pending);
    bool IsFirstCommissionPending() const;

  private:
    MatterBridge()  = default;
    ~MatterBridge() = default;

    static bool g_bypass_zcl_callback;

    /* 本地 Matter 下发回调 */
    MatterDownlinkCallback m_matterDownlinkCallback{nullptr};
    /* Matter 下发载荷 */
    MatterDownlinkUploadPayload m_matterDownlinkUploadPayload{};

    bool (*m_lightOnQuery)(void){nullptr};
    CommissioningUiCallback m_commissioningUiCallback{nullptr};
    bool m_firstCommissionPending{true};

    void SetOn(bool isOn);
    void SetBrightness(uint8_t brightness);
    void SetHsv(uint8_t hue, uint8_t saturation);
    void SetCt(uint16_t colorTemperature);
    void SetXy(uint16_t x, uint16_t y);

    void MatterClearNetwork();

    void MatterUploadSwitch(bool is_on);
    void MatterUploadBrightness(uint8_t driver_brightness_percent);
    void MatterUploadHsv(uint8_t hue, uint8_t saturation);
    /** @param colorTemperature Matter mireds（非 Kelvin） */
    void MatterUploadCt(uint16_t colorTemperature);
    void MatterUploadXy(uint16_t x, uint16_t y);

    void RegisterDeviceEventListener(void);

    /**
     * @brief 触发工厂重置：擦除 Matter/Thread/KVS 后受控重启，重启后进入可配网态
     * @note 可从 sleeptimer 中断或任务上下文调用；中断路径经 FreeRTOS 定时器服务中转。
     */
    void TriggerFactoryReset(void);

    /** @brief 启动 IdentifyTime 边沿轮询（不抢官方 emberAfIdentify 回调） */
    void StartIdentifyMonitorRaw();

    /** @brief 轮询 IdentifyTime：0↔非0 边沿驱动主灯识别 */
    void PollIdentifyTimeRaw();

    static void Safe_Upload_OnOff_Callback(intptr_t context);
    static void Safe_Upload_Brightness_Callback(intptr_t context);
    static void Safe_Upload_Hsv_Callback(intptr_t context);
    static void Safe_Upload_Ct_Callback(intptr_t context);
    static void Safe_Upload_Xy_Callback(intptr_t context);
    static void OnMatterDeviceEvent(const chip::DeviceLayer::ChipDeviceEvent* event, intptr_t arg);

    /**
     * @brief FreeRTOS 定时器服务任务上下文回调：把工厂重置请求安全投递到 Matter 主线程
     * @note  签名匹配 FreeRTOS PendedFunction_t，供 xTimerPendFunctionCall(FromISR) 使用。
     */
    static void DeferredNetworkResetDispatch(void* param1, uint32_t param2);

    /** @brief Identify 轮询定时器回调 */
    static void IdentifyMonitorTimerCallback(chip::System::Layer* layer, void* appState);

    /** @brief Matter 线程入口：启动 Identify 轮询 */
    static void StartIdentifyMonitorHandler(intptr_t arg);

    /* 工厂重置（擦除持久化数据后受控重启）路径 */
    static void DoFactoryResetHandler(intptr_t arg);
    static void FactoryResetRebootTimerCallback(chip::System::Layer* layer, void* appState);

    /** @brief Matter 线程：按 arg 打开/刷新配网窗（arg=1 强制重启倒计时） */
    static void OpenCommissioningWindowHandler(intptr_t arg);

    /** @brief Matter 线程：开机自动配网（需灯开） */
    static void EnsureCommissioningWindowOnBootHandler(intptr_t arg);

    /**
     * @brief 实际打开配网窗（须在 Matter 线程持锁调用）
     * @return true=本次成功 OpenBasicCommissioningWindow
     */
    bool OpenCommissioningWindowRaw(bool forceRestartTimer, bool requireLightOn);
};
