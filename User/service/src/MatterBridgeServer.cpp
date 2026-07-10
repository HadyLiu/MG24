#include "MatterBridgeServer.h"
#include "ColorConverter.h"
#include "DebugLog.h"
#include <cstring>

void MatterBridgeServer::Init()
{
}

/**
 * @brief 本地灯光状态写入下行合并缓存
 */
void MatterBridgeServer::SyncLocalLightState(bool on, uint8_t brightness, const uint16_t* pWrgb)
{
    m_cacheOn = on;
    if (brightness > 0U)
    {
        m_cacheBrightness  = brightness;
        m_lastOnBrightness = brightness;
    }
    else
    {
        m_cacheBrightness = 0U;
    }

    if (pWrgb != nullptr)
    {
        memcpy(m_cacheWrgb, pWrgb, sizeof(m_cacheWrgb));
    }
}

/* 注册唯一下行回调 */
void MatterBridgeServer::RegisterDownlinkHandler(DownlinkHandler handler)
{
    m_downlink = handler;
}

/* 调用已注册的下行回调（空则丢弃） */
void MatterBridgeServer::EmitDownlinkRaw(const DownlinkData& data)
{
    if (m_downlink != nullptr)
    {
        m_downlink(data);
    }
}

/* 用缓存的完整状态构造灯控载荷并发出 */
void MatterBridgeServer::EmitLightControlRaw()
{
    DownlinkData data{};
    data.kind = DownlinkKind::kLightControl;
    data.on   = m_cacheOn;

    // 开灯时若当前亮度为 0（曾被 Level=0 关灯），回退到上次非零亮度
    if (m_cacheOn)
    {
        data.brightness =
            (m_cacheBrightness > 0U) ? m_cacheBrightness : m_lastOnBrightness;
    }
    else
    {
        data.brightness = 0U;
    }

    memcpy(data.wrgb, m_cacheWrgb, sizeof(data.wrgb));
    EmitDownlinkRaw(data);
}

void MatterBridgeServer::OnMatterDataReceived(MatterDownlinkUploadPayload mdc)
{
    LightTypes::WrgbColor wrgb{0U, 0U, 0U, 0U};
    switch (mdc.element)
    {
    case MatterDataElement::kOn:
        // 读取到了开关状态
        LOG_MATTER("Matter downlink OnOff: %s\n", mdc.on ? "ON" : "OFF");
        m_cacheOn = mdc.on;
        if (m_cacheOn && (m_cacheBrightness == 0U))
        {
            m_cacheBrightness = m_lastOnBrightness;
        }
        EmitLightControlRaw();
        break;

    case MatterDataElement::kBrightness:
        // 读取到了亮度状态
        LOG_MATTER("Matter downlink brightness: %d\n", mdc.brightness);
        if (mdc.brightness == 0U)
        {
            // Level=0 视为关灯，保留 m_lastOnBrightness 供下次 OnOff 开灯
            m_cacheOn         = false;
            m_cacheBrightness = 0U;
            EmitLightControlRaw();
            break;
        }

        if (m_cacheOn)
        {
            // 已开灯：正常调光
            m_cacheBrightness  = mdc.brightness;
            m_lastOnBrightness = mdc.brightness;
            EmitLightControlRaw();
        }
        else
        {
            // 关灯态：只更新记忆亮度，绝不点亮。
            // Toggle/Off 后协议栈常跟一条 CurrentLevel=1(MinLevel)，
            // 若此处把 m_cacheOn 置 true，主灯会被重新打开。
            if (mdc.brightness > 1U)
            {
                m_lastOnBrightness = mdc.brightness;
                m_cacheBrightness  = mdc.brightness;
            }
            LOG_MATTER("Ignore Level=%u while OnOff is Off\n", mdc.brightness);
        }
        break;

    case MatterDataElement::kHsv:
        // 读取到了 HSV 颜色
        wrgb = ColorConverter::FromHsv({mdc.color.hsv.hue, mdc.color.hsv.saturation, 254});
        LOG_MATTER("Matter downlink HSV: H=%d, S=%d ,W=%d, R=%d, G=%d, B=%d\n", mdc.color.hsv.hue,
                   mdc.color.hsv.saturation, wrgb.w, wrgb.r, wrgb.g, wrgb.b);
        m_cacheWrgb[0] = wrgb.w;
        m_cacheWrgb[1] = wrgb.r;
        m_cacheWrgb[2] = wrgb.g;
        m_cacheWrgb[3] = wrgb.b;
        EmitLightControlRaw();
        break;

    case MatterDataElement::kCt:
        // 读取到了色温

        wrgb = ColorConverter::FromColorTemperature(mdc.color.ct.colorTemperature);

        LOG_MATTER("Matter downlink CT: %d,W = %d, R = %d, G = %d, B = %d\n", mdc.color.ct.colorTemperature, wrgb.w,
                   wrgb.r, wrgb.g, wrgb.b);
        m_cacheWrgb[0] = wrgb.w;
        m_cacheWrgb[1] = wrgb.r;
        m_cacheWrgb[2] = wrgb.g;
        m_cacheWrgb[3] = wrgb.b;
        EmitLightControlRaw();
        break;
    case MatterDataElement::kXy:
        // 读取到了 XY 颜色

        wrgb = ColorConverter::FromXy({mdc.color.xy.x, mdc.color.xy.y});
        LOG_MATTER("Matter downlink XY: X=%d, Y=%d, W=%d, R=%d, G=%d, B=%d\n", mdc.color.xy.x, mdc.color.xy.y, wrgb.w,
                   wrgb.r, wrgb.g, wrgb.b);
        m_cacheWrgb[0] = wrgb.w;
        m_cacheWrgb[1] = wrgb.r;
        m_cacheWrgb[2] = wrgb.g;
        m_cacheWrgb[3] = wrgb.b;
        EmitLightControlRaw();
        break;
    case MatterDataElement::kIdentify:
        // 处理识别事件
        LOG_MATTER("Matter downlink Identify: %s\n", mdc.on ? "start" : "stop");
        {
            DownlinkData data{};
            data.kind = DownlinkKind::kIdentify;
            data.on   = mdc.on;
            EmitDownlinkRaw(data);
        }
        break;
    case MatterDataElement::kCommissioningDone:
        // 处理配网完成事件
        LOG_MATTER("Matter downlink commissioning done\n");
        {
            DownlinkData data{};
            data.kind = DownlinkKind::kCommissioningDone;
            EmitDownlinkRaw(data);
        }
        break;
    case MatterDataElement::kNetworkConnected:
        // 处理网络已连接事件
        LOG_MATTER("Matter downlink network connected\n");
        {
            DownlinkData data{};
            data.kind = DownlinkKind::kNetworkConnected;
            EmitDownlinkRaw(data);
        }
        break;

    default:
        break;
    }
}