// Wiremin_EEPROM_CAT24M01 — 1Mbit I2C シリアル EEPROM の書き込み／読み出しテスト
//
// 対象: onsemi CAT24M01（128KB = 1Mbit）※ ABLIC S-24CM01C もプロトコル同一
//   デバイスアドレス : 1010 A2 A1 a16 ← 最下位ビットはピンではなく「アドレスの 17bit 目」
//                      このスケッチは A1=HIGH / A2=LOW 前提
//                      → 0x52（下位 64KB）/ 0x53（上位 64KB）
//                      A0 ピンは無い（1 バスに最大 4 個）
//   メモリアドレス   : 17bit（0x00000–0x1FFFF）下位 16bit を Wiremin_*_reg16 で送る
//   ページサイズ     : 256 バイト（境界をまたぐ書き込みはチップ内で先頭に折り返す）
//   書き込みサイクル : 最大 5 ms（完了まで NACK を返すので ACK ポーリングで待つ）
//
// 配線:
//   SDA: PC1 (D3)   SCL: PC2 (D4)
//   プルアップ: 4.7kΩ を SDA/SCL から Vcc へ
//   A1: Vcc   A2: GND   WP: GND   VCC: Vcc   VSS: GND
//   ※ CAT24M01 の A1/A2/WP は内部プルダウンが強めなので GND/Vcc へ直結すること
//
// ⚠ このスケッチは EEPROM の 0x00000 / 0x00100–0x0017F / 0x001FC–0x00203 /
//    0x00300–0x00303 / 0x00400 / 0x00500–0x00507 / 0x005FC–0x005FF /
//    0x0FFFC–0x10003 / 0x10400 / 0x1FFFF を書き換えます。
//    データを保存済みのチップでは実行しないでください。
//
// 実行するとテストが 1 回だけ走ります。もう一度見るにはリセットしてください。
// テスト 10 のブートカウンタは電源を切っても保持されます（不揮発の確認）。

#include <Wiremin.h>
#include <WebHID.h>
#include "Hid.h"

static const uint8_t  EEP_BASE  = 0x52;       // 1010 A2(0) A1(1) → a16 を足して 0x52 / 0x53
static const uint16_t EEP_PAGE  = 256;        // ページサイズ（バイト）
static const uint8_t  EEP_CHUNK = 128;        // 1 転送の上限（Wiremin の len は uint8_t）
static const uint32_t EEP_LAST  = 0x1FFFFUL;  // 最終アドレス（128KB - 1）

// ── EEPROM ヘルパ ───────────────────────────────────────────────────────────
// 分割と書き込み完了待ちは「スケッチ側の仕事」。Wiremin はバス転送だけを担当する。
// CAT24M01 では分割の理由が 3 つある。
//   (a) 256B ページ境界 — またぐとページ先頭に折り返す（書き込みのみ）
//   (b) 64K 境界        — a16 はデバイスアドレス側なのでチップ内で繰り上がらない
//   (c) len が uint8_t  — 1 転送 255 バイトが上限。ここでは 128 で切ってページを半分ずつ扱う

// アドレスの 17bit 目（a16）をデバイスアドレスの最下位に載せる。
static inline uint8_t eepDev(uint32_t addr) {
  return (uint8_t)(EEP_BASE | ((addr >> 16) & 1));
}

// 書き込みサイクルの完了待ち（ACK ポーリング）。
// 書き込み中のチップはアドレスに応答しないので、ACK が返れば完了。
// Wiremin_probe() のタイムアウトが約 5 ms あるため、これ自体が待ち時間になる。
static bool eepReady(uint8_t dev) {
  for (uint8_t i = 0; i < 20; i++)
    if (Wiremin_probe(dev)) return true;
  return false;
}

// ページ境界で分割しながら書き込む。
// 1 ページ（256B）は 64K の約数なので、ページで切れば 64K 境界も自動的に守られる。
static bool eepWrite(uint32_t addr, const uint8_t *data, uint16_t len) {
  while (len) {
    uint16_t n = (uint16_t)(EEP_PAGE - (addr & (EEP_PAGE - 1)));  // このページに残るバイト数
    if (n > len)       n = len;
    if (n > EEP_CHUNK) n = EEP_CHUNK;
    uint8_t dev = eepDev(addr);
    if (!Wiremin_write_reg16(dev, (uint16_t)addr, data, (uint8_t)n)) return false;
    if (!eepReady(dev)) return false;
    addr += n;
    data += n;
    len  -= n;
  }
  return true;
}

// 読み出しはページ境界の制約は無いが、64K 境界では切る必要がある。
// シーケンシャル読み出しは 0x0FFFF の次に 0x00000 へ戻る（a16 は繰り上がらない）。
static bool eepRead(uint32_t addr, uint8_t *buf, uint16_t len) {
  while (len) {
    uint32_t n = 0x10000UL - (addr & 0xFFFFUL);  // この 64K ブロックに残るバイト数
    if (n > len)       n = len;
    if (n > EEP_CHUNK) n = EEP_CHUNK;
    if (!Wiremin_read_reg16(eepDev(addr), (uint16_t)addr, buf, (uint8_t)n)) return false;
    addr += n;
    buf  += n;
    len  -= n;
  }
  return true;
}

// ── 表示ヘルパ ──────────────────────────────────────────────────────────────

static const char HEXCH[] = "0123456789ABCDEF";

static void printHex8(uint8_t v) {
  char b[3] = { HEXCH[(v >> 4) & 0xF], HEXCH[v & 0xF], 0 };
  hid.Print(b);
}

static void printAddr(uint32_t a) {  // 17bit を 5 桁で
  char b[2] = { HEXCH[(a >> 16) & 0xF], 0 };
  hid.Print(b);
  printHex8((uint8_t)(a >> 8));
  printHex8((uint8_t)a);
}

static uint8_t ngCount = 0;

static void result(const char *name, bool ok) {
  hid.Print(name);
  hid.Println(ok ? " OK" : " NG");
  if (!ok) ngCount++;
}

static bool same(const uint8_t *a, const uint8_t *b, uint16_t n) {
  for (uint16_t i = 0; i < n; i++)
    if (a[i] != b[i]) return false;
  return true;
}

// ── テスト本体 ──────────────────────────────────────────────────────────────

static uint8_t wbuf[EEP_CHUNK];
static uint8_t rbuf[EEP_CHUNK];

static void runTests(void) {
  bool ok;
  uint8_t v, r;

  // 1) デバイス検出 — 0x52/0x53 が応答し、0x50 と 0x54 は応答しないこと。
  //    ここが NG なら A1/A2 の配線かプルアップを疑う。
  ok = Wiremin_probe(0x52) && Wiremin_probe(0x53)
    && !Wiremin_probe(0x50) && !Wiremin_probe(0x54);
  result("1 probe", ok);

  // 2) 1 バイト書き込み → 読み出し
  v = 0xA5; r = 0x00;
  ok = eepWrite(0x00000UL, &v, 1) && eepRead(0x00000UL, &r, 1) && (r == 0xA5);
  result("2 byte ", ok);

  // 3) 128 バイト（半ページ）書き込み → 一括読み出し
  for (uint8_t i = 0; i < EEP_CHUNK; i++) wbuf[i] = (uint8_t)(i * 3 + 7);
  ok = eepWrite(0x00100UL, wbuf, EEP_CHUNK)
    && eepRead(0x00100UL, rbuf, EEP_CHUNK)
    && same(wbuf, rbuf, EEP_CHUNK);
  result("3 block", ok);

  // 4) ページ境界（0x00200）をまたぐ 8 バイト書き込み。
  //    分割していなければ後半 4 バイトが 0x00100 側に折り返し、ここで NG になる。
  for (uint8_t i = 0; i < 8; i++) wbuf[i] = (uint8_t)(0xE0 + i);
  ok = eepWrite(0x001FCUL, wbuf, 8)
    && eepRead(0x001FCUL, rbuf, 8)
    && same(wbuf, rbuf, 8)
    && eepRead(0x00100UL, rbuf, 4)        // 折り返していないことの確認
    && rbuf[0] == 7 && rbuf[3] == 16;
  result("4 cross", ok);

  // 5) ページサイズが 256 バイトであることの確認（分割せずに書いて折り返しを見る）。
  //    0x005FC から 8 バイトを 1 転送で書くと、後半 4 バイトはページ先頭 0x00500 に載る。
  //    64B ページの品種なら折り返し先が 0x005C0 になるので、ここで NG になる。
  for (uint8_t i = 0; i < 8; i++) wbuf[i] = 0x00;
  ok = eepWrite(0x00500UL, wbuf, 8);
  for (uint8_t i = 0; i < 8; i++) wbuf[i] = (uint8_t)(0xC0 + i);
  ok = ok
    && Wiremin_write_reg16(eepDev(0x005FCUL), 0x05FC, wbuf, 8)  // わざと分割しない
    && eepReady(eepDev(0x005FCUL))
    && eepRead(0x005FCUL, rbuf, 4) && same(wbuf, rbuf, 4)       // 前半はそのまま
    && eepRead(0x00500UL, rbuf, 4) && same(wbuf + 4, rbuf, 4);  // 後半が先頭へ折り返す
  result("5 wrap ", ok);

  // 6) 64K 境界（0x10000）をまたぐ書き込み。
  //    a16 はデバイスアドレスなので、分割して 0x52 → 0x53 に切り替える必要がある。
  //    読み戻しは境界をまたがないよう 2 回に分けて、書き込み側だけを見る。
  for (uint8_t i = 0; i < 8; i++) wbuf[i] = (uint8_t)(0x31 + i);
  ok = eepWrite(0x0FFFCUL, wbuf, 8)
    && eepRead(0x0FFFCUL, rbuf, 4)
    && eepRead(0x10000UL, rbuf + 4, 4)
    && same(wbuf, rbuf, 8);
  result("6 w64k ", ok);

  // 7) 64K 境界をまたぐ読み出しを 1 回の eepRead で。
  //    分割していなければ 0x0FFFF の次が 0x00000 に戻り、後半 4 バイトが化ける。
  for (uint8_t i = 0; i < 8; i++) rbuf[i] = 0x00;
  ok = eepRead(0x0FFFCUL, rbuf, 8) && same(wbuf, rbuf, 8);
  result("7 r64k ", ok);

  // 8) 最終アドレス 0x1FFFF（17bit アドレスが端まで届いていることの確認）
  v = 0x5A; r = 0x00;
  ok = eepWrite(EEP_LAST, &v, 1) && eepRead(EEP_LAST, &r, 1) && (r == 0x5A);
  result("8 last ", ok);

  // 9) 上位 64KB が下位 64KB とは別の領域であることの確認（a16 が効いているか）。
  //    64KB 品などに読み替えられていると同じ番地を指し、両者が一致して NG になる。
  v = 0x11; ok = eepWrite(0x00400UL, &v, 1);
  v = 0x22; ok = ok && eepWrite(0x10400UL, &v, 1);
  ok = ok && eepRead(0x00400UL, rbuf, 1)
          && eepRead(0x10400UL, rbuf + 1, 1)
          && rbuf[0] == 0x11 && rbuf[1] == 0x22;
  result("9 a16  ", ok);

  // 10) ブートカウンタ — 電源を切っても保持される（不揮発の確認）
  //     0x00300: 'C' 'M' countLow countHigh
  uint16_t count = 0;
  if (eepRead(0x00300UL, rbuf, 4) && rbuf[0] == 'C' && rbuf[1] == 'M')
    count = (uint16_t)rbuf[2] | ((uint16_t)rbuf[3] << 8);
  count++;
  wbuf[0] = 'C'; wbuf[1] = 'M';
  wbuf[2] = (uint8_t)count; wbuf[3] = (uint8_t)(count >> 8);
  result("10 boot", eepWrite(0x00300UL, wbuf, 4));
  hid.Print("  run count = ");
  hid.Println((int)count);

  // 64K 境界をまたぐ 16 バイトのダンプ（0x0FFF8 から）
  hid.Print("dump ");
  printAddr(0x0FFF8UL);
  hid.Println(":");
  if (eepRead(0x0FFF8UL, rbuf, 16)) {
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
  hid.Println("CAT24M01 EEPROM test");
  hid.Println("SDA=PC1(D3) SCL=PC2(D4)");
  hid.Println("dev 0x52/53 A1=H A2=L");
  hid.Println("---");

  Wiremin_begin();  // 100kHz（5V 駆動なら Wiremin_begin_fast() で 400kHz も可）

  runTests();

  hid.Println("---");
  if (ngCount == 0) hid.Println("ALL PASS");
  else { hid.Print("FAIL: "); hid.Print((int)ngCount); hid.Println(" test(s)"); }
  hid.Println("(reset to re-run)");
}

void loop() {}
