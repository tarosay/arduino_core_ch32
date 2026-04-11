/*
  Copyright (c) 2011 Arduino.  All right reserved.

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

/*
  ch32fun.h は直接インクルードしない。
  いまの uiapusb 環境では:
    - FUNCONF_SYSTEM_CORE_CLOCK = 48000000
    - FUNCONF_SYSTICK_USE_HCLK = 1
  なので SysTick->CNT は 48MHz で進む。
*/
extern void DelaySysTick(uint32_t n);

#define DELAY_MS_TICKS 48000u
#define DELAY_US_TICKS 48u

#ifdef __cplusplus
extern "C" {
#endif

static uint8_t  g_timebase_init = 0;
static uint32_t g_timebase_last_cnt = 0;
static uint64_t g_timebase_ticks64 = 0;

/*
  SystemInit() 済みの free-running SysTick->CNT を読む。
  割り込みは使わず、呼ばれるたびに 32bit カウンタ差分を 64bit に積み上げる。
*/
static uint64_t uiapusb_timebase_ticks(void)
{
  uint32_t now = SysTick->CNT;

  if (!g_timebase_init) {
    g_timebase_init = 1;
    g_timebase_last_cnt = now;
    g_timebase_ticks64 = (uint64_t)now;
    return g_timebase_ticks64;
  }

  g_timebase_ticks64 += (uint32_t)(now - g_timebase_last_cnt);
  g_timebase_last_cnt = now;
  return g_timebase_ticks64;
}

uint32_t millis(void)
{
  return (uint32_t)(uiapusb_timebase_ticks() / DELAY_MS_TICKS);
}

uint32_t micros(void)
{
  return (uint32_t)(uiapusb_timebase_ticks() / DELAY_US_TICKS);
}

void delay(uint32_t ms)
{
  if (ms != 0) {
    DelaySysTick(ms * DELAY_MS_TICKS);
  }
}

#ifdef __cplusplus
}
#endif