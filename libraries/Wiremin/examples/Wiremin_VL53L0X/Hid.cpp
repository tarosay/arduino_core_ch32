#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

// グローバルインスタンスの実体
Hid hid;

// ── Print プロトコル内部定数（0x50）──────────────────────────────────────────
#define _HP_MARKER  0x50
#define _HP_MORE    0x80
#define _HP_CLEAR   0x04

// ════════════════════════════════════════════════════════════════════════════
// Print 系（0x50）
// ════════════════════════════════════════════════════════════════════════════

void Hid::_hpSend(uint8_t flags, const char* s, uint8_t n) {
  uint8_t buf[8] = { _HP_MARKER, flags, 0, 0, 0, 0, 0, 0 };
  for (uint8_t i = 0; i < n; i++) buf[i + 2] = (uint8_t)s[i];
  WebHID.send(buf, 8);
  delay(12);
}

void Hid::Print(const char* s) {
  int len = strlen(s), off = 0;
  if (!len) return;
  while (off < len) {
    int n = len - off;
    if (n > 6) n = 6;
    _hpSend((off + n < len) ? _HP_MORE : 0, s + off, n);
    off += n;
  }
}

void Hid::Print(int v) {
  char b[12];
  itoa(v, b, 10);
  Print(b);
}

void Hid::Println(const char* s) {
  Print(s);
  Print("\n");  // 0x0A をペイロードに載せて送信
}

void Hid::Println(int v) {
  char b[12];
  itoa(v, b, 10);
  Println(b);
}

void Hid::Clear() {
  _hpSend(_HP_CLEAR, "", 0);
}

// ════════════════════════════════════════════════════════════════════════════
// Write — バイナリ送信（0x52）
// ════════════════════════════════════════════════════════════════════════════
void Hid::Write(const uint8_t* buf, uint8_t len) {
  uint8_t off = 0;
  while (off < len) {
    uint8_t n = len - off;
    if (n > 5) n = 5;
    uint8_t pkt[8] = { 0x52, (uint8_t)((off + n < len) ? 0x80 : 0x00), n, 0, 0, 0, 0, 0 };
    for (uint8_t i = 0; i < n; i++) pkt[i + 3] = buf[off + i];
    WebHID.send(pkt, 8);
    delay(12);
    off += n;
  }
}

// ════════════════════════════════════════════════════════════════════════════
// Ready — 準備完了通知（0x53）
// ════════════════════════════════════════════════════════════════════════════
void Hid::Ready() {
  uint8_t pkt[8] = { 0x53, 0, 0, 0, 0, 0, 0, 0 };
  WebHID.send(pkt, 8);
  delay(12);
}

// ════════════════════════════════════════════════════════════════════════════
// Send — Raw 送信
// ════════════════════════════════════════════════════════════════════════════
void Hid::Send(const uint8_t* buf, uint8_t len) {
  WebHID.send(buf, len);
  delay(12);
}

// ════════════════════════════════════════════════════════════════════════════
// Recv — Raw 受信（Feature Report）
// ════════════════════════════════════════════════════════════════════════════
uint8_t Hid::Recv(uint8_t* buf, uint8_t maxLen) {
  return WebHID.recv(buf, maxLen);
}

// ════════════════════════════════════════════════════════════════════════════
// WaitAvailable — ブラウザ接続待ち（データを消費しない）
// ════════════════════════════════════════════════════════════════════════════
bool Hid::WaitAvailable(uint32_t timeoutMs) {
  unsigned long t0 = millis();
  while (true) {
    if (WebHID.available() > 0) return true;
    if (timeoutMs > 0 && millis() - t0 >= timeoutMs) return false;
    delay(10);
  }
}

// ════════════════════════════════════════════════════════════════════════════
// GetPos（0x51）
// ════════════════════════════════════════════════════════════════════════════
void Hid::_request(uint8_t queryType) {
  uint8_t buf[8] = { HID_QUERY_MARKER, queryType, 0, 0, 0, 0, 0, 0 };
  WebHID.send(buf, 8);
  delay(12);
}

bool Hid::GetPos(int16_t& x, int16_t& y) {
  _request(HID_QUERY_POS);
  unsigned long t0 = millis();
  uint8_t buf[16];
  while (millis() - t0 < 500) {
    uint8_t len = WebHID.recv(buf, sizeof(buf));
    if (len >= 6 && buf[0] == HID_QUERY_MARKER && buf[1] == HID_QUERY_POS) {
      x = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
      y = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
      return true;
    }
    delay(5);
  }
  return false;
}
