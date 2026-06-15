/*
 * PWMminRemap3.ino — PWMmin TIM2 Remap3 マッピング サンプル（7ピン制御）
 *
 * Tools > PWM = "TIM2 Remap3 (pins 9/15/16)" で使用
 * Tools > U(S)ART = "None" にすること（pin 15/16 が UART TX/RX と競合するため）
 *
 * 制御ピン（7つ）:
 *   TIM1: pin  0 (PA1)  TIM1-CH2
 *         pin  5 (PC3)  TIM1-CH3
 *         pin  6 (PC4)  TIM1-CH4
 *         pin 12 (PD2)  TIM1-CH1
 *   TIM2: pin  9 (PC7)  TIM2-CH2  ← SPI MISO
 *         pin 15 (PD5)  TIM2-CH4  ← UART TX（U(S)ART=None のこと）
 *         pin 16 (PD6)  TIM2-CH3  ← UART RX（U(S)ART=None のこと）
 *
 * ※ pin 3 (PC1 / TIM2-CH1) は省略（ボード上 2.2kΩ プルアップあり）
 *
 * FQBN: UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,pwm=remap3,xserial=none,opt=oslto
 */

#ifndef PWMMIN_TIM2_REMAP3
  #error "Tools > PWM を 'TIM2 Remap3 (pins 9/15/16)' に設定してください"
#endif
#include <PWMmin.h>

static const uint8_t pins[] = {0, 5, 6, 12, 9, 15, 16};
static const uint8_t N = sizeof(pins);

void setup() {}

void loop() {
  // --- 全ピン フェードイン ---
  for (int d = 0; d <= 255; d++) {
    for (uint8_t i = 0; i < N; i++) Pwm_write(pins[i], (uint8_t)d);
    delay(5);
  }
  delay(500);

  // --- 全ピン フェードアウト ---
  for (int d = 255; d >= 0; d--) {
    for (uint8_t i = 0; i < N; i++) Pwm_write(pins[i], (uint8_t)d);
    delay(5);
  }
  delay(500);

  // --- Pwm_freq: TIM1/TIM2 両方の周波数変更 ---
  Pwm_freq(500);                                          // 500 Hz
  for (int d = 0; d <= 255; d++) { Pwm_write(5, (uint8_t)d); delay(3); }
  Pwm_freq(2000);                                         // 2 kHz
  for (int d = 255; d >= 0; d--) { Pwm_write(5, (uint8_t)d); delay(3); }
  Pwm_freq(1000);                                         // デフォルトに戻す

  // --- Pwm_freq_TIM1 / Pwm_freq_TIM2: タイマー個別周波数 ---
  Pwm_freq_TIM1(800);                                     // TIM1 → 800 Hz
  Pwm_freq_TIM2(200);                                     // TIM2 → 200 Hz
  for (int d = 0; d <= 255; d++) {
    Pwm_write(5,  (uint8_t)d);          // TIM1 (800 Hz)
    Pwm_write(9,  (uint8_t)(255 - d));  // TIM2 (200 Hz)
    delay(4);
  }
  Pwm_freq(1000);                                         // 周波数をリセット

  // --- Pwm_stop: PWM 停止・GPIO を入力フロートに復元 ---
  for (uint8_t i = 0; i < N; i++) Pwm_write(pins[i], 128);
  delay(1000);
  for (uint8_t i = 0; i < N; i++) Pwm_stop(pins[i]);
  delay(500);
}
