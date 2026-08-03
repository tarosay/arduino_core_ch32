/*
  dtostrf - Emulation for dtostrf function from avr-libc
  Copyright (c) 2013 Arduino.  All rights reserved.
  Written by Cristian Maglie <c.maglie@arduino.cc>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Largest fraction that still fits an unsigned long (10^9 < 2^32) */
#define DTOSTRF_MAX_PREC  9

/* sout must be large enough for the sign, the digits, the '.', `prec`
   decimals, the padding up to `width`, and the terminator. */
char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  unsigned char negative = 0;
  double rounding = 0.5;
  double remainder;
  unsigned long int_part;
  unsigned long dec_part;
  double decade = 1.0;
  char *p = sout;
  size_t len;
  unsigned int w;
  int i;

  // Handle negative numbers
  if (val < 0.0) {
    negative = 1;
    val = -val;
  }

  if (prec > DTOSTRF_MAX_PREC) {
    prec = DTOSTRF_MAX_PREC;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  for (i = 0; i < prec; ++i) {
    rounding /= 10.0;
  }
  val += rounding;

  // Split into integer and fractional digits
  int_part = (unsigned long)val;
  remainder = val - (double)int_part;

  for (i = 0; i < prec; i++) {
    decade *= 10.0;
  }
  dec_part = (unsigned long)(remainder * decade);

  // Sign first, so that values rounding to "0.xx" keep their minus sign
  if (negative) {
    *p++ = '-';
  }

  // Integer part, most significant digit first
  {
    char digits[11];  // an unsigned long is at most 10 digits
    int n = 0;
    do {
      digits[n++] = (char)('0' + (int)(int_part % 10));
      int_part /= 10;
    } while (int_part);
    while (n) {
      *p++ = digits[--n];
    }
  }

  // Fractional part, zero padded so that 1.05 does not come out as "1.5"
  if (prec > 0) {
    *p++ = '.';
    for (i = prec - 1; i >= 0; i--) {
      p[i] = (char)('0' + (int)(dec_part % 10));
      dec_part /= 10;
    }
    p += prec;
  }
  *p = '\0';

  // Handle minimum field width of the output string
  // width is signed value, negative for left adjustment.
  // Range -128,127
  len = (size_t)(p - sout);
  if (width < 0) {
    negative = 1;
    w = (unsigned int)(-(int)width);
  } else {
    negative = 0;
    w = (unsigned int)width;
  }

  if (len < w) {
    size_t pad = (size_t)w - len;
    if (negative == 0) {
      // right adjustment: shift the text up and blank-fill the front
      memmove(sout + pad, sout, len + 1);
      memset(sout, ' ', pad);
    } else {
      // left adjustment: blank-fill the tail
      memset(sout + len, ' ', pad);
      sout[len + pad] = '\0';
    }
  }

  return sout;
}
