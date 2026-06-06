// Wiremin_read_reg — レジスタ読み書きデモ（BMP280 使用）
// Wiremin_read_reg()（repeated START）と Wiremin_write_reg() の動作確認用。

#include <Wiremin.h>
#include <WebHID.h>
#include "Hid.h"

#define BMP280_ADDR  0x76
#define REG_ID       0xD0  // 期待値: 0x60

static void printHex(uint8_t v) {
  const char *hex = "0123456789ABCDEF";
  char buf[5] = {'0','x', hex[(v>>4)&0xF], hex[v&0xF], 0};
  hid.Print(buf);
}

void setup() {
  WebHID.begin();
  delay(5000);
  hid.Clear();

  Wiremin_begin();

  // Chip ID 読み出し
  uint8_t id = 0;
  if (Wiremin_read_reg(BMP280_ADDR, REG_ID, &id, 1)) {
    hid.Print("Chip ID: ");
    printHex(id);
    hid.Println(id == 0x60 ? " OK" : " unexpected");
  } else {
    hid.Println("Device not found");
  }

  // 6 バイトバースト読み出し（生データ 0xF7〜0xFC）
  uint8_t raw[6] = {};
  if (Wiremin_read_reg(BMP280_ADDR, 0xF7, raw, 6)) {
    hid.Print("Raw:");
    for (uint8_t i = 0; i < 6; i++) {
      hid.Print(" ");
      printHex(raw[i]);
    }
    hid.Println();
  }
}

void loop() {}
