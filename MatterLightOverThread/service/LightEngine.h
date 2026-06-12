/**
 * @file LightEngine.h
 * @brief 灯光引擎服务：开关/亮度/颜色/淡入淡出/混合特效/Flash 记忆
 * @author hady
 * @date 2026-06-12
 * @layer Service
 * @note 主灯 WRGB 状态机，所有亮度/颜色/特效经此唯一入口；由 10ms tick
 * 驱动输出。 淡变策略见 FadePolicy；混合特效支持 Blink/Breath/Hold/Fade 步进。
 */
#pragma once

#include "../hal/NvmStorage.h"
#include "ColorLibrary.h"
#include "app/LightTypes.h"
#include <stdint.h>

/** @brief 灯光状态快照，用于 Identify 等场景的保存/恢复 */
struct LightSnapshot
{
  bool is_on;                ///< 开关态
  uint8_t brightness;        ///< 亮度 0~255
  uint8_t color_cycle_index; ///< 颜色循环索引
  ColorSource color_source;  ///< 颜色来源（库/自定义）
  WrgbColor raw_color;       ///< 原始 WRGB
};

/**
 * @class LightEngine
 * @brief 主灯 WRGB 状态机：替代原 led_ctrl_t 全局状态
 */
class LightEngine
{
public:
  /** @brief 获取 LightEngine 单例
   *  @return 单例引用 */
  static LightEngine& instance();

  /** @brief 上电初始化：默认状态 + 读取 Flash 记忆
   *  @return 无 */
  void init();

  /** @brief 10ms tick：驱动淡变/闪烁/呼吸/混合特效输出
   *  @return 无 */
  void tick10ms();

  /**
   * @brief 设置开关并淡变
   * @param on   开关态
   * @param fade 淡变策略
   * @return 无
   */
  void setOnOff(bool on, FadePolicy fade = FadePolicyProportionalBrightness);

  /**
   * @brief 设置亮度并淡变
   * @param level 亮度 0~255
   * @param fade  淡变策略
   * @return 无
   */
  void setBrightness(uint8_t level,
                     FadePolicy fade = FadePolicyProportionalBrightness);

  /**
   * @brief 按循环索引设置颜色并淡变
   * @param cycleIndex 循环索引 0~11
   * @param fadeMs     淡变时长（ms）
   * @return 无
   */
  void setColorFromCycleIndex(uint8_t cycleIndex,
                              uint16_t fadeMs = LED_FADE_COLOR_SWITCH_MS);

  /**
   * @brief 设置自定义 WRGB 颜色并淡变
   * @param c      WRGB 颜色
   * @param src    颜色来源
   * @param fadeMs 淡变时长（ms）
   * @return 无
   */
  void setColorRaw(const WrgbColor& c, ColorSource src,
                   uint16_t fadeMs = LED_FADE_COLOR_SWITCH_MS);

  /**
   * @brief 按亮度差计算恒速淡变时长
   * @param from 起始亮度
   * @param to   目标亮度
   * @return 淡变时长（ms）
   */
  uint16_t calcFadeMsForBrightnessDelta(uint8_t from, uint8_t to) const;

  /**
   * @brief 解析最终淡变时长
   * @param policy         淡变策略
   * @param fromBrightness 起始亮度
   * @param toBrightness   目标亮度
   * @param fixedMs        固定策略时的时长
   * @return 淡变时长（ms）
   */
  uint16_t resolveFadeMs(FadePolicy policy, uint8_t fromBrightness,
                         uint8_t toBrightness, uint16_t fixedMs) const;

  /**
   * @brief 运行混合特效序列
   * @param steps     步进数组（最多 5 步）
   * @param stepCount 步数
   * @param cb        序列结束回调（可空）
   * @return 无
   */
  void runMixedSequence(const MixedEffectStep* steps, uint8_t stepCount,
                        MixedEffectCallback cb);

  /** @brief 停止混合特效并淡回常态
   *  @return 无 */
  void stopMixedEffects();

  /** @brief 捕获当前灯光状态快照
   *  @return 状态快照 */
  LightSnapshot captureSnapshot() const;

  /**
   * @brief 从快照恢复状态并淡变
   * @param s      状态快照
   * @param fadeMs 淡变时长（ms）
   * @return 无
   */
  void restoreSnapshot(const LightSnapshot& s, uint16_t fadeMs);

  /**
   * @brief 向当前目标状态发起淡变
   * @param fadeMs 淡变时长（ms）
   * @param policy 淡变策略
   * @return 无
   */
  void startFadeToCurrent(uint16_t fadeMs,
                          FadePolicy policy = FadePolicyProportionalBrightness);

  /** @brief 停止当前特效并淡回常态
   *  @return 无 */
  void stopEffect();

  /**
   * @brief 闪烁特效
   * @param brightness 闪烁亮度
   * @param raw        闪烁颜色
   * @param periodMs   周期（ms）
   * @param count      次数，0 表示持续
   * @return 无
   */
  void setBlink(uint8_t brightness, const WrgbColor& raw, uint16_t periodMs,
                uint16_t count);

  /**
   * @brief 呼吸特效
   * @param brightness 呼吸峰值亮度
   * @param raw        呼吸颜色
   * @param count      次数，0 表示持续
   * @return 无
   */
  void setBreath(uint8_t brightness, const WrgbColor& raw, uint16_t count);

  /**
   * @brief 保持特效（瞬时置位并维持一段时间）
   * @param on         开关态
   * @param brightness 亮度
   * @param raw        颜色
   * @param fadeMs     维持时长（ms）
   * @return 无
   */
  void setHold(bool on, uint8_t brightness, const WrgbColor& raw,
               uint16_t fadeMs);

  /**
   * @brief 线性淡变步（用于混合序列中的真实淡入/淡出）
   * @param on         目标开关态（false 表示淡出到 0）
   * @param brightness 目标亮度
   * @param raw        颜色
   * @param fadeMs     淡变时长（ms）
   * @return 无
   */
  void setFade(bool on, uint8_t brightness, const WrgbColor& raw,
               uint16_t fadeMs);

  /**
   * @brief 设置低电锁灯保护
   * @param protect true=锁灯并清输出，false=解除
   * @return 保护后状态（0/1）
   */
  uint8_t setLowBatteryProtection(bool protect);

  /** @brief 将当前状态持久化到 Flash（有变化才写）
   *  @return 无 */
  void saveStateToFlash();

  /** @brief 是否已完成首次配网
   *  @return true 已完成 */
  bool isFirstCommissionDone() const;

  /** @brief 标记首次配网完成并持久化
   *  @return 无 */
  void setFirstCommissionDone();

  /** @brief 用户特效是否空闲（无混合/特效/淡变）
   *  @return true 空闲 */
  bool isUserEffectIdle() const;

  /** @brief 当前开关态 @return true 开 */
  bool isOn() const
  {
    return m_state.is_on;
  }
  /** @brief 当前亮度 @return 0~255 */
  uint8_t brightness() const
  {
    return m_state.brightness;
  }
  /** @brief 当前颜色循环索引 @return 0~11 */
  uint8_t colorCycleIndex() const
  {
    return m_state.color_cycle_index;
  }
  /** @brief 关灯前的历史亮度 @return 0~255 */
  uint8_t historyBrightness() const
  {
    return m_state.history_brightness;
  }
  /** @brief 设置历史亮度 @param v 亮度 @return 无 */
  void setHistoryBrightness(uint8_t v)
  {
    m_state.history_brightness = v;
  }
  /** @brief 最近一次状态变更来源 @return 来源枚举 */
  ChangeOrigin changeOrigin() const
  {
    return m_state.change_origin;
  }
  /** @brief 设置状态变更来源 @param o 来源 @return 无 */
  void setChangeOrigin(ChangeOrigin o)
  {
    m_state.change_origin = o;
  }
  /** @brief 当前颜色来源 @return 来源枚举 */
  ColorSource colorSource() const
  {
    return m_state.color_source;
  }
  /** @brief 设置颜色来源 @param s 来源 @return 无 */
  void setColorSource(ColorSource s)
  {
    m_state.color_source = s;
  }
  /** @brief 当前原始 WRGB @return WRGB */
  WrgbColor rawColor() const
  {
    return m_state.raw_color;
  }
  /** @brief 是否处于低电锁灯保护 @return true 锁灯 */
  bool isLowBatteryProtected() const
  {
    return m_state.low_battery_protected;
  }

  /**
   * @brief 直接更新常态(开关/亮度/颜色)而不触发淡变
   * @param on         开关态
   * @param brightness 亮度
   * @param raw        颜色
   * @return 无
   */
  void updateNormalState(bool on, uint8_t brightness, const WrgbColor& raw);

  /** @brief 获取混合特效缓冲区指针（兼容旧 C API）
   *  @return 缓冲区首地址 */
  MixedEffectStep* mixedEffectsBuffer()
  {
    return m_mixedEffects;
  }

private:
  LightEngine() = default;

  struct State
  {
    bool is_on;
    uint8_t brightness;
    uint8_t history_brightness;
    bool low_battery_protected;
    uint8_t color_cycle_index;
    ChangeOrigin change_origin;
    ColorSource color_source;
    WrgbColor raw_color;
    EffectMode effect_mode;
    uint32_t effect_start_ms;
    uint32_t effect_period_ms;
    uint16_t effect_count;
    WrgbColor cur_color;
    WrgbColor start_color;
    WrgbColor target_color;
    bool fading;
    uint32_t fade_start_ms;
    uint16_t fade_time_ms;
    FadePolicy fade_policy;
    bool mix_lighting_effects;
    uint8_t mix_effect_index;
    uint8_t mix_effect_end;
    bool mix_end_is_on;
    uint8_t mix_end_brightness;
    WrgbColor mix_end_raw_color;
    MixedEffectCallback mix_end_callback;
  };

  State m_state{};
  MixedEffectStep m_mixedEffects[5]{};
  bool m_firstCommissionDone = false;

  static uint16_t clampU16(uint16_t v, uint16_t minV, uint16_t maxV);
  static uint8_t clampU8(uint8_t v, uint8_t minV, uint8_t maxV);
  void applyOutput(const WrgbColor& color);
  WrgbColor scaleColor(const WrgbColor& raw, uint8_t brightness) const;
  void executeMixedIndex(uint8_t index);
  void mixedEffectsService();
  bool loadStateFromFlash();
  void startFadeInternal(uint16_t fadeMs, FadePolicy policy);
};
