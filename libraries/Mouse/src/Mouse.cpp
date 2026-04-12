#include "Mouse.h"
#include "uiapusb.h"
#include "Arduino.h"

static inline int8_t clamp8(int v)
{
    if (v >  127) return  127;
    if (v < -127) return -127;
    return (int8_t)v;
}

void MouseClass::begin()
{
    _buttons = 0;
    uiapusb_begin();
}

void MouseClass::end()
{
    _buttons = 0;
    uiapkbd_mouse_set(0, 0, 0, 0);
}

void MouseClass::move(int x, int y, int wheel)
{
    uiapkbd_mouse_set(_buttons, clamp8(x), clamp8(y), clamp8(wheel));
}

void MouseClass::press(uint8_t button)
{
    _buttons |= button;
    uiapkbd_mouse_set(_buttons, 0, 0, 0);
}

void MouseClass::release(uint8_t button)
{
    _buttons &= (uint8_t)~button;
    uiapkbd_mouse_set(_buttons, 0, 0, 0);
}

void MouseClass::click(uint8_t button)
{
    press(button);
    delay(10);
    release(button);
    delay(5);
}

bool MouseClass::isPressed(uint8_t button)
{
    return (_buttons & button) != 0;
}

MouseClass Mouse;
