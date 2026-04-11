#define ARDUINO_MAIN

#include "Arduino.h"

extern "C" int main(void)
{
    uiap_core_begin();
    setup();
    while (1) {
        loop();
    }
}
