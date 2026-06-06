// Wiremin_master_test — Wiremin 版マスターテスト
// Wire 版の i2c_master_test.ino に対応するサンプル。
//
// Wiremin_slave_test.ino と 2 台の UIAPduino でペアで使う。
// スレーブアドレス: 0x33
//
// LED 点滅パターン:
//   2 回点滅 (200ms) → 高速 (100ms) 送信成功
//   1 回点滅 (800ms) → 低速 (1000ms) 送信成功
//   5 回点滅  (50ms) → スレーブ未応答
//
// Wire 版との API 対応:
//   Wire.beginTransmission + write(&ms,4) + endTransmission
//     → Wiremin_write_reg(addr, 0, bytes, 4)
//   Wire.requestFrom + readBytes
//     → Wiremin_read_reg(addr, 0, buf, 4)
//
// 配線:
//   SDA: PC1 (D3) <-> PC1 (D3)
//   SCL: PC2 (D4) <-> PC2 (D4)
//   GND: GND      <-> GND
//   プルアップ: 4.7kΩ を SDA/SCL から 3.3V へ

#include <Wiremin.h>

#define SLAVE_ADDR  0x33
#define LED_PIN     2   // PC0 = LED_BUILTIN
#define REG_BLINK   0   // スレーブ共有レジスタ 0〜3 = blink ms (LE)

static void led(bool on) { digitalWrite(LED_PIN, on ? HIGH : LOW); }

static void blinkN(int n, int onMs, int offMs) {
  for (int i = 0; i < n; i++) {
    led(true);  delay(onMs);
    led(false); delay(offMs);
  }
}

static bool sendInterval(uint32_t ms) {
  uint8_t b[4] = {(uint8_t)ms, (uint8_t)(ms>>8), (uint8_t)(ms>>16), (uint8_t)(ms>>24)};
  return Wiremin_write_reg(SLAVE_ADDR, REG_BLINK, b, 4);
}

static uint32_t readInterval() {
  uint8_t b[4] = {};
  if (!Wiremin_read_reg(SLAVE_ADDR, REG_BLINK, b, 4)) return 0xFFFFFFFF;
  return (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  led(false);
  delay(500);
  Wiremin_begin();
}

void loop() {
  if (sendInterval(100)) {
    delay(200);
    (void)readInterval();
    blinkN(2, 200, 200);
  } else {
    blinkN(5, 50, 50);
  }
  delay(3000);

  if (sendInterval(1000)) {
    delay(200);
    (void)readInterval();
    blinkN(1, 800, 200);
  } else {
    blinkN(5, 50, 50);
  }
  delay(4000);
}
