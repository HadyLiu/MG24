

// 测试灯效
void TestLightEffectEngine()
{
  // 测试单次效果：红色淡入，持续 500ms
  LightEffectEngine::Instance().StartEffect(
      LightEffectProcessor::GetBezier40BytesFactorFadeIn,
      (const uint16_t[]){0U, 1023U, 0U, 0U}, 255U, 500U);
}

void TestLightSequence()
{
  static const LightSequenceScheduler::SequenceStep pairingNetworkShow[] = {
      // 红色亮起 3200ms -> 绿色亮起 3200ms
      {LightEffectProcessor::CalcBreath80BytesFactor,
       {0U, 1023U, 0U, 0U},
       255U,
       3200U,
       0U},
      {LightEffectProcessor::CalcBreath80BytesFactor,
       {0U, 0U, 1023U, 0U},
       255U,
       3200U,
       0U}};

  // 核心调用：传入剧本、步数、且开启 LoopForever 标志位
  LightSequenceScheduler::Instance().StartSequence(pairingNetworkShow, 2, true);
}