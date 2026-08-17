/*
 * NeoPixelmin_getPixels.ino — getPixels() で画素バッファを直接さわるサンプル
 *
 * 配線:
 *   WS2812B DIN → pin 8 (PC6 / SPI1 MOSI)  ※このピンに固定
 *   WS2812B VCC → 5V(または3.3V)、GND → GND
 *
 * Tools の設定:
 *   Board Version : V1.4
 *   USB           : WebHID Only
 *   Optimize      : Smallest (-Os) with LTO
 *   FQBN 表記なら UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,opt=oslto
 *
 * sketch.yaml は置いていない。置くと arduino-cli も IDE も profile build に入り、
 * インストール済みのコアを見なくなる。
 *
 * ── getPixels() は何のためにあるか ──────────────────────────────────────────
 *   回転や減光は、やりたいことが「バイト列の操作」であって、色として解釈し直す
 *   必要がない。getPixelColor() で 32bit に組み立てて setPixelColor() で分解し
 *   直すと、その往復だけで数百バイトの Flash を使う。16KB しかないこの石では
 *   それが効いてくる。getPixels() はバッファをそのまま渡して往復を省くための
 *   もの。
 *
 * ── 使うときの約束 ──────────────────────────────────────────────────────────
 *   - バイト並びは色順に従う。NEO_GRB なら G, R, B の順で、getPixelColor() の
 *     ような R, G, B への並べ替えはされない
 *   - 入っているのは輝度スケール前の素の値。setBrightness() は非破壊で、
 *     スケーリングは show() の中で行われる
 *   - 有効なのは numPixels() * 3 バイトまで。境界チェックは無いので、範囲は
 *     呼び出し側の責任になる
 *
 * ── 動き ────────────────────────────────────────────────────────────────────
 *   フェーズの間は 0.4 秒 消灯して区切る。
 *
 *    1. 左右一致（赤）  0〜5 を setPixelColor、6〜11 を getPixels で書く
 *       → 12個すべて同じ赤になる。両者が同じ結果になることの確認
 *       → 右半分が緑に見えたらバイト順の取り違え（G,R,B 順である）
 *    2. 左右一致（青）  同上、青で
 *    3. 回転  赤1点を getPixels でバイト回転させる → ドットが1周する
 *    4. 減光  白から getPixels で徐々に減光する
 */

// LEDの個数（バッファサイズもこれに合わせて確保する）
#define LED_COUNT 12
#define NEOPIXELMIN_MAX_LEDS LED_COUNT

#include <NeoPixelmin.h>

// WS2812BのDINを接続する端子番号（SPI1 MOSI 固定）
#define LED_PIN NEOPIXELMIN_PIN

NeoPixelmin pixels(
  LED_COUNT,
  LED_PIN,
  NEO_GRB + NEO_KHZ800);

// NEO_GRB の生バッファ内の並び。ここを間違えると色が入れ替わる。
#define OFF_G 0
#define OFF_R 1
#define OFF_B 2

// getPixels() 経由で 1 画素書く（setPixelColor と同じ結果になる）
static void rawSet(uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
  uint8_t *p = pixels.getPixels() + i * 3u;
  p[OFF_R] = r;
  p[OFF_G] = g;
  p[OFF_B] = b;
}

static void blank(void) {
  pixels.clear();
  pixels.show();
  delay(400);
}

// 左半分を setPixelColor、右半分を getPixels で同じ色にする
static void halfAndHalf(uint8_t r, uint8_t g, uint8_t b) {
  pixels.clear();
  for (uint16_t i = 0; i < LED_COUNT / 2; i++) pixels.setPixelColor(i, r, g, b);
  for (uint16_t i = LED_COUNT / 2; i < LED_COUNT; i++) rawSet(i, r, g, b);
  pixels.show();
  delay(2000);
}

// バッファをバイト単位で 1 画素ぶん回す
static void rawShift(void) {
  uint8_t *p = pixels.getPixels();
  uint16_t m = (uint16_t)(pixels.numPixels() * 3u);
  uint8_t t0 = p[m - 3], t1 = p[m - 2], t2 = p[m - 1];
  for (uint16_t i = m - 1; i >= 3u; i--) p[i] = p[i - 3];
  p[0] = t0;
  p[1] = t1;
  p[2] = t2;
}

// k は 0..256 の倍率。256 で等倍、それ未満で暗くなる
static void rawDim(uint16_t k) {
  uint8_t *p = pixels.getPixels();
  uint16_t m = (uint16_t)(pixels.numPixels() * 3u);
  for (uint16_t i = 0; i < m; i++) p[i] = (uint8_t)(((uint16_t)p[i] * k) >> 8);
}

void setup() {
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(60);  // 明るさ 0～255
  pixels.show();
  delay(500);
}

void loop() {
  // 1. 左右一致（赤）— setPixelColor と getPixels で同じ色になる
  halfAndHalf(255, 0, 0);
  blank();

  // 2. 左右一致（青）
  halfAndHalf(0, 0, 255);
  blank();

  // 3. 回転 — 赤1点が1周する
  pixels.clear();
  pixels.setPixelColor(0, 255, 0, 0);
  pixels.show();
  delay(300);
  for (uint8_t n = 0; n < LED_COUNT; n++) {
    rawShift();
    pixels.show();
    delay(120);
  }
  blank();

  // 4. 減光 — 白から滑らかに消える
  for (uint16_t i = 0; i < LED_COUNT; i++) pixels.setPixelColor(i, 255, 255, 255);
  pixels.show();
  delay(300);
  for (uint8_t n = 0; n < 40; n++) {
    rawDim(230);
    pixels.show();
    delay(50);
  }
  blank();
  delay(600);
}
