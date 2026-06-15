/*
 * PWMminTone.ino — PWMmin ブザー2個テスト（全 API 確認）
 *
 * Tools > PWM = "TIM2 Default (pin 2 / PC0)" で使用
 *
 * 接続:
 *   ブザー1（TIM1）: pin  5 (PC3 / TIM1-CH3) → GND
 *   ブザー2（TIM2）: pin  2 (PC0 / TIM2-CH3) → GND
 *
 * テスト内容:
 *   Phase 1: ドレミ        — Pwm_tone でノンブロッキング順次再生
 *   Phase 2: 2音同時       — Pwm_tone で TIM1・TIM2 同時再生
 *   Phase 3: 和音          — Pwm_freq_TIM1 / Pwm_freq_TIM2 で別音程を同時に鳴らす
 *   Phase 4: 交互          — Pwm_stop で片方ずつ停止
 *   Phase 5: 音量スイープ  — duty 1→128→1 で音量変化を確認
 *
 * FQBN: UIAP_HID:ch32v:CH32V003:pnum=V14,usb=webhid,pwm=default,opt=oslto
 */

#include <PWMmin.h>
PWMMIN_REQUIRE_DEFAULT();

#define PIN_TIM1  5    /* PC3  TIM1-CH3 */
#define PIN_TIM2  2    /* PC0  TIM2-CH3 */

static const uint16_t SCALE[] = {262, 294, 330, 349, 392, 440, 494, 523};
static const uint8_t  SCALE_N = sizeof(SCALE) / sizeof(SCALE[0]);

static const uint16_t CHORDS[][2] = {
  {262, 330},  /* ド＋ミ（長3度） */
  {262, 392},  /* ド＋ソ（完全5度） */
  {330, 392},  /* ミ＋ソ（短3度） */
  {349, 440},  /* ファ＋ラ（長3度） */
};
static const uint8_t CHORDS_N = sizeof(CHORDS) / sizeof(CHORDS[0]);

static void silence(uint16_t ms) {
  Pwm_stop(PIN_TIM1);
  Pwm_stop(PIN_TIM2);
  delay(ms);
}

void setup() {}

void loop() {

  /* ── Phase 1: ドレミ（Pwm_tone ノンブロッキング）───────────────────── */
  for (uint8_t i = 0; i < SCALE_N; i++) {
    Pwm_tone(PIN_TIM1, SCALE[i], 250);
    delay(350);
    Pwm_tone_update();
  }
  delay(500);

  /* ── Phase 2: 2音同時（Pwm_tone TIM1 + TIM2）──────────────────────── */
  Pwm_tone(PIN_TIM1, 262, 600);   /* ド (C4) */
  Pwm_tone(PIN_TIM2, 392, 600);   /* ソ (G4) */
  delay(700);  Pwm_tone_update();

  Pwm_tone(PIN_TIM1, 330, 600);   /* ミ (E4) */
  Pwm_tone(PIN_TIM2, 494, 600);   /* シ (B4) */
  delay(700);  Pwm_tone_update();
  delay(500);

  /* ── Phase 3: 和音（Pwm_freq_TIM1 / Pwm_freq_TIM2 独立制御）──────── */
  for (uint8_t i = 0; i < CHORDS_N; i++) {
    Pwm_freq_TIM1(CHORDS[i][0]);
    Pwm_freq_TIM2(CHORDS[i][1]);
    Pwm_write(PIN_TIM1, 128);
    Pwm_write(PIN_TIM2, 128);
    delay(600);
    silence(150);
  }
  delay(500);

  /* ── Phase 4: 交互（Pwm_stop で片方ずつ停止）──────────────────────── */
  Pwm_freq(440);
  for (uint8_t i = 0; i < 4; i++) {
    Pwm_write(PIN_TIM1, 128);  Pwm_stop(PIN_TIM2);
    delay(300);
    Pwm_stop(PIN_TIM1);        Pwm_write(PIN_TIM2, 128);
    delay(300);
  }
  silence(500);

  /* ── Phase 5: 音量スイープ（duty 1→128→1）─────────────────────────── */
  Pwm_freq(440);
  for (uint8_t d = 1; d <= 128; d++) {
    Pwm_write(PIN_TIM1, d);
    Pwm_write(PIN_TIM2, d);
    delay(15);
  }
  for (uint8_t d = 128; d >= 1; d--) {
    Pwm_write(PIN_TIM1, d);
    Pwm_write(PIN_TIM2, d);
    delay(15);
  }
  silence(300);

  delay(1000);
}
