/**
 * Hid.h  —  Hid クラス宣言
 *
 * ─── Print プロトコル（マーカー 0x50）───────────────────────────────────────
 *  UIAPduino → ブラウザ  (EP3 Input Report, 8 bytes)
 *    byte[0] = 0x50  マーカー
 *    byte[1] = flags   0x80=続きあり  0x02=改行  0x04=画面クリア
 *    byte[2..7] = テキスト本体（最大 6 文字、残りは 0 埋め）
 *
 * ─── Recv プロトコル（ブラウザ → UIAPduino）─────────────────────────────────
 *  ブラウザ: sendFeatureReport(0, data)  →  EP0 Feature Report, 最大 16 bytes
 *  UIAPduino: hid.Recv(buf, maxLen)  →  WebHID.recv() のラッパー
 *
 * ─── GetPos プロトコル（マーカー 0x51）──────────────────────────────────────
 *  UIAPduino → ブラウザ  (EP3 Input Report, 8 bytes)
 *    [0x51, 0x01, 0, 0, 0, 0, 0, 0]      ← GetPos リクエスト
 *  ブラウザ → UIAPduino  (EP0 Feature Report, 16 bytes)
 *    [0x51, 0x01, xLow, xHigh, yLow, yHigh, 0, …]
 * ─────────────────────────────────────────────────────────────────────────
 *
 * 使い方（Print）:
 *   hid.Clear();
 *   hid.Println("=== Hello ===");
 *   hid.Print("count="); hid.Println(n);
 *
 * 使い方（Recv）:
 *   uint8_t buf[16];
 *   uint8_t len = hid.Recv(buf, sizeof(buf));
 *   if (len > 0) { ... }
 */

#ifndef HID_H
#define HID_H

#include <stdint.h>

// ── GetPos プロトコル定数 ──────────────────────────────────────────────────
#define HID_QUERY_MARKER 0x51
#define HID_QUERY_POS 0x01

// ── Hid クラス ──────────────────────────────────────────────────────────────
class Hid {
public:
  // ── HID コンソールへ出力（EP3 → ブラウザ）──────────────────────────────
  void Print(const char* s);         // 文字列（改行なし、6 文字超は自動分割）
  void Print(int v);                 // 整数（改行なし）
  void Println(const char* s = "");  // 文字列 + 改行
  void Println(int v);               // 整数 + 改行
  void Clear();                      // コンソールをクリア

  // ── ブラウザからの受信（EP0 Feature Report）───────────────────────────
  // WebHID.recv() のラッパー。hid-print.html の「送信」ボタンで送ったデータを受け取る。
  // 戻り値: 受信バイト数（0 = データなし）
  uint8_t Recv(uint8_t* buf, uint8_t maxLen);

  // ── カーソル座標取得（hid-print.html では未使用）──────────────────────
  bool GetPos(int16_t& x, int16_t& y);

private:
  void _hpSend(uint8_t flags, const char* s, uint8_t n);
  void _request(uint8_t queryType);
};

// グローバルインスタンスの宣言（実体は Hid.cpp で定義）
extern Hid hid;

#endif
