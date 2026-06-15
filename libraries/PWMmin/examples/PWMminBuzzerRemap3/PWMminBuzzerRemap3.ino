/*
 * PWMminBuzzerRemap3.ino — TIM2 Remap3 ピン確認（pin 3 / 9 / 15 / 16）
 *
 * Tools > PWM    = "TIM2 Remap3 (pins 9/15/16)" で使用
 * Tools > U(S)ART = "None" にすること（pin 15/16 が UART TX/RX と競合）
 *
 * 接続:
 *   ブザーを確認したいピンに差し替えながら使用
 *   ブザー＋ → 各ピン、ブザー－ → GND
 *
 * ビープ回数でどのピンが鳴っているか識別:
 *   pin  3 (PC1 / TIM2-CH1) → ×1  ※ボード上 2.2kΩ プルアップあり
 *   pin  9 (PC7 / TIM2-CH2) → ×2
 *   pin 15 (PD5 / TIM2-CH4) → ×3
 *   pin 16 (PD6 / TIM2-CH3) → ×4
 *
 * FQBN: UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,pwm=remap3,xserial=none,opt=oslto
 */

#include <PWMmin.h>
PWMMIN_REQUIRE_REMAP3();

static void beep(uint8_t pin, uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    Pwm_write(pin, 128);
    delay(200);
    Pwm_stop(pin);
    delay(150);
  }
}

void setup() {}

void loop() {
  Pwm_freq(880);  /* ラ (A5) */

  beep( 3, 1);  delay(800);   /* pin  3: ×1 */
  beep( 9, 2);  delay(800);   /* pin  9: ×2 */
  beep(15, 3);  delay(800);   /* pin 15: ×3 */
  beep(16, 4);  delay(800);   /* pin 16: ×4 */

  delay(1500);
}
