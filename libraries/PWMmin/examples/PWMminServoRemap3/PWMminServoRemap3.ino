/*
 * PWMminServoRemap3.ino — PWMmin サーボモーター制御サンプル（TIM2 Remap3）
 *
 * Tools > PWM    = "TIM2 Remap3 (pins 9/15/16)" で使用
 * Tools > U(S)ART = "None" にすること（pin 15/16 が UART TX/RX と競合）
 *
 * 接続:
 *   サーボ1（TIM2）: pin  9 (PC7 / TIM2-CH2) → サーボ信号線
 *   サーボ2（TIM1）: pin  5 (PC3 / TIM1-CH3) → サーボ信号線
 *   サーボ電源: VCC(5V) / GND
 *
 * テスト内容:
 *   Phase 1: 1軸制御 — サーボ1を 0°→90°→180°→90°→0° と動かす
 *   Phase 2: 2軸独立制御 — サーボ1・2を逆方向に同時に動かす
 *
 * FQBN: UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,pwm=remap3,xserial=none,opt=oslto
 */

#include <PWMmin.h>
PWMMIN_REQUIRE_REMAP3();

#define PIN_SERVO1  9   /* PC7  TIM2-CH2 */
#define PIN_SERVO2  5   /* PC3  TIM1-CH3 */

void setup() {
  Pwm_servo_begin();    /* TIM1/TIM2 両方を 500Hz（サーボ用）に設定 */
}

void loop() {

  /* ── Phase 1: 1軸制御（サーボ1のみ）──────────────────────────────── */
  Pwm_servo(PIN_SERVO1,   0);  delay(1000);
  Pwm_servo(PIN_SERVO1,  90);  delay(1000);
  Pwm_servo(PIN_SERVO1, 180);  delay(1000);
  Pwm_servo(PIN_SERVO1,  90);  delay(1000);
  Pwm_servo(PIN_SERVO1,   0);  delay(1000);

  /* ── Phase 2: 2軸独立制御（サーボ1・2を逆方向に）─────────────────── */
  for (uint8_t angle = 0; angle <= 180; angle += 10) {
    Pwm_servo(PIN_SERVO1, angle);
    Pwm_servo(PIN_SERVO2, 180 - angle);
    delay(100);
  }
  delay(500);
  for (uint8_t angle = 180; angle > 0; angle -= 10) {
    Pwm_servo(PIN_SERVO1, angle);
    Pwm_servo(PIN_SERVO2, 180 - angle);
    delay(100);
  }
  delay(500);
}
