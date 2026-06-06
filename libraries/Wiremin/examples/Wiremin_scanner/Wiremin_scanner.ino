// Wiremin_scanner — Wiremin 版 I2C アドレススキャナー
// Wire 版の i2c_scanner.ino に対応するサンプル。
//
// Wire 版との API 対応:
//   Wire.beginTransmission + Wire.endTransmission → Wiremin_probe(addr)
//   Wire.end() + Wire.begin()                    → Wiremin_end() + Wiremin_begin()
//
// 配線:
//   SDA: PC1 (D3)
//   SCL: PC2 (D4)
//   プルアップ: 4.7kΩ を SDA/SCL から 3.3V へ

#include <Wiremin.h>
#include <WebHID.h>
#include "Hid.h"

static void printHex(uint8_t v) {
  const char *hex = "0123456789ABCDEF";
  char buf[5] = {'0','x', hex[(v>>4)&0xF], hex[v&0xF], 0};
  hid.Print(buf);
}

void setup() {
  WebHID.begin();
  delay(5000);

  hid.Clear();
  hid.Println("Wiremin I2C Scanner");
  hid.Println("SDA=PC1(D3) SCL=PC2(D4)");
  hid.Println("---");

  uint32_t scanCount = 0;

  while (true) {
    // サイクルごとに I2C をリセット（連続 NACK によるロックアップを防止）
    Wiremin_end();
    Wiremin_begin();

    scanCount++;
    uint8_t count = 0;

    for (uint8_t addr = 8; addr < 120; addr++) {
      if (Wiremin_probe(addr)) {
        hid.Print("Found: ");
        printHex(addr);
        hid.Println();
        count++;
      }
    }

    hid.Print("Scan ");
    hid.Print((int)scanCount);
    hid.Print(": Found ");
    hid.Print(count);
    hid.Println(" device(s).");
    hid.Println("---");

    delay(1000);
  }
}

void loop() {}
