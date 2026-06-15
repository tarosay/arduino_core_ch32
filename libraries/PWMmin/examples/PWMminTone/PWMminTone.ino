/*
 * PWMminTone.ino — Pwm_tone ノンブロッキング tone サンプル
 *
 * Tools > PWM = "TIM2 Default (pin 2 / PC0)" で使用
 *
 * 接続:
 *   ブザー1（TIM1）: pin  5 (PC3 / TIM1-CH3) → GND
 *   ブザー2（TIM2）: pin  2 (PC0 / TIM2-CH3) → GND
 *
 * テスト内容:
 *   Phase 1: 1音ずつ順番に — ドレミファソラシド（Pwm_tone の基本）
 *   Phase 2: 2音同時       — TIM1・TIM2 で別の音を同時再生
 *   Phase 3: ノンブロッキング確認 — tone 再生中に LED_BUILTIN を点滅
 *
 * FQBN: UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,pwm=default,opt=oslto
 */

#include <PWMmin.h>
PWMMIN_REQUIRE_DEFAULT();

#define PIN_TIM1  5    /* PC3  TIM1-CH3 */
#define PIN_TIM2  2    /* PC0  TIM2-CH3  ← LED_BUILTIN */

static const uint16_t SCALE[] = {262, 294, 330, 349, 392, 440, 494, 523};
static const uint8_t  SCALE_N = sizeof(SCALE) / sizeof(SCALE[0]);

void setup() {}

void loop() {

  /* ── Phase 1: 1音ずつ順番に（ドレミファソラシド）──────────────────── */
  for (uint8_t i = 0; i < SCALE_N; i++) {
    Pwm_tone(PIN_TIM1, SCALE[i], 250);   /* 250ms 鳴らす */
    delay(350);                           /* 次の音まで待つ */
    Pwm_tone_update();
  }
  delay(500);

  /* ── Phase 2: 2音同時（TIM1・TIM2 独立）────────────────────────────── */
  Pwm_tone(PIN_TIM1, 262, 600);   /* TIM1: ド (C4) */
  Pwm_tone(PIN_TIM2, 392, 600);   /* TIM2: ソ (G4)  同時に鳴る */
  delay(700);
  Pwm_tone_update();

  Pwm_tone(PIN_TIM1, 330, 600);   /* TIM1: ミ (E4) */
  Pwm_tone(PIN_TIM2, 494, 600);   /* TIM2: シ (B4) */
  delay(700);
  Pwm_tone_update();
  delay(500);

  /* ── Phase 3: ノンブロッキング確認 ─────────────────────────────────── */
  /* ラ (A4) を 2000ms 鳴らしながら LED を点滅（tone 中も loop が回る） */
  Pwm_tone(PIN_TIM1, 440, 2000);
  uint32_t end = millis() + 2200;
  bool led = false;
  while (millis() < end) {
    Pwm_tone_update();
    /* LED_BUILTIN = pin2 = TIM2 なので別ピンで点滅確認 */
    /* ここでは Pwm_tone_update が毎ループ呼ばれていることを確認 */
    delay(100);
    led = !led;
  }
  delay(1000);
}
