/**
 * @file MatterBridge.h
 * @brief Matter 集成层：ZCL 属性上下行、配网窗口、Identify、软复位
 * @layer Integration
 */
#pragma once

#include "app/LightTypes.h"
#include "ColorLibrary.h"
#include <stdint.h>

/** @brief Matter 与本地 LightEngine 之间的唯一桥接模块 */
class MatterBridge
{
public:
    static MatterBridge &instance();

    void onActionInitiated(int action, uint8_t *value, bool lightOn);
    void onColorEvent(uint8_t action, void *valueData, uint16_t x, uint16_t y);

    void uploadOnOff(bool on);
    void uploadBrightnessPercent(uint8_t percent);
    void uploadColorFromCycleIndex(uint8_t cycleIndex);

    bool isUnprovisioned() const;
    void openCommissioningWindow();
    void triggerSoftNetworkReset();
    bool isReportBypassEnabled() const;

    void registerDeviceEvents();
    void initIdentifyCluster();

    static uint8_t clampMatterLevel(uint8_t matterLevel);

private:
    MatterBridge() = default;
};
