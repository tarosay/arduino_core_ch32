// Wiremin_EEPROM_24FC256 — I2C シリアル EEPROM の書き込み／読み出しテスト
//
// 対象: Microchip 24FC256（32KB = 256kbit）※ 24LC256 / AT24C256 も同じ手順
//   デバイスアドレス : 0x50（A0/A1/A2 = GND）
//   メモリアドレス   : 16bit（0x0000–0x7FFF）→ Wiremin_write_reg16 / Wiremin_read_reg16
//   ページサイズ     : 64 バイト（境界をまたぐ書き込みはチップ内で先頭に折り返す）
//   書き込みサイクル : 最大 5 ms（完了まで NACK を返すので ACK ポーリングで待つ）
//
// 配線:
//   SDA: PC1 (D3)   SCL: PC2 (D4)
//   プルアップ: 4.7kΩ を SDA/SCL から Vcc へ
//   A0/A1/A2, WP: GND   VCC: Vcc   VSS: GND
//
// ⚠ このスケッチは EEPROM の 0x0000 / 0x0040–0x0083 / 0x0100–0x0103 / 0x7FFF を
//    書き換えます。データを保存済みのチップでは実行しないでください。
//
// 実行するとテストが 1 回だけ走ります。もう一度見るにはリセットしてください。
// テスト 8 のブートカウンタは電源を切っても保持されます（不揮発の確認）。

#include <Wiremin.h>
#include <WebHID.h>
#include "Hid.h"

static const uint8_t  EEP_ADDR = 0x50;    // デバイスアドレス
static const uint8_t  EEP_PAGE = 64;      // ページサイズ（バイト）
static const uint16_t EEP_LAST = 0x7FFF;  // 最終アドレス（32KB - 1）

// ── EEPROM ヘルパ ───────────────────────────────────────────────────────────
// ページ境界の分割と書き込み完了待ちは「スケッチ側の仕事」。
// Wiremin はバス転送だけを担当する。

// 書き込みサイクルの完了待ち（ACK ポーリング）。
// 書き込み中のチップはアドレスに応答しないので、ACK が返れば完了。
// Wiremin_probe() のタイムアウトが約 5 ms あるため、これ自体が待ち時間になる。
static bool eepReady(void) {
  for (uint8_t i = 0; i < 20; i++)
    if (Wiremin_probe(EEP_ADDR)) return true;
  return false;
}

// ページ境界で分割しながら書き込む。
// 分割しないと、境界を越えた分がそのページの先頭に上書きされる。
static bool eepWrite(uint16_t addr, const uint8_t *data, uint16_t len) {
  while (len) {
    uint16_t n = EEP_PAGE - (addr % EEP_PAGE);  // このページに残るバイト数
    if (n > len) n = len;
    if (!Wiremin_write_reg16(EEP_ADDR, addr, data, (uint8_t)n)) return false;
    if (!eepReady()) return false;
    addr += n;
    data += n;
    len  -= n;
  }
  return true;
}

// 読み出しはページ境界の制約なし（チップ内でアドレスが自動で進む）。
static inline bool eepRead(uint16_t addr, uint8_t *buf, uint8_t len) {
  return Wiremin_read_reg16(EEP_ADDR, addr, buf, len);
}

// ── 表示ヘルパ ──────────────────────────────────────────────────────────────

static void printHex8(uint8_t v) {
  const char *hex = "0123456789ABCDEF";
  char b[3] = { hex[(v >> 4) & 0xF], hex[v & 0xF], 0 };
  hid.Print(b);
}

static void printHex16(uint16_t v) {
  printHex8((uint8_t)(v >> 8));
  printHex8((uint8_t)v);
}

static uint8_t ngCount = 0;

static void result(const char *name, bool ok) {
  hid.Print(name);
  hid.Println(ok ? " OK" : " NG");
  if (!ok) ngCount++;
}

static bool same(const uint8_t *a, const uint8_t *b, uint8_t n) {
  for (uint8_t i = 0; i < n; i++)
    if (a[i] != b[i]) return false;
  return true;
}

// ── テスト本体 ──────────────────────────────────────────────────────────────

static uint8_t wbuf[EEP_PAGE];
static uint8_t rbuf[EEP_PAGE];

static void runTests(void) {
  bool ok;
  uint8_t v, r;

  // 1) デバイス検出
  result("1 probe", Wiremin_probe(EEP_ADDR));

  // 2) 1 バイト書き込み → 読み出し（16bit アドレス）
  v = 0xA5; r = 0x00;
  ok = eepWrite(0x0000, &v, 1) && eepRead(0x0000, &r, 1) && (r == 0xA5);
  result("2 byte ", ok);

  // 3) 1 ページ（64 バイト）書き込み → 一括読み出し
  for (uint8_t i = 0; i < EEP_PAGE; i++) wbuf[i] = (uint8_t)(i * 3 + 7);
  ok = eepWrite(0x0040, wbuf, EEP_PAGE)
    && eepRead(0x0040, rbuf, EEP_PAGE)
    && same(wbuf, rbuf, EEP_PAGE);
  result("3 page ", ok);

  // 4) ページ境界（0x0080）をまたぐ 8 バイト書き込み
  //    分割していなければ後半 4 バイトが 0x0040 側に折り返し、ここで NG になる。
  for (uint8_t i = 0; i < 8; i++) wbuf[i] = (uint8_t)(0xE0 + i);
  ok = eepWrite(0x007C, wbuf, 8)
    && eepRead(0x007C, rbuf, 8)
    && same(wbuf, rbuf, 8)
    && eepRead(0x0040, rbuf, 4)          // 折り返していないことの確認
    && rbuf[0] == 7 && rbuf[3] == 16;
  result("4 cross", ok);

  // 5) シーケンシャル読み出し（アドレス指定なしの続き読み = Wiremin_read）
  //    0x0040 から 4 バイト読んだ直後、チップ内のアドレスは 0x0044。
  ok = eepRead(0x0040, rbuf, 4) && Wiremin_read(EEP_ADDR, rbuf + 4, 4);
  for (uint8_t i = 0; ok && i < 8; i++)
    ok = (rbuf[i] == (uint8_t)(i * 3 + 7));
  result("5 seq  ", ok);

  // 6) 最終アドレス 0x7FFF（16bit アドレスが効いていることの確認）
  v = 0x5A; r = 0x00;
  ok = eepWrite(EEP_LAST, &v, 1) && eepRead(EEP_LAST, &r, 1) && (r == 0x5A);
  result("6 last ", ok);

  // 7) len == 0 の読み出しは false（バスを触らない）
  result("7 zlen ", !Wiremin_read(EEP_ADDR, rbuf, 0));

  // 8) ブートカウンタ — 電源を切っても保持される（不揮発の確認）
  //    0x0100: 'E' 'P' countLow countHigh
  uint16_t count = 0;
  if (eepRead(0x0100, rbuf, 4) && rbuf[0] == 'E' && rbuf[1] == 'P')
    count = (uint16_t)rbuf[2] | ((uint16_t)rbuf[3] << 8);
  count++;
  wbuf[0] = 'E'; wbuf[1] = 'P';
  wbuf[2] = (uint8_t)count; wbuf[3] = (uint8_t)(count >> 8);
  result("8 boot ", eepWrite(0x0100, wbuf, 4));
  hid.Print("  run count = ");
  hid.Println((int)count);

  // 先頭 16 バイトのダンプ（0x0040 から）
  hid.Print("dump ");
  printHex16(0x0040);
  hid.Println(":");
  if (eepRead(0x0040, rbuf, 16)) {
    for (uint8_t i = 0; i < 16; i++) {
      printHex8(rbuf[i]);
      hid.Print(" ");
    }
    hid.Println();
  }
}

void setup() {
  WebHID.begin();
  delay(5000);  // ブラウザが接続するまで待つ

  hid.Clear();
  hid.Println("24FC256 EEPROM test");
  hid.Println("SDA=PC1(D3) SCL=PC2(D4)");
  hid.Println("---");

  Wiremin_begin();  // 100kHz（5V 駆動なら Wiremin_begin_fast() で 400kHz も可）

  runTests();

  hid.Println("---");
  if (ngCount == 0) hid.Println("ALL PASS");
  else { hid.Print("FAIL: "); hid.Print((int)ngCount); hid.Println(" test(s)"); }
  hid.Println("(reset to re-run)");
}

void loop() {}
