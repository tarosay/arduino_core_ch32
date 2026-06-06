// Wiremin_BMP280 — Wiremin 版 BMP280 温度・気圧センサ読み取り
// Wire 版の i2c_BMP280_test.ino に対応するサンプル。
//
// Wire 版との API 対応:
//   Wire.beginTransmission + write(reg) + write(val) + endTransmission
//     → Wiremin_write_reg(addr, reg, &val, 1)
//   Wire.beginTransmission + write(reg) + endTransmission(false)
//   + Wire.requestFrom + Wire.read × n
//     → Wiremin_read_reg(addr, reg, buf, n)   ← repeated START 内蔵
//
// 配線:
//   UIAPduino          BMP280
//   PC1 (D3, SDA) ─── SDA
//   PC2 (D4, SCL) ─── SCL
//   3.3V          ─── VCC
//   GND           ─── GND
//   ※ SDA/SCL に 4.7kΩ プルアップ抵抗（to 3.3V）が必要

#include <Wiremin.h>
#include <WebHID.h>
#include "Hid.h"

#define BMP280_ADDR       0x76
#define BMP280_REG_CALIB  0x88
#define BMP280_REG_CTRL   0xF4
#define BMP280_REG_DATA   0xF7

static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t  t_fine;

static bool readN(uint8_t reg, uint8_t *buf, uint8_t len) {
  return Wiremin_read_reg(BMP280_ADDR, reg, buf, len);
}
static void write8(uint8_t reg, uint8_t val) {
  Wiremin_write_reg(BMP280_ADDR, reg, &val, 1);
}

static uint16_t u16le(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static int16_t  s16le(const uint8_t *p) { return (int16_t)u16le(p); }

static bool readCalibration() {
  uint8_t b[24];
  if (!readN(BMP280_REG_CALIB, b, 24)) return false;
  dig_T1 = u16le(&b[0]);  dig_T2 = s16le(&b[2]);  dig_T3 = s16le(&b[4]);
  dig_P1 = u16le(&b[6]);  dig_P2 = s16le(&b[8]);  dig_P3 = s16le(&b[10]);
  dig_P4 = s16le(&b[12]); dig_P5 = s16le(&b[14]); dig_P6 = s16le(&b[16]);
  dig_P7 = s16le(&b[18]); dig_P8 = s16le(&b[20]); dig_P9 = s16le(&b[22]);
  return dig_T1 != 0 && dig_P1 != 0;
}

static int32_t compensateTemp100(int32_t adc_T) {
  int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  int32_t var2 = (((((adc_T >> 4) - (int32_t)dig_T1) * ((adc_T >> 4) - (int32_t)dig_T1)) >> 12) * (int32_t)dig_T3) >> 14;
  t_fine = var1 + var2;
  return (t_fine * 5 + 128) >> 8;
}

static uint32_t compensatePressurePa(int32_t adc_P) {
  int32_t var1 = (t_fine >> 1) - 64000;
  int32_t var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)dig_P6;
  var2 = var2 + ((var1 * (int32_t)dig_P5) << 1);
  var2 = (var2 >> 2) + ((int32_t)dig_P4 << 16);
  var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + (((int32_t)dig_P2 * var1) >> 1)) >> 18;
  var1 = ((32768 + var1) * (int32_t)dig_P1) >> 15;
  if (var1 == 0) return 0;
  uint32_t p = (((uint32_t)((int32_t)1048576 - adc_P) - (uint32_t)(var2 >> 12))) * 3125UL;
  if (p < 0x80000000UL) p = (p << 1) / (uint32_t)var1;
  else p = (p / (uint32_t)var1) * 2UL;
  var1 = ((int32_t)dig_P9 * (int32_t)(((p >> 3) * (p >> 3)) >> 13)) >> 12;
  var2 = ((int32_t)(p >> 2) * (int32_t)dig_P8) >> 13;
  p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
  return p;
}

static void printTemp100(int32_t t100) {
  if (t100 < 0) { hid.Print("-"); t100 = -t100; }
  hid.Print((int)(t100 / 100));
  hid.Print(".");
  int frac = (int)(t100 % 100);
  if (frac < 10) hid.Print("0");
  hid.Print(frac);
}

static void printPressurePa(uint32_t pa) {
  hid.Print((int)(pa / 100));
  hid.Print(".");
  int frac = (int)(pa % 100);
  if (frac < 10) hid.Print("0");
  hid.Print(frac);
}

void setup() {
  WebHID.begin();
  delay(7000);

  hid.Clear();
  hid.Println("BMP280 temp pressure");

  Wiremin_begin();

  if (!readCalibration()) {
    hid.Println("CAL ERR");
    while (1) delay(1000);
  }

  write8(BMP280_REG_CTRL, 0x27);
  delay(100);
}

void loop() {
  static unsigned long lastMs = 0;
  if (millis() - lastMs < 250) return;
  lastMs = millis();

  uint8_t b[6];
  if (!readN(BMP280_REG_DATA, b, 6)) {
    hid.Println("READ ERR");
    delay(1000);
    return;
  }

  int32_t adc_P = ((int32_t)b[0] << 12) | ((int32_t)b[1] << 4) | (b[2] >> 4);
  int32_t adc_T = ((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | (b[5] >> 4);

  int32_t  t100    = compensateTemp100(adc_T);
  uint32_t pressPa = compensatePressurePa(adc_P);

  hid.Print("T:");
  printTemp100(t100);
  hid.Print("C P:");
  printPressurePa(pressPa);
  hid.Println("hPa");
}
