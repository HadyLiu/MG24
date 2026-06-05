#include "AppMatterHandlers.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>

#include "AppConfig.h"

// 引入官方的结构体定义，用于解析指针
#include "LightingManager.h"
#include "RGBLEDWidget.h"

#include "../driver/led_driver.h" // 引入 LED 驱动的头文件，获取 led_ctrl_t 定义和 LED_Start_Fade_RGBW 函数声明

using namespace chip;
using namespace chip::DeviceLayer;

void MyCalculatedRGB(uint16_t chipX, uint16_t chipY)
{
    // 1. 还原为标准的 0.0 ~ 1.0 的 CIE 坐标
    float x = (float)chipX / 65536.0f;
    float y = (float)chipY / 65536.0f;
    float z = 1.0f - x - y;

    // 2. 这里的 Y 顺应亮度，暂设为 1.0 满亮度
    float Y = 1.0f;
    float X = (y > 0) ? (x * Y) / y : 0;
    float Z = (y > 0) ? (z * Y) / y : 0;

    // 3. 使用标准的 sRGB 逆转换矩阵（无视官方指示灯色域）
    float r = 3.2406f * X - 1.5372f * Y - 0.4986f * Z;
    float g = -0.9689f * X + 1.8758f * Y + 0.0415f * Z;
    float b = 0.0557f * X - 0.2040f * Y + 1.0570f * Z;

    // 4. 约束并放大到 0-255 硬件空间
    int R = (r < 0) ? 0 : ((r > 1) ? 255 : (int)(r * 255));
    int G = (g < 0) ? 0 : ((g > 1) ? 255 : (int)(g * 255));
    int B = (b < 0) ? 0 : ((b > 1) ? 255 : (int)(b * 255));

    uint16_t led_r = (uint16_t)(R << 2);
    uint16_t led_g = (uint16_t)(G << 2);
    uint16_t led_b = (uint16_t)(B << 2);
    if (R >= 255)
    {
        led_r = 1023;
    }
    if (G >= 255)
    {
        led_g = 1023;
    }
    if (B >= 255)
    {
        led_b = 1023;
    }

    SILABS_LOG("====> [我的独立转换] 绕过系统限界后的纯正 RGB: R:%d | G:%d | B:%d <====\n", R, G, B);
    // TODO: 送入你的 sm15135e 硬件驱动
    LED_Start_Fade_RGBW(g_led.is_on, g_led.brightness, (led_color_t){.w = 0, .r = led_r, .g = led_g, .b = led_b},
                        LED_FADE_COLOR_SWITCH_MS); // 示例：直接全亮白色，实际应使用 R/G/B 值控制
}

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
        MyCalculatedRGB(x, y); // 计算并输出 RGB 值，绕过官方色域限制
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
// 修正后的回调函数
void OnIdentifyStart(::Identify *identify)
{
    //
    SILABS_LOG("\r\n====> [设备识别] Identify Start Triggered! <====\r\n");
}

void OnIdentifyStop(::Identify *identify)
{
    //
    SILABS_LOG("\r\n====> [设备识别] Identify Stop Triggered! <====\r\n");
}

// 静态初始化结构体
static Identify gIdentify = {chip::EndpointId(1), OnIdentifyStart, OnIdentifyStop,
                             chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator};
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
{
    // 注册 Matter 设备事件监听器，关注配网完成等关键事件
    PlatformMgr().AddEventHandler(OnMatterDeviceEvent, reinterpret_cast<intptr_t>(nullptr));

    // 注册 Identify 事件回调，处理设备识别请求
    (void)gIdentify;
}