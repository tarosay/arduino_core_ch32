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

// EP3 ポーリング間隔は 10ms。15ms 待てば確実に次のポーリングが来る。
#define EP3_WAIT_MS  15

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

        uint8_t sent = 0;
        while (sent < len) {
            uint8_t chunk = (len - sent > 8) ? 8 : (len - sent);
            WebHID.send(buf + sent, chunk);
            sent += chunk;
            // 次のチャンクを送る前に EP3 ポーリングを待つ
            if (sent < len) delay(EP3_WAIT_MS);
        }
    }

    // 1秒ごとにカウンタを送信
    static uint32_t last = 0;
    if (millis() - last >= 1000) {
        last = millis();
        WebHID.send(counter++, 0, 0, 0, 0, 0, 0, 0);
    }
}
