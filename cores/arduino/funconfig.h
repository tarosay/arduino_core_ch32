#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

#define FUNCONF_USE_DEBUGPRINTF 1
#if defined(CH32VM00X)
#define CH32V00x                1   // CH32V002/4/5/6/7 (ch32x00xhw.h)
#else
#define CH32V003                1
#endif
#define FUNCONF_SYSTICK_USE_HCLK 1

#endif
