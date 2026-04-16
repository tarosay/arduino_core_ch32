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

void MouseClass::moveLarge(int x, int y, int wheel, int steps)
{
    if (steps < 1) steps = 1;

    /* 1ステップあたりの移動量。0 になる場合は最小 1px（移動方向に合わせた符号）*/
    int bx = x / steps;
    int by = y / steps;
    if (bx == 0 && x != 0) bx = (x > 0) ? 1 : -1;
    if (by == 0 && y != 0) by = (y > 0) ? 1 : -1;

    while (x != 0 || y != 0) {
        /* 残量が 1ステップ分より少なければ残量をそのまま送る */
        int mx = (x == 0) ? 0 : ((abs(x) <= abs(bx)) ? x : bx);
        int my = (y == 0) ? 0 : ((abs(y) <= abs(by)) ? y : by);
        move(mx, my, wheel);
        x -= mx;
        y -= my;
        delay(10);  /* EP1 ポーリング間隔（10ms）に合わせて確実に受け取らせる */
    }
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
