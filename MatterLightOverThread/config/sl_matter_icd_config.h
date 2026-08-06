#ifndef SL_MATTER_ICD_CONFIG_H
#define SL_MATTER_ICD_CONFIG_H

// SIT ICD configuration for battery-powered Matter light.
// Hub 控制延迟：空闲慢轮询 2s（原 5s）；收到下行后 Active 窗口内 300ms 快轮询。

// <<< Use Configuration Wizard in Context Menu >>>

// <o SL_IDLE_MODE_DURATION_S>  Idle Mode Duration (sec)
// <i> Max subscription reporting interval while in idle mode
#define SL_IDLE_MODE_DURATION_S 600

// <o SL_SHORT_IDLE_MODE_DURATION_S>  Short Idle Mode Duration (sec)
#define SL_SHORT_IDLE_MODE_DURATION_S SL_IDLE_MODE_DURATION_S

// <o SL_ACTIVE_MODE_DURATION_MS>  Active Mode Duration (ms)
// <i> Keep device awake after entering active mode
#define SL_ACTIVE_MODE_DURATION_MS 10000

// <o SL_ACTIVE_MODE_THRESHOLD>  Active Mode Threshold (ms)
// <i> Minimum extension after each network/user activity
#define SL_ACTIVE_MODE_THRESHOLD 1000

// <o SL_ICD_SUPPORTED_CLIENTS_PER_FABRIC>  ICD Supported Clients
#define SL_ICD_SUPPORTED_CLIENTS_PER_FABRIC 2

// <o SL_TRANSPORT_IDLE_INTERVAL>  Transport Idle Interval (ms)
// <i> SIT slow poll (must be <= 15000 ms). 2000ms ≈ 平均 1s 内收到 Hub 指令。
#define SL_TRANSPORT_IDLE_INTERVAL 2000

// <o SL_TRANSPORT_ACTIVE_INTERVAL>  Transport Active Interval (ms)
#define SL_TRANSPORT_ACTIVE_INTERVAL 200

// <o SL_CSL_TIMEOUT>  CSL Timeout (sec)
#define SL_CSL_TIMEOUT 30

// <q CHIP_CONFIG_ENABLE_ICD_DSLS>  Enable Dynamic SIT-LIT Switching
#define CHIP_CONFIG_ENABLE_ICD_DSLS 0

// <q SL_ICD_ENABLE_SELECTIVE_SLEEP>  Enable Selective Sleep
#define SL_ICD_ENABLE_SELECTIVE_SLEEP 1

// <<< end of configuration section >>>

#endif // SL_MATTER_ICD_CONFIG_H
