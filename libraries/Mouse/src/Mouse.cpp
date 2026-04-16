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

void MouseClass::moveLarge(int dx, int dy, int steps)
{
    if (steps < 1) steps = 1;

    /* 1ステップあたりの移動量。0 になる場合は最小 1px（移動方向に合わせた符号）*/
    int bx = dx / steps;
    int by = dy / steps;
    if (bx == 0 && dx != 0) bx = (dx > 0) ? 1 : -1;
    if (by == 0 && dy != 0) by = (dy > 0) ? 1 : -1;

    while (dx != 0 || dy != 0) {
        /* 残量が 1ステップ分より少なければ残量をそのまま送る */
        int mx = (dx == 0) ? 0 : ((abs(dx) <= abs(bx)) ? dx : bx);
        int my = (dy == 0) ? 0 : ((abs(dy) <= abs(by)) ? dy : by);
        move(mx, my);
        dx -= mx;
        dy -= my;
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
