/**
 * WebHIDTest
 *
 * ボード:   HID ProMicro CH32V003 KBD+Mouse
 * バージョン: V1.4 + WebHID (EP3)
 *
 * 動作:
 *   - Web から Feature Report で受け取ったデータをそのまま
 *     EP3 Input Report で Web に返す (エコーバック)
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
    // Web からデータが届いていれば受信してエコーバック
    if (WebHID.available()) {
        uint8_t buf[16];
        uint8_t len = WebHID.recv(buf, sizeof(buf));
        // 先頭 8 バイトを EP3 で返す
        WebHID.send(buf, (len < 8) ? len : 8);
    }

    // 1秒ごとにカウンタを送信
    static uint32_t last = 0;
    if (millis() - last >= 1000) {
        last = millis();
        WebHID.send(counter++, 0, 0, 0, 0, 0, 0, 0);
    }
}
