/**
 * HcSr04Distance.ino — HC-SR04 ultrasonic distance monitor for UIAPduino
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 tarosay
 *
 * Overview:
 *   HC-SR04 超音波センサで距離を 500 ms ごとに計測し、結果を WebHID 経由で
 *   ブラウザに送信します。対応ページ: hid-console.html
 *
 * Distance measurement:
 *   TRIG ピンに 10 µs のパルスを送ると、HC-SR04 が超音波を発射します。
 *   物体で反射した超音波を受信すると ECHO ピンが HIGH になります。
 *   この HIGH 期間（µs）を 58 で割ると距離（cm）が得られます。
 *     distance(cm) = echo_pulse(µs) / 58
 *   計測範囲: 約 2 cm ～ 400 cm
 *   タイムアウト (30 000 µs) を超えた場合は範囲外として "dist=----" を出力します。
 *
 * Output (hid console):
 *   dist=125 cm   ← 正常計測
 *   dist=----     ← 範囲外 / タイムアウト
 *
 * LED behavior:
 *   500 ms ごとに 100 ms 点灯 — 計測サイクル動作中
 *
 * Board and IDE settings:
 *   Board : UIAPduino (HID ProMicro CH32V003, Board Version V1.4 + WebHID (EP3))
 *   USB   : Tools > USB > Keyboard+Mouse+WebHID
 *
 * Wiring — UIAPduino to HC-SR04:
 *
 *   UIAPduino            HC-SR04
 *   ────────────────────────────────
 *   3  (PC1)   →   TRIG
 *   4  (PC2)   ←   ECHO
 *   5V         →   VCC
 *   GND        →   GND
 */

#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN      2        // PC0
#define PIN_TRIG         3        // PC1
#define PIN_ECHO         4        // PC2

#define ECHO_TIMEOUT_US  30000UL  // 30 000 µs ≈ 510 cm max range
#define SEND_INTERVAL_MS 250UL

// Returns distance in cm; returns 0 on timeout (no echo / out of range)
static unsigned measureCm(void) {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  uint32_t us = pulseIn(PIN_ECHO, HIGH, ECHO_TIMEOUT_US);
  return (us == 0) ? 0 : (unsigned)(us / 58UL);
}

void setup() {
  WebHID.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  hid.WaitAvailable();
  hid.Ready();

  hid.Clear();
  hid.Println("=== HC-SR04 Distance Monitor ===");
}

void loop() {
  static uint32_t lastMs = 0;
  if ((uint32_t)(millis() - lastMs) < SEND_INTERVAL_MS) return;
  lastMs = millis();

  unsigned cm = measureCm();

  if (cm == 0) {
    hid.Println("dist=----");
  } else {
    hid.Print("dist=");
    hid.Print((int)cm);
    hid.Println(" cm");
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}
