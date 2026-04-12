/*
 * AutoSave.ino - 定期的に CTRL+S を送信して保存忘れを防ぐ
 *
 * Board : HID ProMicro CH32V003 KBD+Mouse (Board Version: V1.4)
 * 動作  : SAVE_INTERVAL_SEC 秒ごとに CTRL+S を1回送信する
 *         送信直前に LED が3回点滅して通知する
 *
 * ボードを USB に繋いで、エディタやアプリを前面にした状態で使う。
 */

#include <Keyboard.h>

// ---- 設定 -------------------------------------------------------
#define SAVE_INTERVAL_SEC  30      // CTRL+S を送る間隔 (秒)
#define LED_PIN            GPIO_PIN_2   // 基板上の LED (PC0)
// -----------------------------------------------------------------

static void blink(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(80);
        digitalWrite(LED_PIN, LOW);
        delay(80);
    }
}

static void sendCtrlS() {
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('s');
    delay(30);
    Keyboard.releaseAll();
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Keyboard.begin();

    // 起動を知らせる長め点滅
    blink(5);
    delay(2000);   // PC が認識するまで少し待つ
}

void loop() {
    static uint32_t lastSave = 0;

    if (millis() - lastSave >= (uint32_t)SAVE_INTERVAL_SEC * 1000UL) {
        lastSave = millis();

        // 送信前に3回点滅して予告
        blink(3);
        delay(200);

        sendCtrlS();

        // 送信完了を1回点灯で確認
        digitalWrite(LED_PIN, HIGH);
        delay(300);
        digitalWrite(LED_PIN, LOW);
    }
}
