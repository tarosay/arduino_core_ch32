/*
 * PWMmin.h — Lightweight PWM for CH32V003
 *
 * Header-only. No dynamic allocation. No HardwareTimer. Direct register access.
 * ATRLR=255 fixed (8-bit duty 0–255). Default ≈1 kHz.
 *
 * ── TIM1 Default (always available) ────────────────────────────────────────
 *   pin  0  PA1  TIM1-CH2  (D0/A1)
 *   pin  5  PC3  TIM1-CH3  (D5)
 *   pin  6  PC4  TIM1-CH4  (D6/A2)
 *   pin 12  PD2  TIM1-CH1  (D12/A3)
 *
 * ── TIM2 Default  (Tools > PWM = "TIM2 Default") ───────────────────────────
 *   pin  2  PC0  TIM2-CH3  (D2, LED_BUILTIN)
 *
 * ── TIM2 Remap3  (Tools > PWM = "TIM2 Remap3", -DPWMMIN_TIM2_REMAP3) ──────
 *   pin  3  PC1  TIM2-CH1  (D3, 2.2 kΩ pull-up on board)
 *   pin  9  PC7  TIM2-CH2  (D9 / SPI MISO)
 *   pin 15  PD5  TIM2-CH4  (D15 / UART TX)
 *   pin 16  PD6  TIM2-CH3  (D16 / UART RX)
 *
 * ── API ─────────────────────────────────────────────────────────────────────
 *   Pwm_write(pin, duty)      — duty 0–255, PWM 開始（初回で自動初期化）
 *   Pwm_freq(hz)              — TIM1/TIM2 両方の周波数変更（デフォルト 1000 Hz）
 *   Pwm_freq_TIM1(hz)         — TIM1 のみ周波数変更
 *   Pwm_freq_TIM2(hz)         — TIM2 のみ周波数変更
 *   Pwm_stop(pin)             — PWM 停止、GPIO を入力フロートに復元
 *
 * Note: Do NOT use together with analogWrite() on the same timer/pin.
 *       TIM2 Remap3 は PWMmin_write() 初回呼び出し時に AFIO を設定する。
 */
#pragma once
#include <Arduino.h>

#ifndef PWMMIN_FCPU_HZ
  #ifdef F_CPU
    #define PWMMIN_FCPU_HZ ((uint32_t)(F_CPU))
  #else
    #define PWMMIN_FCPU_HZ 48000000UL
  #endif
#endif

/* PSC = F_CPU/(256*freq) - 1  →  ≈1 kHz at 48 MHz → PSC=186, at 24 MHz → PSC=92 */
#ifndef PWMMIN_PSC_DEFAULT
  #define PWMMIN_PSC_DEFAULT ((uint16_t)(PWMMIN_FCPU_HZ / 256000UL - 1UL))
#endif

/* ── 設定チェックマクロ（スケッチの先頭で呼ぶ）────────────────────────── */
#ifdef PWMMIN_TIM2_REMAP3
  #define PWMMIN_REQUIRE_DEFAULT() \
    static_assert(0, "Tools > PWM を 'TIM2 Default (pin 2 / PC0)' に設定してください")
  #define PWMMIN_REQUIRE_REMAP3()  /* ok */
#else
  #define PWMMIN_REQUIRE_DEFAULT() /* ok */
  #define PWMMIN_REQUIRE_REMAP3()  \
    static_assert(0, "Tools > PWM を 'TIM2 Remap3 (pins 9/15/16)' に設定してください")
#endif

static uint16_t _pm_psc1 = PWMMIN_PSC_DEFAULT;
static uint16_t _pm_psc2 = PWMMIN_PSC_DEFAULT;

/* ── Pwm_write ──────────────────────────────────────────────────────────── */

static void Pwm_write(uint8_t pin, uint8_t duty)
{
  switch (pin) {

  /* ── TIM1 Default ─────────────────────────────────────────────────────── */
  case 0:  /* PA1  TIM1-CH2 */
    RCC->APB2PCENR |= RCC_TIM1EN | RCC_IOPAEN;
    GPIOA->CFGLR = (GPIOA->CFGLR & ~0x000000F0U) | 0x000000B0U;
    TIM1->PSC    = _pm_psc1;  TIM1->ATRLR = 255;
    TIM1->CH2CVR = duty;
    TIM1->CHCTLR1 = (TIM1->CHCTLR1 & ~0x7000U) | 0x6000U;
    TIM1->CCER  |= TIM_CC2E;
    TIM1->BDTR  |= TIM_MOE;  TIM1->CTLR1 |= TIM_CEN;
    break;

  case 5:  /* PC3  TIM1-CH3 */
    RCC->APB2PCENR |= RCC_TIM1EN | RCC_IOPCEN;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x0000F000U) | 0x0000B000U;
    TIM1->PSC    = _pm_psc1;  TIM1->ATRLR = 255;
    TIM1->CH3CVR = duty;
    TIM1->CHCTLR2 = (TIM1->CHCTLR2 & ~0x0070U) | 0x0060U;
    TIM1->CCER  |= TIM_CC3E;
    TIM1->BDTR  |= TIM_MOE;  TIM1->CTLR1 |= TIM_CEN;
    break;

  case 6:  /* PC4  TIM1-CH4 */
    RCC->APB2PCENR |= RCC_TIM1EN | RCC_IOPCEN;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x000F0000U) | 0x000B0000U;
    TIM1->PSC    = _pm_psc1;  TIM1->ATRLR = 255;
    TIM1->CH4CVR = duty;
    TIM1->CHCTLR2 = (TIM1->CHCTLR2 & ~0x7000U) | 0x6000U;
    TIM1->CCER  |= TIM_CC4E;
    TIM1->BDTR  |= TIM_MOE;  TIM1->CTLR1 |= TIM_CEN;
    break;

  case 12: /* PD2  TIM1-CH1 */
    RCC->APB2PCENR |= RCC_TIM1EN | RCC_IOPDEN;
    GPIOD->CFGLR = (GPIOD->CFGLR & ~0x00000F00U) | 0x00000B00U;
    TIM1->PSC    = _pm_psc1;  TIM1->ATRLR = 255;
    TIM1->CH1CVR = duty;
    TIM1->CHCTLR1 = (TIM1->CHCTLR1 & ~0x0070U) | 0x0060U;
    TIM1->CCER  |= TIM_CC1E;
    TIM1->BDTR  |= TIM_MOE;  TIM1->CTLR1 |= TIM_CEN;
    break;

  /* ── TIM2 ─────────────────────────────────────────────────────────────── */
#ifdef PWMMIN_TIM2_REMAP3
  case 3:  /* PC1  TIM2-CH1  Remap3 */
    RCC->APB2PCENR |= RCC_AFIOEN;
    AFIO->PCFR1 = (AFIO->PCFR1 & ~0x300U) | 0x300U;
    RCC->APB1PCENR |= RCC_TIM2EN;  RCC->APB2PCENR |= RCC_IOPCEN;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x000000F0U) | 0x000000B0U;
    TIM2->PSC    = _pm_psc2;  TIM2->ATRLR = 255;
    TIM2->CH1CVR = duty;
    TIM2->CHCTLR1 = (TIM2->CHCTLR1 & ~0x0070U) | 0x0060U;
    TIM2->CCER  |= TIM_CC1E;  TIM2->CTLR1 |= TIM_CEN;
    break;

  case 9:  /* PC7  TIM2-CH2  Remap3 */
    RCC->APB2PCENR |= RCC_AFIOEN;
    AFIO->PCFR1 = (AFIO->PCFR1 & ~0x300U) | 0x300U;
    RCC->APB1PCENR |= RCC_TIM2EN;  RCC->APB2PCENR |= RCC_IOPCEN;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0xF0000000U) | 0xB0000000U;
    TIM2->PSC    = _pm_psc2;  TIM2->ATRLR = 255;
    TIM2->CH2CVR = duty;
    TIM2->CHCTLR1 = (TIM2->CHCTLR1 & ~0x7000U) | 0x6000U;
    TIM2->CCER  |= TIM_CC2E;  TIM2->CTLR1 |= TIM_CEN;
    break;

  case 16: /* PD6  TIM2-CH3  Remap3 */
    RCC->APB2PCENR |= RCC_AFIOEN;
    AFIO->PCFR1 = (AFIO->PCFR1 & ~0x300U) | 0x300U;
    RCC->APB1PCENR |= RCC_TIM2EN;  RCC->APB2PCENR |= RCC_IOPDEN;
    GPIOD->CFGLR = (GPIOD->CFGLR & ~0x0F000000U) | 0x0B000000U;
    TIM2->PSC    = _pm_psc2;  TIM2->ATRLR = 255;
    TIM2->CH3CVR = duty;
    TIM2->CHCTLR2 = (TIM2->CHCTLR2 & ~0x0070U) | 0x0060U;
    TIM2->CCER  |= TIM_CC3E;  TIM2->CTLR1 |= TIM_CEN;
    break;

  case 15: /* PD5  TIM2-CH4  Remap3 */
    RCC->APB2PCENR |= RCC_AFIOEN;
    AFIO->PCFR1 = (AFIO->PCFR1 & ~0x300U) | 0x300U;
    RCC->APB1PCENR |= RCC_TIM2EN;  RCC->APB2PCENR |= RCC_IOPDEN;
    GPIOD->CFGLR = (GPIOD->CFGLR & ~0x00F00000U) | 0x00B00000U;
    TIM2->PSC    = _pm_psc2;  TIM2->ATRLR = 255;
    TIM2->CH4CVR = duty;
    TIM2->CHCTLR2 = (TIM2->CHCTLR2 & ~0x7000U) | 0x6000U;
    TIM2->CCER  |= TIM_CC4E;  TIM2->CTLR1 |= TIM_CEN;
    break;

#else  /* TIM2 Default */
  case 2:  /* PC0  TIM2-CH3  Default */
    RCC->APB1PCENR |= RCC_TIM2EN;  RCC->APB2PCENR |= RCC_IOPCEN;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x0000000FU) | 0x0000000BU;
    TIM2->PSC    = _pm_psc2;  TIM2->ATRLR = 255;
    TIM2->CH3CVR = duty;
    TIM2->CHCTLR2 = (TIM2->CHCTLR2 & ~0x0070U) | 0x0060U;
    TIM2->CCER  |= TIM_CC3E;  TIM2->CTLR1 |= TIM_CEN;
    break;
#endif

  default:
    break;
  }
}

/* ── Pwm_freq ───────────────────────────────────────────────────────────── */

static void Pwm_freq_TIM1(uint32_t hz)
{
  if (hz == 0) return;
  uint32_t psc = PWMMIN_FCPU_HZ / (256UL * hz);
  if (psc > 0) psc--;
  if (psc > 65535UL) psc = 65535U;
  _pm_psc1 = (uint16_t)psc;
  if (TIM1->CTLR1 & TIM_CEN) TIM1->PSC = _pm_psc1;
}

static void Pwm_freq_TIM2(uint32_t hz)
{
  if (hz == 0) return;
  uint32_t psc = PWMMIN_FCPU_HZ / (256UL * hz);
  if (psc > 0) psc--;
  if (psc > 65535UL) psc = 65535U;
  _pm_psc2 = (uint16_t)psc;
  if (TIM2->CTLR1 & TIM_CEN) TIM2->PSC = _pm_psc2;
}

static inline void Pwm_freq(uint32_t hz)
{
  Pwm_freq_TIM1(hz);
  Pwm_freq_TIM2(hz);
}

/* ── Pwm_stop ───────────────────────────────────────────────────────────── */

static void Pwm_stop(uint8_t pin)
{
  switch (pin) {
  case 0:  /* PA1  TIM1-CH2 */
    TIM1->CCER &= ~TIM_CC2E;
    GPIOA->CFGLR = (GPIOA->CFGLR & ~0x000000F0U) | 0x00000040U;
    break;
  case 5:  /* PC3  TIM1-CH3 */
    TIM1->CCER &= ~TIM_CC3E;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x0000F000U) | 0x00004000U;
    break;
  case 6:  /* PC4  TIM1-CH4 */
    TIM1->CCER &= ~TIM_CC4E;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x000F0000U) | 0x00040000U;
    break;
  case 12: /* PD2  TIM1-CH1 */
    TIM1->CCER &= ~TIM_CC1E;
    GPIOD->CFGLR = (GPIOD->CFGLR & ~0x00000F00U) | 0x00000400U;
    break;
#ifdef PWMMIN_TIM2_REMAP3
  case 3:  /* PC1  TIM2-CH1 */
    TIM2->CCER &= ~TIM_CC1E;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x000000F0U) | 0x00000040U;
    break;
  case 9:  /* PC7  TIM2-CH2 */
    TIM2->CCER &= ~TIM_CC2E;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0xF0000000U) | 0x40000000U;
    break;
  case 16: /* PD6  TIM2-CH3 */
    TIM2->CCER &= ~TIM_CC3E;
    GPIOD->CFGLR = (GPIOD->CFGLR & ~0x0F000000U) | 0x04000000U;
    break;
  case 15: /* PD5  TIM2-CH4 */
    TIM2->CCER &= ~TIM_CC4E;
    GPIOD->CFGLR = (GPIOD->CFGLR & ~0x00F00000U) | 0x00400000U;
    break;
#else
  case 2:  /* PC0  TIM2-CH3 */
    TIM2->CCER &= ~TIM_CC3E;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~0x0000000FU) | 0x00000004U;
    break;
#endif
  default:
    break;
  }
}
