/*
  ToneNoTone - tone() / noTone() 動作確認サンプル

  tone(pin, frequency) でブザーを鳴らし続け、
  noTone(pin) で止めるテストです。

  HID ProMicro CH32V003 KBD+Mouse ボードでは A2 (PC4) が
  TIM1_CH4 に接続されているため、ハードウェア PWM で
  CPU をブロックせずに無限再生できます。

  接続:
    A2 (PC4) --- ブザー(+) --- GND

  対応ボード: HID ProMicro CH32V003 KBD+Mouse
*/

#define BUZZER_PIN A2

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop()
{
  // 440 Hz (ラ音) で鳴らす
  tone(BUZZER_PIN, 440);
  delay(1500);

  // noTone() で停止
  noTone(BUZZER_PIN);
  delay(500);

  // 880 Hz (1オクターブ上) で鳴らす
  tone(BUZZER_PIN, 880);
  delay(1500);

  noTone(BUZZER_PIN);
  delay(500);

  // 1760 Hz でさらに1オクターブ上
  tone(BUZZER_PIN, 1760);
  delay(1500);

  noTone(BUZZER_PIN);
  delay(1000);
}
