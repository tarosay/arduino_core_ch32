/*
  ToneBasic - tone() 基本動作確認サンプル

  tone(pin, frequency) で 440 Hz を鳴らし続けます。
  ハードウェア PWM により CPU をブロックせず無限再生します。
  止めるには noTone(pin) を呼んでください。

  接続:
    A2 (PC4) --- ブザー(+) --- GND

  対応ボード: HID ProMicro CH32V003 KBD+Mouse
*/

#define BUZZER_PIN A2

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  tone(BUZZER_PIN, 440);  // 鳴らし始める（ノンブロッキング）
}

void loop()
{
  // tone() はハードウェア PWM で動いているため
  // loop() は自由に他の処理ができる
  delay(1200);

  // 別の周波数に切り替えたい場合は tone() を再度呼ぶだけでよい
  // tone(BUZZER_PIN, 880);
  // delay(1200);

  // 停止したい場合は noTone() を呼ぶ
  // noTone(BUZZER_PIN);
}
