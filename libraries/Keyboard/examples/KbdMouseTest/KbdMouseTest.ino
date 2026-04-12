/**
 * KbdMouseTest
 *
 * HID ProMicro CH32V003 KBD+Mouse ボード用の動作確認スケッチ。
 *
 * 動作:
 *   - setup: USB 接続後、"Hello UIAPduino!\n" とタイプする
 *   - loop:  5秒ごとにマウスを右に 20px 動かす
 *
 * ボード選択: HID ProMicro CH32V003 KBD+Mouse
 */

#include <Keyboard.h>
#include <Mouse.h>

void setup() {
  Keyboard.begin();   // Mouse.begin() は省略可 (共有 USB バス)
  delay(3000);        // USB 接続待ち

  // "Hello UIAPduino!" とタイプ
  Keyboard.println("Hello UIAPduino!");
}

void loop() {
  delay(5000);

  // マウスを右に 20px 動かす
  Mouse.move(20, 0);

  delay(500);

  // マウスを元の位置に戻す
  Mouse.move(-20, 0);
}
