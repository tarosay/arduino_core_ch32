#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

// グローバルインスタンスの実体
Hid hid;

// ── Print プロトコル内部定数 ──────────────────────────────────────────────────
#define _HP_MARKER 0x50
#define _HP_MORE 0x80
#define _HP_NL 0x02
#define _HP_CLEAR 0x04

// ════════════════════════════════════════════════════════════════════════════
// Print 系（EP3 → ブラウザ）
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
  int len = strlen(s), off = 0;
  if (!len) {
    _hpSend(_HP_NL, "", 0);
    return;
  }
  while (off < len) {
    int n = len - off;
    if (n > 6) n = 6;
    bool last = (off + n >= len);
    _hpSend(last ? _HP_NL : _HP_MORE, s + off, n);
    off += n;
  }
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
// Recv — ブラウザからの受信（WebHID.recv のラッパー）
// ════════════════════════════════════════════════════════════════════════════
uint8_t Hid::Recv(uint8_t* buf, uint8_t maxLen) {
  return WebHID.recv(buf, maxLen);
}

// ════════════════════════════════════════════════════════════════════════════
// GetPos（hid-print.html では未使用。他スケッチとの Hid.h 互換性のため実装）
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
