// Wiremin_size_test — Flash サイズ計測用（Wiremin.h）
// i2c_size_test と同じ処理を Wiremin.h で実装したもの。
// ビルド後の Flash 使用量を Wire 版と比較する。

#include <Wiremin.h>

#define SLAVE_ADDR  0x33
#define LED_PIN     2

uint8_t buf[4];

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Wiremin_begin();

  // write 4 bytes
  Wiremin_write(SLAVE_ADDR, buf, 4);

  // read 4 bytes
  Wiremin_read(SLAVE_ADDR, buf, 4);

  // probe
  bool ok = Wiremin_probe(SLAVE_ADDR);
  digitalWrite(LED_PIN, ok ? HIGH : LOW);
}

void loop() {}
