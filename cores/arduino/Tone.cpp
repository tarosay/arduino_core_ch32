/* Tone.cpp

  A Tone Generator Library for CH32 Arduino core.

  Original software bit-bang by Brett Hagman.
  Hardware PWM path added to support true indefinite playback (duration=0)
  without blocking the CPU. When the target pin has a timer channel in
  PinMap_TIM the tone is generated via hardware PWM (50% duty cycle) so
  tone(pin, freq) returns immediately and plays forever until noTone().
  tone(pin, freq, duration) uses the same hardware path but blocks for
  duration ms before stopping, matching Arduino behaviour.

  Boards without TIM_MODULE_ENABLED, or pins that have no timer channel,
  fall back to the original software bit-bang loop.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#include "Arduino.h"
#include "PeripheralPins.h"

extern "C" void DelaySysTick(uint32_t n);

#ifndef DELAY_US_TICKS
#define DELAY_US_TICKS 48u
#endif

/* ------------------------------------------------------------------ */
/* Hardware PWM path (requires TIM_MODULE_ENABLED)                     */
/* ------------------------------------------------------------------ */
#if defined(TIM_MODULE_ENABLED) && !defined(TIM_MODULE_ONLY)

// Active tone state — 12 bytes of RAM total
static PinName      _tone_pin     = NC;
static TIM_TypeDef *_tone_tim     = NULL;
static uint32_t     _tone_channel = 0;

static uint32_t _tone_get_timer_clk(TIM_TypeDef *tim)
{
  RCC_ClocksTypeDef cs = {0};
  RCC_GetClocksFreq(&cs);

#if defined(CH32V00x) || defined(CH32X035) || defined(CH32VM00X)
  // These devices have a single bus; timer clock equals HCLK.
  (void)tim;
  return cs.HCLK_Frequency;
#else
  uint32_t pclk, ppre;
  if (getTimerClkSrc(tim) == 2) {
    pclk = cs.PCLK2_Frequency;
    ppre = (RCC->CFGR0 & RCC_PPRE2) >> 11;
  } else {
    pclk = cs.PCLK1_Frequency;
    ppre = (RCC->CFGR0 & RCC_PPRE1) >> 8;
  }
  // When APBx prescaler > 1 the timer clock is doubled.
  return ((ppre & 0x4u) != 0u) ? pclk * 2u : pclk;
#endif
}

static void _tone_hw_start(PinName p, TIM_TypeDef *tim, uint32_t channel,
                            unsigned int frequency)
{
  TIM_HandleTypeDef htim = {0};
  htim.Instance = tim;
  enableTimerClock(&htim);

  pinmap_pinout(p, PinMap_TIM);

  uint32_t clk        = _tone_get_timer_clk(tim);
  uint32_t period_cyc = clk / frequency;
  uint16_t psc        = (uint16_t)((period_cyc - 1u) >> 16);
  period_cyc          = period_cyc / (uint32_t)(psc + 1u);

  TIM_TimeBaseInitTypeDef tbase = {0};
  tbase.TIM_Prescaler     = psc;
  tbase.TIM_Period        = (uint16_t)(period_cyc - 1u);
  tbase.TIM_CounterMode   = TIM_CounterMode_Up;
  tbase.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInit(tim, &tbase);

  TIM_OCInitTypeDef oc = {0};
  oc.TIM_OCMode      = TIM_OCMode_PWM1;
  oc.TIM_OutputState = TIM_OutputState_Enable;
  oc.TIM_Pulse       = (uint16_t)(period_cyc / 2u);
  oc.TIM_OCPolarity  = TIM_OCPolarity_High;
#if defined(TIM_CC1NE)
  oc.TIM_OutputNState = TIM_OutputNState_Disable;
  oc.TIM_OCNPolarity  = TIM_OCNPolarity_High;
  oc.TIM_OCIdleState  = TIM_OCIdleState_Reset;
  oc.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
#endif

  switch (channel) {
    case TIM_Channel_1:
      TIM_OC1Init(tim, &oc);
      TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
      break;
    case TIM_Channel_2:
      TIM_OC2Init(tim, &oc);
      TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
      break;
    case TIM_Channel_3:
      TIM_OC3Init(tim, &oc);
      TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
      break;
    case TIM_Channel_4:
      TIM_OC4Init(tim, &oc);
      TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);
      break;
    default:
      return;
  }

  TIM_ARRPreloadConfig(tim, ENABLE);
  TIM_CCxCmd(tim, channel, TIM_CCx_Enable);
  tim->BDTR |= TIM_MOE;
  TIM_Cmd(tim, ENABLE);
}

static void _tone_hw_stop(void)
{
  if (_tone_tim == NULL) {
    return;
  }

  TIM_CCxCmd(_tone_tim, _tone_channel, TIM_CCx_Disable);
  TIM_Cmd(_tone_tim, DISABLE);

  if (_tone_pin != NC) {
    GPIO_TypeDef *port    = get_GPIO_Port(CH_PORT(_tone_pin));
    uint32_t      pin_mask = CH_MAP_GPIO_PIN(_tone_pin);
    if (port != NULL) {
      pin_function(_tone_pin,
                   CH_PIN_DATA(CH_MODE_OUTPUT_50MHz, CH_CNF_OUTPUT_PP,
                               NOPULL, AFIO_NONE));
      digital_io_write(port, pin_mask, 0);
    }
  }

  _tone_tim = NULL;
  _tone_pin = NC;
}

#endif /* TIM_MODULE_ENABLED && !TIM_MODULE_ONLY */

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration)
{
  if (frequency == 0) {
    return;
  }

  PinName p = digitalPinToPinName(_pin);
  if (p == NC) {
    return;
  }

#if defined(TIM_MODULE_ENABLED) && !defined(TIM_MODULE_ONLY)
  TIM_TypeDef *tim = (TIM_TypeDef *)pinmap_peripheral(p, PinMap_TIM);
  if (tim != (TIM_TypeDef *)NP) {
    uint32_t function = pinmap_function(p, PinMap_TIM);
    // Skip complementary (CHxN) channels — they need CCxNCmd, not CCxCmd.
    if (!CH_PIN_INVERTED(function)) {
      uint32_t channel = getTimerChannel(p);

      _tone_hw_stop();
      _tone_pin     = p;
      _tone_tim     = tim;
      _tone_channel = channel;
      _tone_hw_start(p, tim, channel, frequency);

      if (duration > 0) {
        delay(duration);
        _tone_hw_stop();
      }
      // duration == 0: return immediately; timer keeps running autonomously.
      return;
    }
  }
#endif

  /* Software bit-bang fallback for pins without a timer channel.        */
  /* For duration == 0 this plays for ~1 s (no hardware timer available). */
  uint32_t half_period_us = 500000UL / frequency;
  if (half_period_us == 0) {
    half_period_us = 1;
  }

  uint32_t toggles;
  if (duration > 0) {
    toggles = (2UL * frequency * duration) / 1000UL;
    if (toggles == 0) {
      toggles = 1;
    }
  } else {
    toggles = 2UL * frequency; // ~1 second — no timer available for this pin
  }

  GPIO_TypeDef *port    = get_GPIO_Port(CH_PORT(p));
  uint32_t      pin_mask = CH_MAP_GPIO_PIN(p);
  if (port == NULL) {
    return;
  }

  pin_function(p, CH_PIN_DATA(CH_MODE_OUTPUT_50MHz, CH_CNF_OUTPUT_PP,
                               NOPULL, AFIO_NONE));

  uint32_t delay_ticks = half_period_us * DELAY_US_TICKS;
  for (uint32_t i = 0; i < toggles; i++) {
    digital_io_toggle(port, pin_mask);
    DelaySysTick(delay_ticks);
  }
  digital_io_write(port, pin_mask, 0);
}

void noTone(uint8_t _pin, bool destruct)
{
  (void)(destruct);
  PinName p = digitalPinToPinName(_pin);
  if (p == NC) {
    return;
  }

#if defined(TIM_MODULE_ENABLED) && !defined(TIM_MODULE_ONLY)
  if (p == _tone_pin) {
    _tone_hw_stop();
    return;
  }
#endif

  GPIO_TypeDef *port    = get_GPIO_Port(CH_PORT(p));
  uint32_t      pin_mask = CH_MAP_GPIO_PIN(p);
  if (port != NULL) {
    digital_io_write(port, pin_mask, 0);
  }
}
