#include "matter_interfacing.h"

//// 引入 Matter SDK 核心集群控制头文件
// #include <app-common/zap-generated/ids/Clusters.h>
// #include <app-common/zap-generated/attributes/Accessors.h>
//// #include <app/util/af.h>
// #include <platform/CHIPDeviceLayer.h>
//
//// 🎯 引入你的灯光物理驱动头文件
// #include "../driver/led_driver.h"
//
// using namespace ::chip::app::Clusters;
//
// extern "C" void Matter_Interface_Init(void)
//{ SILABS_LOG("[Interface] Matter Interfacing Module Initialized."); }
//
///**
// * @brief 1. 数据下发核心接口 (手机/生态控制设备)
// * @note  当用户通过手机 Apple Home / Google Home 调节灯光时，Matter SDK 解析完无线报文
// * 会自动回调此全局函数。我们在此处拦截并喂给你的自定义物理驱动。
// */
// void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t mask, uint16_t type, uint16_t size,
//                                       uint8_t *value)
//{
//    // 过滤 Endpoint：主照明灯通常固定在 Endpoint 1
//    if (attributePath.mEndpointId != 1)
//    {
//        return;
//    }
//
//    // ----------------------------------------------------
//    // 🎯 拦截：开关控制 (OnOff Cluster)
//    // ----------------------------------------------------
//    if (attributePath.mClusterId == OnOff::Id)
//    {
//        if (attributePath.mAttributeId == OnOff::Attributes::OnOff::Id)
//        {
//            bool matter_on = *value; // 拿到手机下发的布尔值：true代表开，false代表关
//            SILABS_LOG("======> [下发] Matter 收到开关指令: %s", matter_on ? "ON" : "OFF");
//
//            // 对接物理驱动：渐变到对应状态，保持当前的亮度和色温索引
//            // 400ms 代表标准过渡时间
//            LED_Start_Fade_Logic(matter_on, g_led.brightness, g_led.color_index, 400);
//        }
//    }
//
//    // ----------------------------------------------------
//    // 🎯 拦截：亮度控制 (LevelControl Cluster)
//    // ----------------------------------------------------
//    else if (attributePath.mClusterId == LevelControl::Id)
//    {
//        if (attributePath.mAttributeId == LevelControl::Attributes::CurrentLevel::Id)
//        {
//            uint8_t raw_level = *value; // 手机下发的原始亮度范围：0 ~ 254
//
//            // 将 Matter 的 0~254 线性转换到你驱动内部需要的 1 ~ 100% 范围
//            uint8_t bri_percent = ((uint32_t)raw_level * 100) / 254;
//            if (bri_percent < 1)
//                bri_percent = 1; // 边界保护
//
//            SILABS_LOG("======> [下发] Matter 收到亮度数据: %d -> 转换百分比: %d%%", raw_level, bri_percent);
//
//            // 对接物理驱动：更新亮度，开关保持开启状态
//            LED_Start_Fade_Logic(g_led.is_on, bri_percent, g_led.color_index, 400);
//        }
//    }
//
//    // ----------------------------------------------------
//    // 🎯 拦截：颜色与色温控制 (ColorControl Cluster)
//    // ----------------------------------------------------
//    else if (attributePath.mClusterId == ColorControl::Id)
//    {
//        // 判定是否是色温下发 (Color Temperature Mireds)
//        if (attributePath.mAttributeId == ColorControl::Attributes::ColorTemperatureMireds::Id)
//        {
//            uint16_t mireds = *((uint16_t *)value); // 拿到下发的 Mireds 值
//            uint32_t kelvin = 1000000 / mireds;     // 转换为人类常识的开尔文色温(K)
//
//            SILABS_LOG("======> [下发] Matter 收到色温 Mireds: %d (实际约 %d K)", mireds, kelvin);
//
//            // 🎯 业务逻辑：根据计算出的 kelvin 值的范围，映射匹配到你 g_color_table 中最接近的 color_index
//            uint8_t matched_index = g_led.color_index;
//            if (kelvin <= 2500)
//                matched_index = 2; // 比如你出厂默认约 2450K 的 color2
//            else if (kelvin >= 5000)
//                matched_index = 0; // 比如冷白光 color0
//            // ... 你可以根据你的 13 种颜色表进一步细化映射
//
//            // 对接物理驱动
//            LED_Start_Fade_Logic(g_led.is_on, g_led.brightness, matched_index, 400);
//        }
//    }
//}
//
///**
// * @brief 2. 数据上传核心接口 (设备主动上报状态给手机)
// * @note  当本地发生物理行为导致灯的状态变更，调用此函数同步修改 Matter 属性数据库，
// * Matter 协议栈会自动将新状态通知到所有已配对连接的手机 App。
// */
// void MatterEventHandler(const ChipDeviceEvent *event, intptr_t arg)
//{
//    switch (event->Type)
//    {
//    // 🎯 更改为 kCommissioningComplete
//    case DeviceEventType::kCommissioningComplete:
//        SILABS_LOG("🎯 Matter Connection Established: Commissioning Complete!");
//        TriggerPairingSuccessAnimation();
//        break;
//
//    case DeviceEventType::kCHIPoBLEConnectionEstablished:
//    {
//        SILABS_LOG("BLE connection established with phone.");
//        break;
//    }
//
//    default: break;
//    }
//}