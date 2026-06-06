// Wiremin_bmi270 — BMI270 加速度・ジャイロ 6軸読み取り
//
// 初期化にコンフィグファイル（8,192バイト）を使用。
// BMI270 I2C アドレス: 0x68 (SDO=GND) または 0x69 (SDO=VCC)
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
#include "bmi270_cfg.h"

#define BMI270_ADDR   0x68

// ── レジスタアドレス ─────────────────────────────────────────────────────────
#define REG_CHIP_ID       0x00
#define REG_ACC_DATA      0x0C  // 6バイト: ax_lo,ax_hi, ay_lo,ay_hi, az_lo,az_hi
#define REG_GYR_DATA      0x12  // 6バイト: gx_lo,gx_hi, gy_lo,gy_hi, gz_lo,gz_hi
#define REG_INTERNAL_STS  0x21
#define REG_ACC_CONF      0x40
#define REG_ACC_RANGE     0x41
#define REG_GYR_CONF      0x42
#define REG_GYR_RANGE     0x43
#define REG_INIT_CTRL     0x59
#define REG_INIT_ADDR_0   0x5B
#define REG_INIT_DATA     0x5E
#define REG_PWR_CONF      0x7C
#define REG_PWR_CTRL      0x7D
#define REG_CMD           0x7E

// ── コンフィグ書き込み（32バイト単位）──────────────────────────────────────
static bool bmi270_load_config() {
  uint8_t v;

  // アドバンスドパワーセーブ無効
  v = 0x00; Wiremin_write_reg(BMI270_ADDR, REG_PWR_CONF, &v, 1);
  delay(1);

  // 初期化開始
  v = 0x00; Wiremin_write_reg(BMI270_ADDR, REG_INIT_CTRL, &v, 1);

  // コンフィグファイルを 32バイト単位で書き込む
  for (uint16_t i = 0; i < sizeof(bmi270_config_file); i += 32) {
    uint8_t addr[2] = {
      (uint8_t)((i / 2) & 0x0F),
      (uint8_t)((i / 2) >> 4)
    };
    Wiremin_write_reg(BMI270_ADDR, REG_INIT_ADDR_0, addr, 2);
    Wiremin_write_reg(BMI270_ADDR, REG_INIT_DATA, &bmi270_config_file[i], 32);
  }

  // 初期化完了
  v = 0x01; Wiremin_write_reg(BMI270_ADDR, REG_INIT_CTRL, &v, 1);
  delay(20);

  // 初期化ステータス確認（0x21 の下位4ビット = 0x01 で成功）
  if (!Wiremin_read_reg(BMI270_ADDR, REG_INTERNAL_STS, &v, 1)) return false;
  return (v & 0x0F) == 0x01;
}

static bool bmi270_init() {
  uint8_t v;

  // チップID確認
  if (!Wiremin_read_reg(BMI270_ADDR, REG_CHIP_ID, &v, 1)) return false;
  if (v != 0x24) return false;

  // ソフトリセット
  v = 0xB6; Wiremin_write_reg(BMI270_ADDR, REG_CMD, &v, 1);
  delay(2);

  // コンフィグ書き込み
  if (!bmi270_load_config()) return false;

  // ACC: 100Hz, normal, ±8G
  v = 0xA8; Wiremin_write_reg(BMI270_ADDR, REG_ACC_CONF, &v, 1);
  v = 0x02; Wiremin_write_reg(BMI270_ADDR, REG_ACC_RANGE, &v, 1);

  // GYR: 100Hz, normal, ±2000dps
  v = 0xA8; Wiremin_write_reg(BMI270_ADDR, REG_GYR_CONF, &v, 1);
  v = 0x00; Wiremin_write_reg(BMI270_ADDR, REG_GYR_RANGE, &v, 1);

  // ACC + GYR 有効化
  v = 0x0E; Wiremin_write_reg(BMI270_ADDR, REG_PWR_CTRL, &v, 1);
  delay(100);

  return true;
}

// ── 出力ヘルパー（符号付き整数） ────────────────────────────────────────────
static void printInt(int16_t v) {
  if (v < 0) { hid.Print("-"); v = -v; }
  hid.Print((int)v);
}

void setup() {
  WebHID.begin();
  delay(7000);
  hid.Clear();
  hid.Println("BMI270 init...");

  Wiremin_begin();

  if (!bmi270_init()) {
    hid.Println("INIT FAILED");
    while (1) delay(1000);
  }
  hid.Println("OK");
}

void loop() {
  static unsigned long lastMs = 0;
  if (millis() - lastMs < 100) return;
  lastMs = millis();

  uint8_t raw[6];
  if (!Wiremin_read_reg(BMI270_ADDR, REG_ACC_DATA, raw, 6)) {
    hid.Println("READ ERR");
    return;
  }

  int16_t ax = (int16_t)((raw[1] << 8) | raw[0]);
  int16_t ay = (int16_t)((raw[3] << 8) | raw[2]);
  int16_t az = (int16_t)((raw[5] << 8) | raw[4]);

  hid.Print("ax:"); printInt(ax);
  hid.Print(",ay:"); printInt(ay);
  hid.Print(",az:"); printInt(az);
  hid.Print("\n");
}
