// Wiremin_slave_test — Wiremin 版スレーブテスト
// Wire 版の i2c_slave_test.ino に対応するサンプル。
//
// Wiremin_master_test.ino と 2 台の UIAPduino でペアで使う。
// スレーブアドレス: 0x33
//
// 動作:
//   共有レジスタ [0..3] に LED 点滅間隔 (ms, LE) が書き込まれる。
//   ISR が自動的にレジスタを更新するため、ループは読み出すだけでよい。
//
// Wire 版との API 対応:
//   Wire.begin(addr) + onReceive(cb) + onRequest(cb)
//     → Wiremin_slave_begin(addr)  ← ISR が自動処理、コールバック不要
//   Wire.read() (コールバック内)
//     → Wiremin_slave_get(reg)
//   Wire.write(&val, 4) (コールバック内)
//     → Wiremin_slave_set(reg, val) で値を置いておく (ISR が自動送信)
//
// 配線:
//   SDA: PC1 (D3) <-> PC1 (D3)
//   SCL: PC2 (D4) <-> PC2 (D4)
//   GND: GND      <-> GND
//   プルアップ: 4.7kΩ を SDA/SCL から 3.3V へ

#include <Wiremin.h>

#define MY_ADDR   0x33
#define LED_PIN   2       // PC0 = LED_BUILTIN
#define REG_BLINK 0       // レジスタ 0〜3 = blink ms (uint32_t LE)

// 共有レジスタ [0..3] から uint32_t (LE) を読む
static uint32_t getBlinkMs() {
  uint32_t ms = (uint32_t)Wiremin_slave_get(0)
              | ((uint32_t)Wiremin_slave_get(1) << 8)
              | ((uint32_t)Wiremin_slave_get(2) << 16)
              | ((uint32_t)Wiremin_slave_get(3) << 24);
  if (ms == 0 || ms > 60000) ms = 500;  // デフォルト 500ms
  return ms;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // デフォルト 500ms をレジスタに書いておく
  Wiremin_slave_set(0, 500 & 0xFF);
  Wiremin_slave_set(1, (500 >> 8) & 0xFF);
  Wiremin_slave_set(2, 0);
  Wiremin_slave_set(3, 0);
  Wiremin_slave_begin(MY_ADDR);
}

void loop() {
  static uint32_t lastMs = millis();
  static bool ledState = false;

  uint32_t blinkMs = getBlinkMs();
  if (millis() - lastMs >= blinkMs) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastMs = millis();
  }
}
