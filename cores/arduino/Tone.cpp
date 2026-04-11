/* Tone.cpp

  A Tone Generator Library - Software Implementation for ch32fun/rv003usb

  Original by Brett Hagman, modified for CH32V003 with ch32fun.
  This version uses software bit-bang (DelaySysTick) instead of
  HardwareTimer to avoid heap allocation and interrupt issues.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"

extern "C" void DelaySysTick(uint32_t n);

#ifndef DELAY_US_TICKS
#define DELAY_US_TICKS 48u
#endif

// frequency (in hertz) and duration (in milliseconds).
void tone(uint8_t _pin, unsigned int frequency, unsigned long duration)
{
  if (frequency == 0) {
    return;
  }

  // Half-period in microseconds
  uint32_t half_period_us = 500000UL / frequency;
  if (half_period_us == 0) {
    half_period_us = 1;
  }

  // Total number of toggles (2 toggles per cycle)
  uint32_t toggles;
  if (duration > 0) {
    toggles = (2UL * frequency * duration) / 1000UL;
    if (toggles == 0) {
      toggles = 1;
    }
  } else {
    // duration == 0 means play indefinitely; limit to ~1 second
    toggles = 2UL * frequency;
  }

  PinName p = digitalPinToPinName(_pin);
  if (p == NC) {
    return;
  }

  GPIO_TypeDef *port = get_GPIO_Port(CH_PORT(p));
  uint32_t pin_mask = CH_MAP_GPIO_PIN(p);

  if (port == NULL) {
    return;
  }

  // Set pin as output
  pin_function(p, CH_PIN_DATA(CH_MODE_OUTPUT_50MHz, CH_CNF_OUTPUT_PP, NOPULL, AFIO_NONE));

  uint32_t delay_ticks = half_period_us * DELAY_US_TICKS;

  for (uint32_t i = 0; i < toggles; i++) {
    digital_io_toggle(port, pin_mask);
    DelaySysTick(delay_ticks);
  }

  // Ensure pin is LOW after tone finishes
  digital_io_write(port, pin_mask, 0);
}

void noTone(uint8_t _pin, bool destruct)
{
  (void)(destruct);
  PinName p = digitalPinToPinName(_pin);
  if (p != NC) {
    GPIO_TypeDef *port = get_GPIO_Port(CH_PORT(p));
    uint32_t pin_mask = CH_MAP_GPIO_PIN(p);
    if (port != NULL) {
      digital_io_write(port, pin_mask, 0);
    }
  }
}
