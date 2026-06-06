// i2c_size_test — Flash サイズ計測用（Wire.h）
// Wiremin_size_test と同じ処理を Wire.h で実装したもの。
// ビルド後の Flash 使用量を Wiremin 版と比較する。

#include <Wire.h>

#define SLAVE_ADDR  0x33
#define LED_PIN     2

uint8_t buf[4];

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Wire.begin();

  // write 4 bytes
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(buf, 4);
  Wire.endTransmission();

  // read 4 bytes
  Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)4);
  for (int i = 0; i < 4; i++) buf[i] = Wire.read();

  // probe
  Wire.beginTransmission(SLAVE_ADDR);
  bool ok = (Wire.endTransmission() == 0);
  digitalWrite(LED_PIN, ok ? HIGH : LOW);
}

void loop() {}
