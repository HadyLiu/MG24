#include "AppMatterHandlers.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/color-control-server/color-control-server.h>

#include "AppConfig.h"

// 引入官方的结构体定义，用于解析指针
#include "LightingManager.h"
#include "RGBLEDWidget.h"

// 重置网络的核心函数声明
#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>
#include <setup_payload/SetupPayload.h>
#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h> // 必须明确包含此头文件，解决 Server 未定义问题

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/OpenThread/GenericThreadStackManagerImpl_OpenThread.h>
#endif

// 引入 OpenThread 原生 API 所需的头文件
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <openthread/instance.h>
#include <openthread/dataset.h>
#include <platform/OpenThread/OpenThreadUtils.h>
#endif

#include "../driver/led_wrgb.h"         // 引入 LED 驱动的头文件，获取 led_ctrl_t 定义和 LED_Start_Fade_RGBW 函数声明
#include "../driver/led_mode_convert.h" // 引入 LED 模式转换的头文件，获取 MyCalculatedRGB 函数声明
#include <app/clusters/identify-server/CodegenIntegration.h>

using namespace chip;
using namespace chip::DeviceLayer;

// =================================================================
// 🎯 承接：开关与亮度数据
// =================================================================
void MyActionInitiatedBridge(int aAction, uint8_t *aValue, bool lightOn)
{
    extern void ConditionalWake_up(void);
    ConditionalWake_up(); // 每次行动请求都触发唤醒，确保系统及时响应用户操作
    SILABS_LOG("====> [matter] 收到行动请求: %d <====\n", aAction);

    //  判断是否是开关
    if (aAction == LightingManager::ON_ACTION || aAction == LightingManager::OFF_ACTION)
    {
        if (aAction == LightingManager::ON_ACTION)
        {
            led_set_status(true);
            // 防呆保护：如果历史记录的旧亮度太低或为0，默认恢复到 50% 或者是 100% 亮度，防止开灯不亮
            if (led_get_history_brightness() <= 1)
            {
                led_set_history_brightness(100); // 默认恢复到最大亮度
            }
            led_set_brightness(led_get_history_brightness());
        }
        else if (aAction == LightingManager::OFF_ACTION)
        {
            led_set_status(false);
            // 只有当当前亮度大于 1 时，才值得记录为历史亮度
            if (led_get_brightness() >= 1)
            {
                led_set_history_brightness(led_get_brightness());
            }
            led_set_brightness(0); // 关灯逻辑
        }
        LED_SaveStateToFlash(); // 每次状态变化后保存当前状态到 Flash，以便下次上电恢复
        SILABS_LOG("====> [matter] 开关事件触发: %s | 恢复亮度: %d <====\n", led_get_status() ? "ON" : "OFF", led_get_brightness());

        // 如果没有有效的色彩缓存，从当前色表中提出来恢复
        custom_raw_color_safeguard(led_get_color_index());

        // 🎯 核心修复点：明确地把全局同步更新后的状态和亮度塞入渐变控制器
        led_set_change_origin(StateChangeOrigin::MATTER_APP); // 标记来源
        LED_Start_Fade_RGBW(led_get_status(), led_get_brightness(), led_get_custom_raw(), LED_FADE_COLOR_SWITCH_MS);
    }
    // 1. 判断是否是亮度
    if (aAction == LightingManager::LEVEL_ACTION) //&& g_led.is_on == true)
    {
        if (aValue != nullptr)
        {
            uint8_t brightness = *aValue;
            SILABS_LOG("====> [matter] 亮度: %d <====\n", brightness);

            uint16_t out_brightness = (uint16_t)(brightness * LED_BRIGHTNESS_MAX) >> 8; // 将 0-255 映射到 0-100
            SILABS_LOG("====> [matter] 最终亮度: %d | 灯状态: %s <====\n", out_brightness, led_get_status() ? "ON" : "OFF");
            led_set_change_origin(StateChangeOrigin::MATTER_APP); // 标记来源
            LED_Start_Fade_RGBW(led_get_status(), out_brightness, led_get_custom_raw(), LED_FADE_COLOR_SWITCH_MS);
            LED_SaveStateToFlash(); // 每次状态变化后保存当前状态到 Flash，以便下次上电恢复
        }
    }
}

uint16_t out_ex, out_ey; // 定义全局变量用于存储最终的 XY 值

// =================================================================
// 🎯 承接：色彩与色温数据
// =================================================================
void MyColorEventHandlerBridge(uint8_t action, void *valueData, uint16_t X, uint16_t Y)
{
    if (valueData == nullptr)
    {
        return;
    }
    extern void ConditionalWake_up(void);
    ConditionalWake_up(); // 每次行动请求都触发唤醒，确保系统及时响应用户操作

    // 将泛型指针强转为官方标准颜色数据结构体
    auto *colorData = reinterpret_cast<RGBLEDWidget::ColorData_t *>(valueData);

    // 显式将 uint8_t 强转为官方的 Action_t 枚举，提高代码可读性
    switch (static_cast<LightingManager::Action_t>(action))
    {
    // =========================================================================
    // 模式 1：HSV 颜色模式 (对应官方枚举值 3)
    // Google Home, Alexa 或本地彩光触发
    // =========================================================================
    case LightingManager::COLOR_ACTION_HSV:
    {
        uint8_t hue = colorData->hsv.h;        // 色调 (0-254)
        uint8_t saturation = colorData->hsv.s; // 饱和度 (0-254)

        SILABS_LOG("====> [色彩模式 - HSV] 行动ID: %d | Hue: %d, Saturation: %d <====\n", action, hue, saturation);

        // 🎯 TODO: 在此调用你的 HSV 转 RGB 驱动
        // 示例：led_driver_set_hsv(hue, saturation);
        uint8_t r, g, b;
        led_set_change_origin(StateChangeOrigin::MATTER_APP);           // 标记来源
        LedDriver_ConvertHsvToRgb(hue, saturation, 254, &r, &g, &b);    // 直接调用转换函数计算 RGB，实际应用中请替换为你的驱动函数
        LED_Start_Fade_RGBW_8bit(0, r, g, b, LED_FADE_COLOR_SWITCH_MS); // 白色通道 W 固定为 0，实际应用中可根据需要调整
        LED_SaveStateToFlash();                                         // 每次状态变化后保存当前状态到 Flash，以便下次上电恢复
        break;
    }

    // =========================================================================
    // 模式 2：Color Temperature (冷暖色温模式，对应官方枚举值 4)
    // =========================================================================
    case LightingManager::COLOR_ACTION_CT:
    {
        uint16_t mireds = colorData->ct.ctMireds;

        // 物理色温（Kelvin）与 Mireds 的标准换算公式
        uint32_t kelvin = (mireds > 0) ? (1000000 / mireds) : 0;

        SILABS_LOG("====> [色彩模式 - 色温] 行动ID: %d | Mireds: %d, 绝对色温: %d K <====\n", action, mireds, kelvin);

        // 🎯 TODO: 在此调用你的双色温驱动控制冷暖比例
        // 示例：main_light_control_set_ct(kelvin);
        uint8_t w, r, g, b;
        led_set_change_origin(StateChangeOrigin::MATTER_APP);           // 标记来源
        Light_Calc_CT_To_WRGB(kelvin, &w, &r, &g, &b);                  // 计算出对应的 RGBW 基准值，实际应用中请替换为你的驱动函数
        LED_Start_Fade_RGBW_8bit(w, r, g, b, LED_FADE_COLOR_SWITCH_MS); // 将计算出的 RGBW 值应用到硬件
        LED_SaveStateToFlash();                                         // 每次状态变化后保存当前状态到 Flash，以便下次上电恢复
        break;
    }

    // =========================================================================
    // 模式 3：CIE 1931 XY 颜色模式 (对应官方枚举值 5)
    // Apple Home 等生态下发的绝对坐标模式
    // =========================================================================
    case LightingManager::COLOR_ACTION_XY:
    {
        // g_ex = out_ex; // 更新全局变量
        // g_ey = out_ey; // 更新全局变量
        uint8_t r, g, b;
        led_set_change_origin(StateChangeOrigin::MATTER_APP);           // 标记来源
        Light_Calc_XY_To_RGB(out_ex, out_ey, &r, &g, &b);               // 计算并输出 RGB 值
        LED_Start_Fade_RGBW_8bit(0, r, g, b, LED_FADE_COLOR_SWITCH_MS); // 白色通道 W 固定为 0，实际应用中可根据需要调整
        LED_SaveStateToFlash();                                         // 每次状态变化后保存当前状态到 Flash，以便下次上电恢复
        break;
    }

    // =========================================================================
    // 其他非色彩 Action 过滤（如 ON_ACTION, OFF_ACTION, LEVEL_ACTION 等）
    // =========================================================================
    default:
        // 这里不需要报错，因为开关、亮度事件通常由别的 Handler（如 MyOnOffEventHandlerBridge）处理
        SILABS_LOG("====> [色彩模式] 未知的行动ID: %d <====\n", action);
        break;
    }
}

// ================= 2. 上传数据 =================
/**
 * @brief 本地上报回环阻断标志：true 表示当前属性写入来自本地同步，不应再反向触发控制链
 */
static bool g_bypass_zcl_callback = false;

bool IsMatterReportBypassEnabled(void)
{ return g_bypass_zcl_callback; }

/**
 * @brief 在 Matter 线程执行 OnOff 属性写入
 */
static void Safe_Upload_OnOff_Callback(intptr_t context)
{
    const bool       is_on = (context != 0);
    chip::EndpointId targetEndpoint = 1;
    static uint8_t   last_reported_onoff = 0xFF;
    const uint8_t    onoff_value = is_on ? 1 : 0;

    if (onoff_value == last_reported_onoff)
    {
        return;
    }

    g_bypass_zcl_callback = true;
    chip::Protocols::InteractionModel::Status status = chip::app::Clusters::OnOff::Attributes::OnOff::Set(targetEndpoint, is_on);
    g_bypass_zcl_callback = false;

    if (status == chip::Protocols::InteractionModel::Status::Success)
    {
        last_reported_onoff = onoff_value;
        SILABS_LOG("====> [matter async report] 开关上报成功: %s <====\n", is_on ? "ON" : "OFF");
    }
    else
    {
        SILABS_LOG("====> [matter async report] 开关上报失败: 状态码 0x%02X <====\n", static_cast<uint8_t>(status));
    }
}

/**
 * @brief 供驱动层调用的公开接口：上报开关状态
 */
void Upload_Matter_OnOff(bool is_on)
{
    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(Safe_Upload_OnOff_Callback, is_on ? 1 : 0);
    if (err != CHIP_NO_ERROR)
    {
        SILABS_LOG("====> [matter async report] 开关上报投递失败: 0x%" CHIP_ERROR_FORMAT " <====\n", err.Format());
    }
}

// 1. 运行在 Matter 主线程环境下的安全回调函数
static void Safe_Upload_Brightness_Callback(intptr_t context)
{
    uint8_t          driver_brightness_percent = static_cast<uint8_t>(context);
    chip::EndpointId targetEndpoint = 1;

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
    chip::Protocols::InteractionModel::Status status = chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Set(targetEndpoint, matter_level);

    // 解除阻断
    g_bypass_zcl_callback = false;

    if (status == chip::Protocols::InteractionModel::Status::Success)
    {
        SILABS_LOG("====> [matter async report] 纯数据上报成功（未触发LEVEL_ACTION）: %d%% <====\n", driver_brightness_percent);
        last_reported_level = matter_level;
    }
}

// 2. 供驱动层调用的公开接口
void Upload_Matter_Brightness(uint8_t driver_brightness_percent)
{
    if (driver_brightness_percent > 100)
    {
        driver_brightness_percent = 100;
    }
    // 投递到 Matter 线程
    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(Safe_Upload_Brightness_Callback, static_cast<intptr_t>(driver_brightness_percent));
    if (err != CHIP_NO_ERROR)
    {
        SILABS_LOG("====> [matter async report] 亮度上报投递失败: 0x%" CHIP_ERROR_FORMAT " <====\n", err.Format());
    }
}
// ================= 3. 设备识别 (Identify) =================

/**
 * @brief 🎯 属于你自己的真正 Identify 开始回调函数
 */
void MyUserIdentifyStartHandler(Identify *identify)
{
    ChipLogProgress(Zcl, "====> [铁证闭环拦截] 收到网关指令：Identify 闪烁开始！ <====");
    // 在这里写你的 LED 闪烁触发代码
    extern void    LED_SetBlink(uint8_t brightness, uint8_t color_index, uint16_t period_ms, uint16_t count);
    extern uint8_t led_get_brightness(void);
    extern uint8_t led_get_color_index(void);
    extern void    ConditionalWake_up(void);
    ConditionalWake_up(); // 每次行动请求都触发唤醒，确保系统及时响应用户操作
    LED_SetBlink(led_get_brightness(), led_get_color_index(), 800, 2);
}

/**
 * @brief 🎯 属于你自己的真正 Identify 结束回调函数
 */
void MyUserIdentifyStopHandler(Identify *identify)
{
    ChipLogProgress(Zcl, "====> [铁证闭环拦截] 收到网关指令：Identify 闪烁结束！ <====");

    // 在这里写你的 LED 停止闪烁代码
}

/**
 * @brief 在初始化函数（如 RegisterDeviceEventListener）中调用此函数即可
 */
void InitUserIdentifyCluster()
{
    // 2. 🔥 核心魔法：利用第 160/165 行的特性，直接静态实例化一个属于我们自己的处理器
    // 它在构造时会自动执行 RegisterLegacyIdentify 并注入全局数据模型中！
    static Identify sMyPrivateIdentify(1,                                                     // Endpoint 1
                                       MyUserIdentifyStartHandler,                            // 你的开始函数
                                       MyUserIdentifyStopHandler,                             // 你的结束函数
                                       chip::app::Clusters::Identify::IdentifyTypeEnum::kNone // 默认识别类型
    );

    ChipLogProgress(Zcl, "====> [应用层处理器挂载成功] 应用层专属 Identify 实例已成功注入全局数据模型！");
}

//  ================= 4. 配网成功通知 =================
static void OnMatterDeviceEvent(const ChipDeviceEvent *event, intptr_t arg)
{
    static bool is_animation_triggered = false; // 静态变量作为锁，确保配对成功动画只触发一次

    if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
    {
        is_animation_triggered = false;
    }

    switch (event->Type)
    {
    // 🎯 核心事件：配网完成（手机成功将设备加入家庭网络）
    case DeviceEventType::kCommissioningComplete:
        SILABS_LOG("🎯 Matter Connection Established: Commissioning Complete!");
        if (!is_animation_triggered)
        {
            is_animation_triggered = true; // 上锁
            extern void TriggerPairingSuccessAnimation(void);
            TriggerPairingSuccessAnimation();
        }
        break;

    case DeviceEventType::kCHIPoBLEConnectionEstablished: SILABS_LOG("BLE connection established with phone."); break;
    default: break;
    }
}

void RegisterDeviceEventListener(void)
{
    // 注册 Matter 设备事件监听器，关注配网完成等关键事件
    PlatformMgr().AddEventHandler(OnMatterDeviceEvent, reinterpret_cast<intptr_t>(nullptr));

    // 运行时强符号重写，确保我们的函数被挂载到系统默认实例上
    InitUserIdentifyCluster();
    SILABS_LOG("====> [Identify] 设备识别群集成功注册到 Endpoint 1 <====");
}

///**
// * @brief 重置网络的核心函数，供异步调用
// */
// static void DoSoftNetworkResetHandler(intptr_t arg)
//{
//    ChipLogProgress(DeviceLayer, "=============================================");
//    ChipLogProgress(DeviceLayer, "[SoftReset] 开始执行在线网络重置（不重启）...");
//    ChipLogProgress(DeviceLayer, "=============================================");
//
//    // =================【新增：提取并打印上一次的配网数据】=================
//    ChipLogProgress(DeviceLayer, "------ [历史数据备份] 正在读取旧网络配置 ------");
//
//    // 1.1 尝试读取旧的 Matter Fabric 信息
//    bool hasFabrics = false;
//    for (const auto & fabricInfo : chip::Server::GetInstance().GetFabricTable())
//    {
//        if (fabricInfo.GetFabricIndex() != chip::kUndefinedFabricIndex)
//        {
//            hasFabrics = true;
//            ChipLogProgress(DeviceLayer, "  [Matter旧数据] Fabric Index: 0x%X", fabricInfo.GetFabricIndex());
//            ChipLogProgress(DeviceLayer, "  [Matter旧数据] 网关节点 ID: 0x" ChipLogFormatX64, ChipLogValueX64(fabricInfo.GetNodeId()));
//            ChipLogProgress(DeviceLayer, "  [Matter旧数据] 生态 Fabric ID: 0x" ChipLogFormatX64, ChipLogValueX64(fabricInfo.GetFabricId()));
//        }
//    }
//
//    // 1.2 尝试读取旧的 Thread Dataset 参数
// #if CHIP_DEVICE_CONFIG_ENABLE_THREAD
//    otInstance *otInst = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
//    if (otInst != nullptr)
//    {
//        otOperationalDataset dataset;
//        if (otDatasetGetActive(otInst, &dataset) == OT_ERROR_NONE)
//        {
//            if (dataset.mComponents.mIsNetworkNamePresent) {
//                ChipLogProgress(DeviceLayer, "  [Thread旧数据] 网络名称: %s", dataset.mNetworkName.m8);
//            }
//            if (dataset.mComponents.mIsPanIdPresent) {
//                ChipLogProgress(DeviceLayer, "  [Thread旧数据] PAN ID: 0x%04X", dataset.mPanId);
//            }
//            if (dataset.mComponents.mIsChannelPresent) {
//                ChipLogProgress(DeviceLayer, "  [Thread旧数据] 无线通道: %d", dataset.mChannel);
//            }
//        }
//    }
// #endif
//    ChipLogProgress(DeviceLayer, "--------------------------------------------");
//
//    // =================【修改：带安全保护的 Fabric 清除】=================
//    // 2. 移除所有 Fabric 绑定关系
//    if (hasFabrics)
//    {
//        // 只有真正存在有效网关绑定时才执行删除，防止空转崩溃重启
//        chip::Server::GetInstance().GetFabricTable().DeleteAllFabrics();
//        ChipLogProgress(DeviceLayer, "[SoftReset] Matter Fabrics 已全部清除");
//    }
//    else
//    {
//        ChipLogProgress(DeviceLayer, "[SoftReset] 设备处于未配网状态，跳过 Fabric 清除（安全拦截）");
//    }
//
//    // 3. 使用更兼容的 OpenThread 原生机制清除网络数据
// #if CHIP_DEVICE_CONFIG_ENABLE_THREAD
//    if (otInst != nullptr)
//    {
//        ChipLogProgress(DeviceLayer, "[SoftReset] 正在通过空数据集重置 Thread 状态...");
//
//        // 强行把 OpenThread 状态机降级，停止尝试连接
//        otThreadSetEnabled(otInst, false);
//        otIp6SetEnabled(otInst, false);
//
//        // 初始化一个结构体全为 0 的空数据集
//        otOperationalDataset emptyDataset;
//        std::memset(&emptyDataset, 0, sizeof(emptyDataset));
//
//        // 显式声明这个数据集没有任何有效组件
//        emptyDataset.mComponents.mIsNetworkKeyPresent = false;
//        emptyDataset.mComponents.mIsExtendedPanIdPresent = false;
//        emptyDataset.mComponents.mIsPanIdPresent = false;
//        emptyDataset.mComponents.mIsChannelPresent = false;
//        emptyDataset.mComponents.mIsActiveTimestampPresent = false;
//
//        // 把这个完全无意义的空数据集应用进去，强行覆盖掉 NVM 中的旧配网数据
//        otDatasetSetActive(otInst, &emptyDataset);
//
//        ChipLogProgress(DeviceLayer, "[SoftReset] OpenThread 原生 Dataset 覆盖清除成功！");
//    }
//    else
//    {
//        ChipLogError(DeviceLayer, "[SoftReset] 错误：无法获取 OpenThread 实例句柄！");
//    }
// #endif
//
//    // 4. 重新开启蓝牙广播，让设备可以被重新搜索
//    chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
//    ChipLogProgress(DeviceLayer, "[SoftReset] 蓝牙配网广播已重新开启，等待新配网...");
//
//    ChipLogProgress(DeviceLayer, "=============================================");
//    ChipLogProgress(DeviceLayer, "[SoftReset] 在线网络重置完成！设备保持在线。");
//    ChipLogProgress(DeviceLayer, "=============================================");
//}

/**
 * @brief 重置网络的外部接口函数，供按键长按回调等调用
 * @note 此函数是线程安全的，可以在任意上下文（如 FreeRTOS 其他任务）中调用 ，判断Fabric是否存在，避免空转崩溃重启
 * @note 该函数会将重置请求投递到 Matter 的主事件循环线程中异步执行，确保线程安全
 * @note 配网失败的防御机制：在核心重置函数中增加了对 Fail-Safe
 * 状态的强制解除和配网状态机的重置，确保即使处于配网失败的中间态也能安全退出，避免死锁和重启
 */
static void DoSoftNetworkResetHandler(intptr_t arg)
{
    ChipLogProgress(DeviceLayer, "=============================================");
    ChipLogProgress(DeviceLayer, "[SoftReset] 开始执行在线网络重置（不重启）...");
    ChipLogProgress(DeviceLayer, "=============================================");

    // =================================================================
    // 1. 强行重置配网窗口状态
    // =================================================================
    ChipLogProgress(DeviceLayer, "[SoftReset] 正在关闭并复位配网状态机...");

    // 无论当前配网是成功、失败、还是进行到中途，强行关闭配网窗口。
    // 这在 Matter 栈内部会自动释放与该配网周期关联的临时会话和未闭合的握手通道。
    chip::Server::GetInstance().GetCommissioningWindowManager().CloseCommissioningWindow();

    // =================================================================
    // 2. 安全检查：仅在真正配过网时才删除 Fabric
    // =================================================================
    bool hasFabrics = false;
    for (const auto &fabricInfo : chip::Server::GetInstance().GetFabricTable())
    {
        if (fabricInfo.GetFabricIndex() != chip::kUndefinedFabricIndex)
        {
            hasFabrics = true;
            ChipLogProgress(DeviceLayer, "  [Matter旧数据] 发现激活的 Fabric Index: 0x%X", fabricInfo.GetFabricIndex());
        }
    }

    if (hasFabrics)
    {
        // 只有配过网才删除，防止未配网或配网失败中途空转导致内部迭代器断言 Crash
        chip::Server::GetInstance().GetFabricTable().DeleteAllFabrics();
        ChipLogProgress(DeviceLayer, "[SoftReset] Matter Fabrics 已全部清除");
    }
    else
    {
        ChipLogProgress(DeviceLayer, "[SoftReset] 未发现有效已激活 Fabric，跳过清除（安全拦截成功）。");
    }

    // =================================================================
    // 3. 使用 OpenThread 原生底层机制清除 Thread 状态
    // =================================================================
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    otInstance *otInst = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
    if (otInst != nullptr)
    {
        ChipLogProgress(DeviceLayer, "[SoftReset] 正在清空 OpenThread 数据集...");
        // 显式让底层状态机断开连接，不向旧网关发送分离通知，直接抹除
        otThreadSetEnabled(otInst, false);
        otIp6SetEnabled(otInst, false);

        // 使用全 0 结构体覆盖，彻底洗干净 NVM 里的 Thread 凭证
        otOperationalDataset emptyDataset;
        std::memset(&emptyDataset, 0, sizeof(emptyDataset));
        emptyDataset.mComponents.mIsNetworkKeyPresent = false;
        emptyDataset.mComponents.mIsExtendedPanIdPresent = false;
        emptyDataset.mComponents.mIsPanIdPresent = false;
        emptyDataset.mComponents.mIsChannelPresent = false;
        emptyDataset.mComponents.mIsActiveTimestampPresent = false;

        otDatasetSetActive(otInst, &emptyDataset);
        ChipLogProgress(DeviceLayer, "[SoftReset] OpenThread 数据集已强制覆盖清空");
    }
#endif

    // =================================================================
    // 4. 重新开启蓝牙广播，拉回初始配网状态
    // =================================================================
    chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
    ChipLogProgress(DeviceLayer, "[SoftReset] 蓝牙配网广播已重新开启，等待新配网...");

    ChipLogProgress(DeviceLayer, "=============================================");
    ChipLogProgress(DeviceLayer, "[SoftReset] 在线网络重置完成！设备状态已安全归零。");
    ChipLogProgress(DeviceLayer, "=============================================");
}

/**
 * @brief 供外部（如按键长按回调）调用的接口函数
 * @note 此函数是线程安全的，可以在任意上下文（如 FreeRTOS 其他任务）中调用
 */
void TriggerNetworkResetWithoutReboot(void)
{
    // 将重置业务投递到 Matter 的主事件循环线程中异步执行，确保线程安全
    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(DoSoftNetworkResetHandler, 0);

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "[SoftReset] 投递重置任务失败: %" CHIP_ERROR_FORMAT, err.Format());
    }
    else
    {
        ChipLogProgress(DeviceLayer, "[SoftReset] 已成功成功将重置请求发送至 Matter 线程");
    }
}
