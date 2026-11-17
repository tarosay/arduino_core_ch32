/**
 * Hid.h  —  Hid クラス宣言（HidPrint 版）
 *
 * ─── Print プロトコル（マーカー 0x50）───────────────────────────────────────
 *  UIAPduino → ブラウザ  (Input Report, 8 bytes)
 *    byte[0] = 0x50  マーカー
 *    byte[1] = flags   0x80=続きあり  0x04=画面クリア
 *    byte[2..7] = テキスト本体（最大 6 文字、残りは 0 埋め）
 *    ※ 改行は 0x0A をテキストに含めて送信（NL フラグは使用しない）
 *
 * ─── Write プロトコル（マーカー 0x52）──────────────────────────────────────
 *  UIAPduino → ブラウザ  (Input Report, 8 bytes)
 *    byte[0] = 0x52  マーカー
 *    byte[1] = flags   0x80=続きあり
 *    byte[2] = ペイロード長（0..5）
 *    byte[3..7] = バイナリペイロード
 *
 * ─── Ready プロトコル（マーカー 0x53）──────────────────────────────────────
 *  UIAPduino → ブラウザ  (Input Report, 8 bytes)
 *    byte[0] = 0x53  マーカー（準備完了通知）
 *    byte[1..7] = 0x00  予約
 *
 * ─── GetPos プロトコル（マーカー 0x51）──────────────────────────────────────
 *  UIAPduino → ブラウザ  (Input Report, 8 bytes)
 *    [0x51, 0x01, 0, 0, 0, 0, 0, 0]        ← GetPos リクエスト
 *  ブラウザ → UIAPduino  (Feature Report, 32 bytes)
 *    [0x51, 0x01, xLow, xHigh, yLow, yHigh, 0, …]
 *
 * ─── Recv（ブラウザ → UIAPduino）────────────────────────────────────────────
 *  ブラウザ: sendFeatureReport(0, data)  →  Feature Report, 最大 32 bytes
 *  UIAPduino: hid.Recv(buf, maxLen)
 *
 * ─── Send（Raw 送信）────────────────────────────────────────────────────────
 *  UIAPduino → ブラウザ  (Input Report, len bytes)
 *  マーカーなし。バイト列をそのまま送信する。
 * ─────────────────────────────────────────────────────────────────────────
 */

#ifndef HID_H
#define HID_H

#include <stdint.h>

// ── GetPos プロトコル定数 ──────────────────────────────────────────────────
#define HID_QUERY_MARKER 0x51
#define HID_QUERY_POS    0x01

// ── Hid クラス ──────────────────────────────────────────────────────────────
class Hid {
public:
  // ── テキスト送信 Print プロトコル（0x50）────────────────────────────────
  void Print(const char* s);         // テキスト送信（改行なし）
  void Print(int v);                 // 整数送信（改行なし）
  void Println(const char* s = ""); // テキスト送信 + 改行（0x0A）
  void Println(int v);               // 整数送信 + 改行（0x0A）
  void Clear();                      // 画面クリア

  // ── バイナリ送信 Write プロトコル（0x52）────────────────────────────────
  // サイズフィールド付きでバイナリデータを送信（0x00 を含むデータも正確に転送可能）
  void Write(const uint8_t* buf, uint8_t len);

  // ── 準備完了通知 Ready プロトコル（0x53）────────────────────────────────
  // 初期化完了後にブラウザへ通知する
  void Ready();

  // ── Raw 送信（マーカーなし）───────────────────────────────────────────────
  // バイト列をそのまま Input Report として送信する
  void Send(const uint8_t* buf, uint8_t len);

  // ── Raw 受信（Feature Report）────────────────────────────────────────────
  // 戻り値: 受信バイト数（0 = データなし）
  uint8_t Recv(uint8_t* buf, uint8_t maxLen);

  // ── ブラウザ接続待ち ──────────────────────────────────────────────────────
  // ブラウザから Feature Report が届くまでブロックする（データは消費しない）。
  // timeoutMs = 0 で無限待ち。タイムアウト時は false を返す。
  bool WaitAvailable(uint32_t timeoutMs = 0);

  // ── カーソル座標取得（0x51）───────────────────────────────────────────────
  bool GetPos(int16_t& x, int16_t& y);

private:
  void _hpSend(uint8_t flags, const char* s, uint8_t n);
  void _request(uint8_t queryType);
};

// グローバルインスタンスの宣言
extern Hid hid;

#endif
