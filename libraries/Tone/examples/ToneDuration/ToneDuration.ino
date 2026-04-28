/*
  ToneDuration - tone() 3引数（duration 指定）サンプル

  tone(pin, frequency, duration) で指定ミリ秒だけ音を出し、
  自動的に停止します。

  注意: duration を指定した場合、tone() は duration ms の間
  CPU をブロックします（その後 noTone() は不要です）。
  duration なし (tone(pin, freq)) はノンブロッキングです。

  接続:
    A2 (PC4) --- ブザー(+) --- GND

  対応ボード: HID ProMicro CH32V003 KBD+Mouse
*/

#define BUZZER_PIN A2

// ドレミファソラシド (C4 スケール)
static const unsigned int scale[] = {
  262,  // C4 ド
  294,  // D4 レ
  330,  // E4 ミ
  349,  // F4 ファ
  392,  // G4 ソ
  440,  // A4 ラ
  494,  // B4 シ
  523,  // C5 ド
};

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop()
{
  // ドレミファソラシド を順番に鳴らす
  for (int i = 0; i < 8; i++) {
    // 300 ms 鳴らして自動停止（tone() 内部で delay(300) が走る）
    tone(BUZZER_PIN, scale[i], 300);
    // tone() が戻った時点で音は止まっている
    // 音符間の無音区間
    delay(100);
  }

  // 全音符を素早く連続 (100 ms ずつ)
  for (int i = 0; i < 8; i++) {
    tone(BUZZER_PIN, scale[i], 100);
    delay(50);
  }

  // 次のループまで 1 秒休憩
  delay(1000);
}
