/*
 * NeoPixelmin_ring.ino — WS2812B 12連リングのサンプル
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
 * インストール済みのコアを見なくなる。バージョンを書かなければ解決できずに失敗し、
 * 書けば index から取り直したコアでビルドされる。どちらにしても、手元で直した
 * ライブラリを実機で試せない。
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

void setup() {
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(50);  // 明るさ 0～255
  pixels.show();
}

void loop() {
  // 赤色を順番に点灯
  for (int i = 0; i < LED_COUNT; i++) {
    pixels.clear();
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
    delay(20);
  }

  // 緑色を順番に点灯
  for (int i = 0; i < LED_COUNT; i++) {
    pixels.clear();
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    pixels.show();
    delay(20);
  }

  // 青色を順番に点灯
  for (int i = 0; i < LED_COUNT; i++) {
    pixels.clear();
    pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    pixels.show();
    delay(20);
  }

  // 全体を明るく → 暗く
  for (int i = 10; i < 256; i++) {
    for (int j = 0; j < LED_COUNT; j++) {
      pixels.setPixelColor(j, pixels.Color(i, i, i));
    }
    pixels.show();
    delay(2);
  }
  for (int i = 255; i >= 10; i--) {
    for (int j = 0; j < LED_COUNT; j++) {
      pixels.setPixelColor(j, pixels.Color(i, i, i));
    }
    pixels.show();
    delay(2);
  }
}
