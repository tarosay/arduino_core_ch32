/*
 * PWMminBasic.ino — PWMmin ライブラリ 基本サンプル
 *
 * pin 5 (PC3 / TIM1-CH3) で LED をフェードイン・フェードアウトする。
 * pin 5 は TIM1 ピンのため Tools > PWM の設定に関係なく使用可能。
 *
 * FQBN: UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,opt=oslto
 */

#include <PWMmin.h>

void setup() {
  // Pwm_write() の初回呼び出しで自動初期化されるため setup は不要
}

void loop() {
  for (int duty = 0; duty <= 255; duty++) {
    Pwm_write(5, (uint8_t)duty);
    delay(5);
  }
  for (int duty = 255; duty >= 0; duty--) {
    Pwm_write(5, (uint8_t)duty);
    delay(5);
  }
}
