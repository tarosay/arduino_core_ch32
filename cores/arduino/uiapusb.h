#ifndef _UIAPUSB_H
#define _UIAPUSB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void uiap_core_begin(void);
void uiapusb_begin(void);
int  uiapusb_available(void);
int  uiapusb_read(uint8_t *buf, int maxlen);
int  uiapusb_write(const uint8_t *buf, int len);

#ifdef __cplusplus
}
#endif

#endif