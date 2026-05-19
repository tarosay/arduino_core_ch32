/**
 * i2c_BMP280_test.ino  —  BMP280 温度・気圧センサ読み取りサンプル
 *
 * 対象ボード : UIAPduino (CH32V003F4, UIAP_HID v1.2.1 以降)
 * センサ     : BMP280 (I2C アドレス 0x76)
 *
 * 配線:
 *   UIAPduino          BMP280
 *   PC1 (D3, SDA) ─── SDA
 *   PC2 (D4, SCL) ─── SCL
 *   3.3V          ─── VCC
 *   GND           ─── GND
 *   ※ SDA/SCL に 4.7kΩ プルアップ抵抗（to 3.3V）が必要
 *
 * 動作:
 *   - 起動後 7 秒待機（WebHID 接続を待つ）
 *   - BMP280 のキャリブレーションデータを読み込む
 *   - 250ms ごとに温度（℃）・気圧（hPa）・高度（m）を HID コンソールへ出力
 *
 * 出力例（WebHID Lab / hid-print.html で確認）:
 *   BMP280 temp pressure
 *   T20:20 T35:35 温度T:25.13 気圧P:1013.25
 *   T20:20 T35:35 温度T:25.14 気圧P:1013.24
 *   ※ "T20:20 T35:35" はシリアルプロッタのスケール固定用ダミー値
 *
 * 高度計算について:
 *   BMP280 の気圧値から高度を求めることは可能だが、
 *   <math.h> の powf() を使用すると CH32V003F4 の 16KB フラッシュを
 *   超過するため、このサンプルでは実装していない。
 *   計算式（気圧高度計算式）:
 *     altitude = 44330 × (1 − (P / P0)^0.1903)
 *     P0 = 101325 Pa（標準海面気圧）
 *   フラッシュ容量に余裕があるボードでは <math.h> を include し
 *   上記式を powf() で実装できる。
 */

#include <Wire.h>
#include <WebHID.h>
#include "Hid.h"

// ── BMP280 I2C アドレス ────────────────────────────────────────────────────
// SDO ピンを GND に接続した場合 0x76、VCC に接続した場合 0x77
#define BMP280_ADDR       0x76

// ── BMP280 レジスタアドレス ───────────────────────────────────────────────
#define BMP280_REG_CALIB     0x88  // キャリブレーションデータ先頭（24 バイト）
#define BMP280_REG_CTRL_MEAS 0xF4  // 測定モード設定
#define BMP280_REG_PRESS_MSB 0xF7  // 気圧・温度データ先頭（6 バイト）

// ── 温度補正係数（BMP280 固有の値、起動時に読み込む） ─────────────────────
static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;

// ── 気圧補正係数 ──────────────────────────────────────────────────────────
static uint16_t dig_P1;
static int16_t  dig_P2;
static int16_t  dig_P3;
static int16_t  dig_P4;
static int16_t  dig_P5;
static int16_t  dig_P6;
static int16_t  dig_P7;
static int16_t  dig_P8;
static int16_t  dig_P9;

// 温度・気圧補正計算で共有される中間値
static int32_t t_fine;

// ── I2C ヘルパー関数 ──────────────────────────────────────────────────────

// 指定レジスタに 1 バイト書き込む
static void write8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// 指定レジスタから len バイト連続読み込む（リピートスタート使用）
static bool readN(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(BMP280_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {  // false = リピートスタート
    return false;
  }
  if (Wire.requestFrom(BMP280_ADDR, len) != len) {
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }
  return true;
}

// BMP280 はリトルエンディアン（LSB 先）でデータを格納する
static uint16_t u16le(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static int16_t s16le(const uint8_t *p) {
  return (int16_t)u16le(p);
}

// ── キャリブレーションデータ読み込み ─────────────────────────────────────
// BMP280 内部の NVM から補正係数を取得する（電源投入ごとに 1 回だけ実行）
static bool readCalibration() {
  uint8_t b[24];
  if (!readN(BMP280_REG_CALIB, b, 24)) {
    return false;
  }
  dig_T1 = u16le(&b[0]);
  dig_T2 = s16le(&b[2]);
  dig_T3 = s16le(&b[4]);

  dig_P1 = u16le(&b[6]);
  dig_P2 = s16le(&b[8]);
  dig_P3 = s16le(&b[10]);
  dig_P4 = s16le(&b[12]);
  dig_P5 = s16le(&b[14]);
  dig_P6 = s16le(&b[16]);
  dig_P7 = s16le(&b[18]);
  dig_P8 = s16le(&b[20]);
  dig_P9 = s16le(&b[22]);

  // dig_T1 と dig_P1 が 0 ならセンサ未接続と判断
  return dig_T1 != 0 && dig_P1 != 0;
}

// ── 温度補正計算（BMP280 データシート 4.2.3 準拠） ─────────────────────────
// 戻り値: 0.01℃ 単位（例: 3217 → 32.17℃）
// 副作用: t_fine を更新（気圧補正で使用）
static int32_t compensateTemp100(int32_t adc_T) {
  int32_t var1 =
    ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  int32_t var2 =
    (((((adc_T >> 4) - ((int32_t)dig_T1)) *
       ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
     ((int32_t)dig_T3)) >> 14;
  t_fine = var1 + var2;
  return (t_fine * 5 + 128) >> 8;
}

// ── 気圧補正計算（BMP280 データシート 4.2.3 準拠） ─────────────────────────
// 事前に compensateTemp100() を呼び t_fine を更新しておくこと
// 戻り値: Pa 単位（例: 101325 → 1013.25 hPa）
static uint32_t compensatePressurePa(int32_t adc_P) {
  int32_t var1 = (t_fine >> 1) - 64000;
  int32_t var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
  var2 = var2 + ((var1 * ((int32_t)dig_P5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)dig_P4) << 16);
  var1 =
    (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
     ((((int32_t)dig_P2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((int32_t)dig_P1)) >> 15);
  if (var1 == 0) return 0;  // ゼロ除算防止

  uint32_t p =
    (((uint32_t)(((int32_t)1048576) - adc_P) - (uint32_t)(var2 >> 12))) * 3125UL;
  if (p < 0x80000000UL) {
    p = (p << 1) / ((uint32_t)var1);
  } else {
    p = (p / (uint32_t)var1) * 2UL;
  }
  var1 =
    (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
  var2 = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;
  p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
  return p;
}

// ── HID 出力ヘルパー ──────────────────────────────────────────────────────

// 温度を "XX.XX" 形式で出力（0.01℃ 単位の値を受け取る）
static void printTemp100(int32_t t100) {
  if (t100 < 0) {
    hid.Print("-");
    t100 = -t100;
  }
  hid.Print((int)(t100 / 100));
  hid.Print(".");
  int frac = (int)(t100 % 100);
  if (frac < 10) hid.Print("0");
  hid.Print(frac);
}

// 気圧を "XXXX.XX" 形式で出力（Pa 単位の値を受け取る）
static void printPressurePa(uint32_t pa) {
  hid.Print((int)(pa / 100));
  hid.Print(".");
  int frac = (int)(pa % 100);
  if (frac < 10) hid.Print("0");
  hid.Print(frac);
}

// ── Arduino エントリポイント ──────────────────────────────────────────────

void setup() {
  WebHID.begin();
  delay(7000);  // WebHID 接続待ち（ブラウザで HID デバイスを開くまでの余裕）

  hid.Clear();
  hid.Println("BMP280 temp pressure");

  Wire.begin();  // I2C マスターとして初期化（SDA=PC1, SCL=PC2）

  // キャリブレーションデータ読み込み
  if (!readCalibration()) {
    hid.Println("CAL ERR");  // センサ未接続またはアドレス違い
    while (1) delay(1000);
  }

  // 測定モード設定: osrs_t x1（温度 1 回平均）, osrs_p x1（気圧 1 回平均）, ノーマルモード
  write8(BMP280_REG_CTRL_MEAS, 0x27);
  delay(100);  // 最初の測定完了待ち
}

void loop() {
  uint8_t b[6];

  // 250ms 間隔で出力
  static unsigned long lastMillis = 0;
  unsigned long now = millis();
  if (now - lastMillis < 250) return;
  lastMillis = now;

  // 気圧(3バイト) + 温度(3バイト) を一括読み込み（レジスタ 0xF7〜0xFC）
  if (!readN(BMP280_REG_PRESS_MSB, b, 6)) {
    hid.Println("READ ERR");
    delay(1000);
    return;
  }

  // ADC 生データ（20bit）を復元
  int32_t adc_P = ((int32_t)b[0] << 12) | ((int32_t)b[1] << 4) | ((int32_t)b[2] >> 4);
  int32_t adc_T = ((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | ((int32_t)b[5] >> 4);

  // 補正計算（温度を先に計算して t_fine を更新してから気圧を計算する）
  int32_t  t100    = compensateTemp100(adc_T);
  uint32_t pressPa = compensatePressurePa(adc_P);

  // HID コンソールへ出力
  // "T20:20 T35:35" はシリアルプロッタのスケール固定用ダミー値
  hid.Print("T20:20 T35:35 温度T:");
  printTemp100(t100);
  hid.Print(" 気圧P:");
  printPressurePa(pressPa);
  hid.Println("");
}
