/*
  Copyright (c) 2011 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

extern "C" {
#include "stdlib.h"
#include "stdint.h"
}
#include "WMath.h"

// Own generator instead of newlib's rand()/srand(): this core does not link
// newlib (-nostdlib), so rand()/srand() are undefined, and linking newlib only
// for them costs flash. xorshift32 needs no multiply (V003 has no M extension).
static uint32_t _rand_next = 1;

static long _rand(void)
{
  _rand_next ^= _rand_next << 13;
  _rand_next ^= _rand_next >> 17;
  _rand_next ^= _rand_next << 5;
  return (long)(_rand_next & RAND_MAX);
}

extern void randomSeed(uint32_t dwSeed)
{
  if (dwSeed != 0) {
    _rand_next = dwSeed;
  }
}

extern long random(long howbig)
{
  if (howbig == 0) {
    return 0 ;
  }

  return _rand() % howbig;
}

extern long random(long howsmall, long howbig)
{
  if (howsmall >= howbig) {
    return howsmall;
  }

  long diff = howbig - howsmall;

  return random(diff) + howsmall;
}

extern long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

extern uint16_t makeWord(uint16_t w)
{
  return w ;
}

extern uint16_t makeWord(uint8_t h, uint8_t l)
{
  return (h << 8) | l ;
}
