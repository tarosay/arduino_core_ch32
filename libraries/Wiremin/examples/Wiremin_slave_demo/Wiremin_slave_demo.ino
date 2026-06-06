/*
 * Wiremin_slave_demo — Wiremin slave mode demo
 *
 * このデバイス (0x10) がスレーブとして動作し、
 * 共有レジスタ[0] に ADC 値を書き続ける。
 * マスター側は Wiremin_read_reg(0x10, 0, &buf, 1) で読み出す。
 *
 * 共有レジスタのトランザクション形式:
 *   書き込み: START → 0x10+W → reg_idx → value → STOP
 *   読み出し: START → 0x10+W → reg_idx → rSTART → 0x10+R → data → NACK+STOP
 */
#include <Wiremin.h>

#define MY_ADDR  0x10
#define REG_ADC  0       // shared register 0: ADC value (0–255)
#define REG_LED  1       // shared register 1: LED control (0=off, 1=on)
#define LED_PIN  2       // PC0

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Wiremin_slave_begin(MY_ADDR);
}

void loop() {
  // Write ADC reading to shared register (master can read it)
  uint16_t raw = analogRead(A0);
  Wiremin_slave_set(REG_ADC, (uint8_t)(raw >> 2)); // 10-bit → 8-bit

  // Read LED control register written by master
  digitalWrite(LED_PIN, Wiremin_slave_get(REG_LED) ? HIGH : LOW);

  delay(10);
}
