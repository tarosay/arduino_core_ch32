/*
 * PWMminBuzzerTIM1.ino — TIM1 残りピン確認（pin 0 / 6 / 12）
 *
 * Tools > PWM = "TIM2 Default (pin 2 / PC0)" で使用
 * pin 5 (TIM1-CH3) は PWMminBuzzer.ino で確認済み。
 *
 * 接続:
 *   ブザーを確認したいピンに差し替えながら使用
 *   ブザー＋ → 各ピン、ブザー－ → GND
 *
 * ビープ回数でどのピンが鳴っているか識別:
 *   pin  0 (PA1 / TIM1-CH2) → ×1
 *   pin  6 (PC4 / TIM1-CH4) → ×2
 *   pin 12 (PD2 / TIM1-CH1) → ×3
 *
 * FQBN: UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,pwm=default,opt=oslto
 */

#include <PWMmin.h>

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

  beep( 0, 1);  delay(800);   /* pin  0: ×1 */
  beep( 6, 2);  delay(800);   /* pin  6: ×2 */
  beep(12, 3);  delay(800);   /* pin 12: ×3 */

  delay(1500);
}
