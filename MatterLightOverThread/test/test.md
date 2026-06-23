

#### 测试灯效

使用 StartEffect

```cpp

  BspLedWrgb::Instance().Init();
  LightEffectEngine::Instance().Init(
      [](const uint16_t* channelDuties) {
        // BspUartLog::DebugLog(1, "W=%u, R=%u, G=%u, B=%u", channelDuties[0],
        //                      channelDuties[1], channelDuties[2],
        //                      channelDuties[3]);
        BspLedWrgb::Instance().LedWrgbSetDuty(
            channelDuties[0], channelDuties[1], channelDuties[2],
            channelDuties[3]);
      },
      BspLedWrgb::GetLedMaxNum(), BspLedWrgb::GetMaxPwmBits(),
      LightEffectProcessor::GetMaxFactorBits());
  g_LightEffect.Init(
      [](uint32_t elapsedMs) {
        //  BspUartLog::DebugLog(1, "Tick triggered!");
        LightEffectEngine::Instance().UpdateTicks(elapsedMs);
      },
      10);
  LightEffectEngine::Instance().RegisterTimerControlCallback(
      [](bool start) { g_LightEffect.Control(start); });

  LightEffectEngine::Instance().StartEffect(
      LightEffectProcessor::GetBezier40BytesFactorFadeIn,
      (const uint16_t[]){0U, 1023U, 0U, 0U}, 255U, 400U);


```



```cpp

// ========================================================================
    // 1. 系统组件基础级初始化
    // ========================================================================
    BspLedWrgb::Instance().Init();
    
    // 初始化渲染引擎底层
    LightEffectEngine::Instance().Init(
        [](const uint16_t* channelDuties) {
            BspLedWrgb::Instance().LedWrgbSetDuty(
                channelDuties[0], channelDuties[1], channelDuties[2], channelDuties[3]);
        },
        BspLedWrgb::GetLedMaxNum(), 
        BspLedWrgb::GetMaxPwmBits(),
        LightEffectProcessor::GetMaxFactorBits()
    );

    // 初始化时序调度器，完成内核级闭环
    LightSequenceScheduler::Instance().Init();

    // 绑定外部 10ms 硬件定时器，驱动引擎时钟增量轴
    g_LightEffect.Init(
        [](uint32_t elapsedMs) {
            LightEffectEngine::Instance().UpdateTicks(elapsedMs);
        },
        10
    );

    // 将定时器启停控制逻辑注册到引擎中，实现低功耗随开随停
    LightEffectEngine::Instance().RegisterTimerControlCallback(
        [](bool start) { g_LightEffect.Control(start); }
    );

    // ========================================================================
    // 2. 场景业务演练（全权通过 Scheduler 控制）
    // ========================================================================
    
    // 场景 A：只想放一个普通的、单步的 400ms 红色呼吸灯（退化兼容模式）
    static const LightSequenceScheduler::SequenceStep singleBreathEffect[] = 
    {
        { LightEffectProcessor::CalcBreath80BytesFactor, {0U, 1023U, 0U, 0U}, 255U, 400U, 0U }
    };
    // 传 1 步，非无限循环。执行完这 400ms，引擎自动关闭硬件时钟并深睡
    // LightSequenceScheduler::Instance().StartSequence(singleBreathEffect, 1, false);


    // 场景 B：高级酷炫级联：红绿交替心跳，且无限死循环（例如 Matter 处于配网状态中）
    static const LightSequenceScheduler::SequenceStep pairingNetworkShow[] = 
    {
        // 红色亮起 1000ms -> 绿色亮起 1000ms
        { LightEffectProcessor::CalcBreath80BytesFactor, {0U, 1023U, 0U, 0U}, 255U, 1000U, 0U },
        { LightEffectProcessor::CalcBreath80BytesFactor, {0U, 0U, 1023U, 0U}, 255U, 1000U, 0U }
    };
    
    // 核心调用：传入剧本、步数、且开启 LoopForever 标志位
    LightSequenceScheduler::Instance().StartSequence(pairingNetworkShow, 2, true);



```


