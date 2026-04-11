#ifndef _HIDUIAP_CLASS_H
#define _HIDUIAP_CLASS_H

#include "uiapusb.h"

class HIDuiapClass {
public:
    void begin(void)                          { uiapusb_begin(); }
    int  available(void)                      { return uiapusb_available(); }
    int  read(uint8_t *buf, int maxlen)       { return uiapusb_read(buf, maxlen); }
    int  write(const uint8_t *buf, int len)   { return uiapusb_write(buf, len); }
};

extern HIDuiapClass HIDuiap;

#endif
