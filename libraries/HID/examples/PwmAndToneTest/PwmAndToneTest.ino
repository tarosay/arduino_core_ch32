/*
  PwmAndToneTest - PWM と tone() の動作確認（HID Terminal に進行状況を出力）

  各操作の前後に文字列を送るので、どこで止まったかが分かります。

  必要な設定:
    Tools > USB = "Terminal HID"
    Tools > PWM = "TIM2 Default (pin 2 / PC0)"

  使用ピン:
    pin 2 (PC0, TIM2-CH3, LED_BUILTIN) : PWM  … PWMmin
    pin 6 (PC4, TIM1-CH4)              : ブザー … tone()

  PWM と tone() は必ず別のタイマーに分けてください。tone() は鳴らす周波数に
  合わせて、そのピンが属するタイマーの PSC / ATRLR を書き換えます。同じ
  タイマーを PWM にも使っていると、PWM 側のデューティが壊れます。

  CH32V003 では analogWrite() を使わないでください。HardwareTimer を丸ごと
  引き込むため Flash を 3〜4 KB 余計に消費し、さらに下記の問題があります。
  代わりに PWMmin の Pwm_write() を使ってください。
    - TIM1 と TIM2 の両方を analogWrite() すると、operator new のプール
      (256 B) が足りず無言でフリーズする（HardwareTimer 1 個で 144 B）
    - analogWrite() → pinMode() → analogWrite() と往復するたびに
      136 B が回収されない
*/
#include <PWMmin.h>

PWMMIN_REQUIRE_DEFAULT();

static void send_text(const char* s) {
  const char* p = s;
  while (*p) p++;
  HIDuiap.write((const uint8_t*)s, (int)(p - s));
}

void setup() {
  HIDuiap.begin();
  delay(5000);
  send_text("Pwm And Tone Test\n");
}

void loop() {
  send_text("before PWM\n");
  Pwm_write(2, 128);          // duty 50%（analogWrite(2, 128) 相当）
  send_text("after PWM\n");
  delay(2500);

  send_text("before tone\n");
  tone(6, 1000);
  send_text("after tone\n");
  delay(2500);

  send_text("before noTone\n");
  noTone(6);
  send_text("after noTone\n");
  delay(2500);

  send_text("before PWM stop\n");
  Pwm_stop(2);
  send_text("after PWM stop\n");
  delay(2500);

  send_text("---\n");
  delay(2500);
}
