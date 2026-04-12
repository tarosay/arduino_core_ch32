#pragma once
#include <stdint.h>
#include "uiapusb.h"

#ifndef UIAP_WEBHID
#error "WebHID ライブラリを使うには: Tools > Board Version Select > V1.4 + WebHID (EP3) を選択してください"
#endif

/**
 * WebHID ライブラリ
 *
 * ボード選択: HID ProMicro CH32V003 KBD+Mouse  →  Board Version: V1.4 + WebHID (EP3)
 *
 * UIAPduino → Web : send(buf, len)        EP3 Input Report (8 bytes)
 * Web → UIAPduino : available() / recv()  EP0 Feature Report (最大 16 bytes)
 */
class WebHIDClass {
public:
    void begin() { uiapusb_begin(); }

    // UIAPduino → Web へ 8 bytes 送信 (EP3 Input Report)
    void send(const uint8_t *buf, uint8_t len = 8) {
        uiapwebhid_send(buf, len);
    }

    // 個別バイト指定版 (最大8バイト)
    void send(uint8_t b0, uint8_t b1=0, uint8_t b2=0, uint8_t b3=0,
              uint8_t b4=0, uint8_t b5=0, uint8_t b6=0, uint8_t b7=0) {
        uint8_t buf[8] = {b0, b1, b2, b3, b4, b5, b6, b7};
        uiapwebhid_send(buf, 8);
    }

    // Web からデータが届いているか (Feature Report)
    uint8_t available() { return uiapwebhid_available(); }

    // Web からデータを受け取る (Feature Report, 最大 16 bytes)
    uint8_t recv(uint8_t *buf, uint8_t maxlen = 16) {
        return uiapwebhid_recv(buf, maxlen);
    }
};

extern WebHIDClass WebHID;
