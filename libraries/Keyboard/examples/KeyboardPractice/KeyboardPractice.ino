/**
 * KeyboardPractice
 *
 * ボード:   HID ProMicro CH32V003 KBD+Mouse
 * バージョン: V1.4 + WebHID (EP3)
 *
 * 使い方:
 *   https://tarosay.github.io/uiap-hid-web/keyboard.html を開き、
 *   「実行」ボタンを押してワークエリアにフォーカスを当ててから実行する。
 *
 *   練習したいステップのコードブロックだけコメント(//)を外して
 *   Arduino IDE から書き込む。
 *
 * ポイント:
 *   特殊キー（矢印・BackSpace・Enter・Home など）は
 *   Keyboard.write() を使う。
 *   write() は内部で press → 10ms 待機 → release → 50ms 待機 を行うので
 *   USB ポーリング（10ms）に確実にキャッチされる。
 *   さらにスケッチ側でも delay(50) を入れると、
 *   連続する特殊キー操作の取りこぼしを防げる。
 */

#include <WebHID.h>
#include <Keyboard.h>

void setup() {
  WebHID.begin();
  Keyboard.begin();
  delay(2000);    // USB 接続が安定するまで待つ
}

void loop() {
  uint8_t buf[16];
  uint8_t len = WebHID.recv(buf, sizeof(buf));

  if (len > 0) {
    delay(500);   // ブラウザがワークエリアにフォーカスを移すまで待つ

    // ─── Step 1: "Hello UIAPduino World." を入力する ─────────────────
    Keyboard.print("Hello UIAPduino World.");

    // ─── Step 2: カーソルを 6 個左に動かして W を w に修正する ──────
    // Keyboard.print("Hello UIAPduino World.");
    // delay(100);
    // for (int i = 0; i < 6; i++) {  // 6 回繰り返す
    //   Keyboard.write(KEY_LEFT_ARROW);
    //   delay(50);
    // }
    // Keyboard.write(KEY_DELETE);  // "W" を削除
    // delay(50);
    // Keyboard.print("w");  // 小文字 "w" を入力

    // ─── Step 3: Backspace で "!" を削除する ─────────────────────────
    // Keyboard.print("UIAPduino!");
    // Keyboard.write(KEY_BACKSPACE);  // "!" を削除

    // ─── Step 4: Enter キーで改行する ────────────────────────────────
    // Keyboard.print("Hello,");
    // Keyboard.write(KEY_RETURN);  // 改行
    // delay(50);
    // Keyboard.print("UIAPduino!");

    // ─── Step 5: Home キーで行頭に戻り文字を挿入する ─────────────────
    // Keyboard.print("UIAPduino <<");
    // delay(100);
    // Keyboard.write(KEY_HOME);  // 行頭へ移動
    // delay(50);
    // Keyboard.print(">> ");  // 行頭に挿入
  }
}
