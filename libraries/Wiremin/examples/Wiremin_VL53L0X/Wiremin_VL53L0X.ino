/**
 * Wiremin_VL53L0X — VL53L0X（TOF センサ）の距離を HID Graph に表示
 *
 * ボード: HID ProMicro CH32V003
 * USB:   WebHID Only（Tools → USB）
 * 対応ページ: hid-graph.html
 *
 * VL53L0X の距離を 50 ms ごとに「dist[mm]:値」の形で送ります。
 * 測定できなかったとき（8190 が返る）とタイムアウト時は「nodata」を送ります。
 *
 * I2C は Wiremin を使います（Wire.h は使いません）。
 * VL53L0X ライブラリは Wire.h 専用なので、Wiremin で動くように直したものを
 * このフォルダの VL53L0Xmin.h に入れています。
 * 配線: SDA → PC1、SCL → PC2
 */

#include <Wiremin.h>
#include "VL53L0Xmin.h"
#include <WebHID.h>
#include "Hid.h"

#define PERIOD_MS   50     // 距離を送る間隔 [ms]
#define MM_NODATA   8190   // 測定できなかったときに返る値

VL53L0X sensor;
static uint32_t lastMs;

void setup() {
  WebHID.begin();

  hid.WaitAvailable();  // ブラウザ接続待ち
  hid.Clear();          // HID Graph の記録を消して、0 から始める

  Wiremin_begin();  // I2C 100 kHz
  sensor.setTimeout(500);
  if (!sensor.init()) {
    hid.Println("INIT ERROR");
    while (1) delay(1000);
  }
  sensor.startContinuous(PERIOD_MS);

  lastMs = millis();
}

void loop() {
  uint32_t now = millis();
  if ((uint32_t)(now - lastMs) < PERIOD_MS) return;
  lastMs = now;

  uint16_t mm = sensor.readRangeContinuousMillimeters();
  if (sensor.timeoutOccurred() || mm >= MM_NODATA) {
    hid.Println("nodata");
  } else {
    hid.Print("dist[mm]:");
    hid.Println((int)mm);
  }
}
