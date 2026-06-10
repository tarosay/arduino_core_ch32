/**
 * SeekWriteAt.ino — SDmin ランダムアクセス API（v1.2.5）の最小デモ
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 tarosay
 *
 * 概要:
 *   1. TEST.TXT を作成し "0123456789..." の 64 バイトを書き込む
 *   2. sm_seek(10) で読み位置を移動し、1 バイト読んで検証
 *   3. sm_write_at() で途中の 3 バイト（位置 5〜7）を "XYZ" に部分上書き
 *   4. sm_seek(5) で読み直し、"XYZ" になっていることを検証
 *
 * 実行後に SD カードを PC で開くと、TEST.TXT が
 *   01234XYZ89...
 * になっています（ファイルの作り直しなしで途中だけ書き換わる）。
 *
 * LED 表示:
 *   ゆっくり 3 回点滅 → 全テスト成功
 *   高速点滅し続ける → 失敗（SD 初期化失敗・検証不一致など）
 *
 * ボード設定:
 *   Board : UIAPduino (HID ProMicro CH32V003, Board Version V1.4)
 *   FQBN  : UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,opt=oslto
 *   ※ UIAPduino SD 版は microSD がオンボード接続（CS = 6 / PC4）
 */

#include <Arduino.h>
#include <SDmin.h>

#define LED_PIN 2
static const uint8_t PIN_SS = 6;   // CS = A2 = PC4

static void blinkForever(uint16_t ms) {
  for (;;) {
    digitalWrite(LED_PIN, HIGH); delay(ms);
    digitalWrite(LED_PIN, LOW);  delay(ms);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(3000);

  // ── SD 初期化 ──
  if (!sm_init(PIN_SS) || !sm_mount()) blinkForever(100);

  // ── 1. テストファイル作成（'0'〜'9' を繰り返す 64 バイト）──
  if (!sm_open_w("TEST.TXT")) blinkForever(100);
  for (uint8_t i = 0; i < 64; i++) {
    uint8_t c = (uint8_t)('0' + (i % 10));
    if (!sm_write(&c, 1)) blinkForever(100);
  }
  sm_close_w();

  // ── 2. sm_seek: 位置 10 へ移動して読む（'0' のはず）──
  if (!sm_open_r("TEST.TXT")) blinkForever(100);
  if (!sm_seek(10)) blinkForever(100);
  uint8_t c;
  if (sm_read(&c, 1) != 1 || c != '0') blinkForever(100);
  sm_close_r();

  // ── 3. sm_write_at: 位置 5〜7 を "XYZ" に部分上書き ──
  //（ファイルは開いていない状態で呼ぶこと）
  if (!sm_write_at("TEST.TXT", 5, (const uint8_t*)"XYZ", 3)) blinkForever(100);

  // ── 4. 検証: 位置 5 から 3 バイト読むと "XYZ" のはず ──
  if (!sm_open_r("TEST.TXT")) blinkForever(100);
  if (!sm_seek(5)) blinkForever(100);
  uint8_t buf[3];
  if (sm_read_full(buf, 3) != 3) blinkForever(100);
  sm_close_r();
  if (buf[0] != 'X' || buf[1] != 'Y' || buf[2] != 'Z') blinkForever(100);

  // ── 成功: ゆっくり 3 回点滅 ──
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(400);
    digitalWrite(LED_PIN, LOW);  delay(400);
  }
}

void loop() {
  // 何もしない（結果は LED と TEST.TXT で確認）
}
