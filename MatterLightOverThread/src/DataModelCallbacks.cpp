/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 * @file
 *   This file implements the handler for data model messages.
 */

#include "AppConfig.h"
#include "AppMatterHandlers.h"
#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
#include "RGBLEDWidget.h"
#endif //(defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
#include "LightingManager.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <lib/support/logging/CHIPLogging.h>

#ifdef SL_MATTER_ENABLE_AWS
#include "MatterAws.h"
#endif // SL_MATTER_ENABLE_AWS

#include "sl_component_catalog.h"
#ifdef SL_CATALOG_ZIGBEE_ZCL_FRAMEWORK_CORE_PRESENT
#include <MultiProtocolDataModelHelper.h>
#endif // SL_CATALOG_ZIGBEE_ZCL_FRAMEWORK_CORE_PRESENT

using namespace ::chip;
using namespace ::chip::app::Clusters;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type, uint16_t size, uint8_t *value)
{
    [[maybe_unused]] EndpointId endpointId = attributePath.mEndpointId;
    ClusterId                   clusterId = attributePath.mClusterId;
    AttributeId                 attributeId = attributePath.mAttributeId;
    ChipLogProgress(Zcl, "Cluster callback: " ChipLogFormatMEI, ChipLogValueMEI(clusterId));

    if (clusterId == OnOff::Id && attributeId == OnOff::Attributes::OnOff::Id)
    {
        // 抑制本地上报属性 -> 回调再次触发控制的回环
        if (IsMatterReportBypassEnabled())
        {
            ChipLogProgress(Zcl, "Skip OnOff callback due to local report bypass");
            return;
        }

#ifdef SL_MATTER_ENABLE_AWS
        ChipLogProgress(Zcl, "sending light state update");
        MatterAwsSendMsg("light/state", (const char *)(value ? (*value ? "on" : "off") : "invalid"));
#endif // SL_MATTER_ENABLE_AWS
        LightMgr().InitiateAction(AppEvent::kEventType_Light, *value ? LightingManager::ON_ACTION : LightingManager::OFF_ACTION, value);
    }
    // WIP Apply attribute change to Light
    else if (clusterId == LevelControl::Id)
    {
        ChipLogProgress(Zcl, "Level Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId), type,
                        *value, size);

        if (attributeId == LevelControl::Attributes::CurrentLevel::Id && value != nullptr)
        {
            // 抑制本地上报属性 -> 回调再次触发控制的回环
            if (IsMatterReportBypassEnabled())
            {
                ChipLogProgress(Zcl, "Skip Level callback due to local report bypass");
                return;
            }

            LightMgr().InitiateAction(AppEvent::kEventType_Light, LightingManager::LEVEL_ACTION, value);
        }
    }
    // WIP Apply attribute change to Light
    if (clusterId == ColorControl::Id)
    {
        ChipLogProgress(Zcl, "Color Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId), type,
                        *value, size);
#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)

#if 0
         if (clusterId == ColorControl::Id && attributeId == ColorControl::Attributes::CurrentX::Id)
        {
             ChipLogProgress(Zcl, "Color Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId),
             type,
                             *value, size);

            LightMgr().InitiateLightCtrlAction(AppEvent::kEventType_Light, LightingManager::COLOR_ACTION_XY, attributeId, value);
        }
         else if (clusterId == ColorControl::Id && attributeId == ColorControl::Attributes::CurrentY::Id)
        {
            ChipLogProgress(Zcl, "Color Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId),
            type,
                            *value, size);
            LightMgr().InitiateLightCtrlAction(AppEvent::kEventType_Light, LightingManager::COLOR_ACTION_XY, attributeId, value);
        }
#else

        static uint16_t s_raw_x = 0;
        static uint16_t s_raw_y = 0;
        static bool     s_x_ready = false;
        static bool     s_y_ready = false;

        if (attributeId == ColorControl::Attributes::CurrentX::Id)
        {
            if (value != nullptr)
            {
                s_raw_x = *reinterpret_cast<uint16_t *>(value);
                s_x_ready = true;
            }
        }
        else if (attributeId == ColorControl::Attributes::CurrentY::Id)
        {
            if (value != nullptr)
            {
                s_raw_y = *reinterpret_cast<uint16_t *>(value);
                s_y_ready = true;
            }
        }

        if (s_x_ready && s_y_ready)
        {
            s_x_ready = false;
            s_y_ready = false;

            // 因为 X 和 Y 都是 16 位（2字节），我们用一个 4 字节的数组把它们捆绑在一起
            uint16_t        combined_xy[2];
            extern uint16_t out_ex, out_ey; // 声明外部全局变量，实际定义在 LightingManager.cpp 中
            out_ex = s_raw_x;               // 更新全局变量
            out_ey = s_raw_y;               // 更新全局变量
            combined_xy[0] = s_raw_x;       // 塞入高保真 X
            combined_xy[1] = s_raw_y;       // 塞入高保真 Y

            // 3. 🚀 继续调用官方函数！但注意：我们传的是捆绑好的 combined_xy 指针！
            // 这样，主任务队列收到事件时，就能同时拿到最新、最纯净的 X 和 Y 了！
            // 传我们打包好的纯净数据
            LightMgr().InitiateLightCtrlAction(AppEvent::kEventType_Light, LightingManager::COLOR_ACTION_XY, attributeId,
                                               reinterpret_cast<uint8_t *>(combined_xy));
        }
#endif // 0

        if (clusterId == ColorControl::Id && attributeId == ColorControl::Attributes::CurrentHue::Id)
        {
            ChipLogProgress(Zcl, "Color Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId), type,
                            *value, size);
            LightMgr().InitiateLightCtrlAction(AppEvent::kEventType_Light, LightingManager::COLOR_ACTION_HSV, attributeId, value);
        }
        else if (clusterId == ColorControl::Id && attributeId == ColorControl::Attributes::CurrentSaturation::Id)
        {
            ChipLogProgress(Zcl, "Color Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId), type,
                            *value, size);
            LightMgr().InitiateLightCtrlAction(AppEvent::kEventType_Light, LightingManager::COLOR_ACTION_HSV, attributeId, value);
        }
        else if (attributeId == ColorControl::Attributes::ColorTemperatureMireds::Id)
        {
            if (size != sizeof(uint16_t))
            {
                ChipLogError(Zcl, "Wrong length for ColorControl value: %d", size);
                return;
            }
            LightMgr().InitiateLightCtrlAction(AppEvent::kEventType_Light, LightingManager::COLOR_ACTION_CT, attributeId, value);
        }
#endif // (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
    }
    else if (clusterId == Identify::Id)
    {
        ChipLogProgress(Zcl, "Identify attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId), type, *value,
                        size);
        // 1. 判断是 Identify Cluster 的事件
        extern uint8_t g_Identify; // 声明外部全局变量，实际定义在 entry.cpp 中
        g_Identify = 1;
        ChipLogProgress(Zcl, "Identify 检测成功");
    }
    extern uint8_t g_Identify;
    g_Identify = clusterId;
    ChipLogProgress(Zcl, "clusterId = " ChipLogFormatMEI " attributeId = " ChipLogFormatMEI, ChipLogValueMEI(clusterId),
                    ChipLogValueMEI(attributeId));
#ifdef SL_CATALOG_ZIGBEE_ZCL_FRAMEWORK_CORE_PRESENT
    MultiProtocolDataModel::WriteMatterAttributeValueToZigbee(endpointId, clusterId, attributeId, value, type);
#endif // SL_CATALOG_ZIGBEE_ZCL_FRAMEWORK_CORE_PRESENT
}

/** @brief OnOff Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * TODO Issue #3841
 * emberAfOnOffClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 * The logic here expects something similar to the deprecated Plugins callback
 * emberAfPluginOnOffClusterServerPostInitCallback.
 *
 */
void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
    // TODO: implement any additional Cluster Server init actions
}
