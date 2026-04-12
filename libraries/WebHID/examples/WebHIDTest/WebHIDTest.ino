/**
 * WebHIDTest
 *
 * ボード:   HID ProMicro CH32V003 KBD+Mouse
 * バージョン: V1.4 + WebHID (EP3)
 *
 * 動作:
 *   - Web から Feature Report で受け取ったデータを
 *     8 バイトずつ分割して EP3 Input Report で全バイト返す (エコーバック)
 *   - 1秒ごとにカウンタ値を EP3 で送信
 *
 * Chrome/Edge で WebHID API を使って接続してください。
 */

#include <WebHID.h>

uint8_t counter = 0;

void setup() {
    WebHID.begin();
    delay(2000);  // USB 接続待ち
}

void loop() {
    // Web からデータが届いていれば受信して全バイトをエコーバック
    if (WebHID.available()) {
        uint8_t buf[16];
        uint8_t len = WebHID.recv(buf, sizeof(buf));

        // 8 バイトずつ分割して送信
        uint8_t sent = 0;
        while (sent < len) {
            // 前の送信が完了するまで待つ
            while (WebHID.busy()) {}
            uint8_t chunk = (len - sent > 8) ? 8 : (len - sent);
            WebHID.send(buf + sent, chunk);
            sent += chunk;
        }
    }

    // 1秒ごとにカウンタを送信
    static uint32_t last = 0;
    if (millis() - last >= 1000) {
        last = millis();
        // 前の送信が完了していれば送る
        if (!WebHID.busy()) {
            WebHID.send(counter++, 0, 0, 0, 0, 0, 0, 0);
        }
    }
}
