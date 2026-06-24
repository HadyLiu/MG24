#include "MatterBridgeServer.h"
#include "ColorConverter.h"
#include "DebugLog.h"
#include <cstring>

void MatterBridgeServer::Init()
{
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
  data.kind       = DownlinkKind::kLightControl;
  data.on         = m_cacheOn;
  data.brightness = m_cacheOn ? m_cacheBrightness : 0U;
  memcpy(data.wrgb, m_cacheWrgb, sizeof(data.wrgb));
  EmitDownlinkRaw(data);
}

void MatterBridgeServer::OnMatterDataReceived(MatterDownlinkUploadPayload mdc)
{
  LightTypes::WrgbColor wrgb{0U, 0U, 0U, 0U};
  switch (mdc.element)
  {
  case MatterDownlinkUpdateElement::kOn:
    // 读取到了开关状态
    LOG_MATTER("收到 Matter 下行开关: %s\n", mdc.on ? "开" : "关");
    m_cacheOn = mdc.on;
    EmitLightControlRaw();
    break;

  case MatterDownlinkUpdateElement::kBrightness:
    // 读取到了亮度状态
    LOG_MATTER("收到 Matter 下行亮度: %d\n", mdc.brightness);
    m_cacheBrightness = mdc.brightness;
    if (mdc.brightness > 0U)
    {
      m_cacheOn = true;
    }
    EmitLightControlRaw();
    break;

  case MatterDownlinkUpdateElement::kHsv:
    // 读取到了 HSV 颜色
    wrgb = ColorConverter::FromHsv(
        {mdc.color.hsv.hue, mdc.color.hsv.saturation, 254});
    LOG_MATTER("收到 Matter 下行 HSV: H=%d, S=%d ,W=%d, R=%d, G=%d, B=%d\n",
               mdc.color.hsv.hue, mdc.color.hsv.saturation, wrgb.w, wrgb.r,
               wrgb.g, wrgb.b);
    m_cacheWrgb[0] = wrgb.w;
    m_cacheWrgb[1] = wrgb.r;
    m_cacheWrgb[2] = wrgb.g;
    m_cacheWrgb[3] = wrgb.b;
    EmitLightControlRaw();
    break;

  case MatterDownlinkUpdateElement::kCt:
    // 读取到了色温

    wrgb = ColorConverter::FromColorTemperature(mdc.color.ct.colorTemperature);

    LOG_MATTER("收到 Matter 下行色温: %d,W = %d, R = %d, G = %d, B = %d\n",
               mdc.color.ct.colorTemperature, wrgb.w, wrgb.r, wrgb.g, wrgb.b);
    m_cacheWrgb[0] = wrgb.w;
    m_cacheWrgb[1] = wrgb.r;
    m_cacheWrgb[2] = wrgb.g;
    m_cacheWrgb[3] = wrgb.b;
    EmitLightControlRaw();
    break;
  case MatterDownlinkUpdateElement::kXy:
    // 读取到了 XY 颜色

    wrgb = ColorConverter::FromXy({mdc.color.xy.x, mdc.color.xy.y});
    LOG_MATTER("收到 Matter 下行 XY: X=%d, Y=%d, W=%d, R=%d, G=%d, B=%d\n",
               mdc.color.xy.x, mdc.color.xy.y, wrgb.w, wrgb.r, wrgb.g, wrgb.b);
    m_cacheWrgb[0] = wrgb.w;
    m_cacheWrgb[1] = wrgb.r;
    m_cacheWrgb[2] = wrgb.g;
    m_cacheWrgb[3] = wrgb.b;
    EmitLightControlRaw();
    break;
  case MatterDownlinkUpdateElement::kIdentify:
    // 处理识别事件
    LOG_MATTER("收到 Matter 下行识别事件: %s\n", mdc.on ? "开始" : "结束");
    {
      DownlinkData data{};
      data.kind = DownlinkKind::kIdentify;
      data.on   = mdc.on;
      EmitDownlinkRaw(data);
    }
    break;
  case MatterDownlinkUpdateElement::kCommissioningDone:
    // 处理配网完成事件
    LOG_MATTER("收到 Matter 下行配网完成事件\n");
    {
      DownlinkData data{};
      data.kind = DownlinkKind::kCommissioningDone;
      EmitDownlinkRaw(data);
    }
    break;
  case MatterDownlinkUpdateElement::kNetworkConnected:
    // 处理网络已连接事件
    LOG_MATTER("收到 Matter 下行网络已连接事件\n");
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