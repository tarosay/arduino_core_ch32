#pragma once
#include <stdint.h>

#define MOUSE_LEFT    0x01
#define MOUSE_RIGHT   0x02
#define MOUSE_MIDDLE  0x04

class MouseClass {
public:
    void begin();
    void end();

    // 相対移動 (-127 ～ 127)
    void move(int x, int y, int wheel = 0);

    // 大きな相対移動（-127～127 の範囲を超える移動を steps 回に分割して送信）
    // 例: Mouse.moveLarge(500, -300, 20); → 20 ステップに分割して移動
    void moveLarge(int dx, int dy, int steps = 10);

    // ボタン操作
    void press(uint8_t button = MOUSE_LEFT);
    void release(uint8_t button = MOUSE_LEFT);
    void click(uint8_t button = MOUSE_LEFT);
    bool isPressed(uint8_t button = MOUSE_LEFT);

private:
    uint8_t _buttons;
};

extern MouseClass Mouse;
