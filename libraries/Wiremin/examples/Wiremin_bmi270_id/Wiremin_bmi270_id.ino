// Wiremin_bmi270_id — BMI270 チップID読み出し
//
// BMI270 の I2C アドレス: 0x68 (SDO=GND) または 0x69 (SDO=VCC)
// レジスタ 0x00 (CHIP_ID) を読み出す。期待値: 0x24
//
// 配線:
//   UIAPduino          BMI270
//   PC1 (D3, SDA) ─── SDA
//   PC2 (D4, SCL) ─── SCL
//   3.3V          ─── VCC, SDO(→GND で 0x68)
//   GND           ─── GND
//   プルアップ: 4.7kΩ を SDA/SCL から 3.3V へ

#include <Wiremin.h>
#include <WebHID.h>
#include "Hid.h"

#define BMI270_ADDR  0x68   // SDO=GND: 0x68 / SDO=VCC: 0x69
#define REG_CHIP_ID  0x00   // 期待値: 0x24

static void printHex(uint8_t v) {
  const char *hex = "0123456789ABCDEF";
  char buf[5] = {'0','x', hex[(v>>4)&0xF], hex[v&0xF], 0};
  hid.Print(buf);
}

void setup() {
  WebHID.begin();
  delay(5000);
  hid.Clear();
  hid.Println("BMI270 chip ID test");

  Wiremin_begin();

  // チップID読み出し
  uint8_t id = 0;
  if (Wiremin_read_reg(BMI270_ADDR, REG_CHIP_ID, &id, 1)) {
    hid.Print("CHIP_ID: ");
    printHex(id);
    hid.Println(id == 0x24 ? " -> OK" : " -> NG (expected 0x24)");
  } else {
    hid.Println("I2C error (device not found?)");
  }
}

void loop() {}
