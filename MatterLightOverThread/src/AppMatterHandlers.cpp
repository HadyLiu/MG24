#include "AppMatterHandlers.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>

#include "AppConfig.h"

// 引入官方的结构体定义，用于解析指针
#include "LightingManager.h"
#include "RGBLEDWidget.h"

using namespace chip;
using namespace chip::DeviceLayer;

// =================================================================
// 🎯 承接：开关与亮度数据
// =================================================================
void MyActionInitiatedBridge(int aAction, uint8_t *aValue)
{
    // 1. 判断是否是亮度
    if (aAction == 2) // 2 对应 LightingManager::LEVEL_ACTION
    {
        if (aValue != nullptr)
        {
            uint8_t brightness = *aValue;
            SILABS_LOG("====> [我的独立文件] 亮度: %d <====\n", brightness);
            // TODO: 执行你的驱动 iadc_driver 或 led_driver
        }
    }
    // 2. 判断是否是开关
    else
    {
        bool is_on = (aAction == 0); // 0 对应 LightingManager::ON_ACTION
        SILABS_LOG("====> [我的独立文件] 开关: %s <====\n", is_on ? "ON" : "OFF");
        // TODO: 执行你的驱动 main_light_control
    }
}

// =================================================================
// 🎯 承接：色彩与色温数据
// =================================================================
void MyColorEventHandlerBridge(uint8_t action, void *valueData)
{
    if (valueData == nullptr)
    {
        return;
    }
    // 将泛型指针强转为官方标准颜色数据结构体
    auto *colorData = reinterpret_cast<RGBLEDWidget::ColorData_t *>(valueData);

    switch (action)
    {
    // 模式 1：CIE 1931 XY 颜色模式 (最常见的生态下发模式，如 Apple Home)
    case LightingManager::COLOR_ACTION_XY: // 对应枚举值 4
    {
        uint16_t x = colorData->xy.x;
        uint16_t y = colorData->xy.y;

        SILABS_LOG("====> [色彩模式 - XY] X: %d, Y: %d <====\n", x, y);

        // 🎯 TODO: 在此调用你的 RGB 驱动 (例如 sm15135e 或 led_pwm_port)
        // 示例：sm15135e_set_xy(x, y);
        break;
    }

    // 模式 2：Color Temperature (冷暖色温模式)
    case LightingManager::COLOR_ACTION_CT: // 对应枚举值 6
    {
        uint16_t mireds = colorData->ct.ctMireds;

        // 物理色温（Kelvin）与 Mireds 的标准换算公式
        uint32_t kelvin = (mireds > 0) ? (1000000 / mireds) : 0;

        SILABS_LOG("====> [色彩模式 - 色温] Mireds: %d, 绝对色温: %d K <====\n", mireds, kelvin);

        // 🎯 TODO: 在此调用你的双色温驱动控制冷暖比例
        // 示例：main_light_control_set_ct(kelvin);
        break;
    }

    // 模式 3：HSV 颜色模式 (色调与饱和度，部分生态如 Google Home 或特定的本地按键触发)
    case LightingManager::COLOR_ACTION_HSV: // 对应枚举值 5
    {
        uint8_t hue = colorData->hsv.h;        // 色调 (Hue)
        uint8_t saturation = colorData->hsv.s; // 饱和度 (Saturation)

        SILABS_LOG("====> [色彩模式 - HSV] Hue: %d, Saturation: %d <====\n", hue, saturation);

        // 🎯 TODO: 在此调用你的 HSV 转 RGB 驱动
        // 示例：led_driver_set_hsv(hue, saturation);
        break;
    }

    default: SILABS_LOG("====> [色彩模式 - 未知] Action ID: %d <====\n", action); break;
    }
}

// ================= 2. 上传数据 =================
void Upload_Temperature_Data(int16_t local_temp_hundredths)
{
    EndpointId targetEndpoint = 1;
    app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(targetEndpoint, local_temp_hundredths);
}

// ================= 3. 设备识别 (Identify) =================
void OnIdentifyStart(Identify *identify)
{ /* LED 闪烁 */
}
void OnIdentifyStop(Identify *identify)
{ /* LED 停止 */
}

static Identify gIdentify = {EndpointId(1), OnIdentifyStart, OnIdentifyStop, app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator};

// ================= 4. 配网成功通知 =================
static void OnMatterDeviceEvent(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type)
    {
    // 🎯 核心事件：配网完成（手机成功将设备加入家庭网络）
    case DeviceEventType::kCommissioningComplete:
        SILABS_LOG("🎯 Matter Connection Established: Commissioning Complete!");

        // 运行配对成功特效
        extern void TriggerPairingSuccessAnimation(void);
        TriggerPairingSuccessAnimation();
        break;

    // 辅助判定事件：手机通过蓝牙与设备建立安全会话连接（处于扫码配对中间状态）
    case DeviceEventType::kCHIPoBLEConnectionEstablished:
    {
        SILABS_LOG("BLE connection established with phone.");
        break;
    }

    default: break;
    }
}

void RegisterDeviceEventListener(void)
{ PlatformMgr().AddEventHandler(OnMatterDeviceEvent, reinterpret_cast<intptr_t>(nullptr)); }