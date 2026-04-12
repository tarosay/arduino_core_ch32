#pragma once
#include <stdint.h>
#include <stddef.h>

// -------------------------------------------------------
// USB HID modifier key bits (for pressRaw / releaseRaw)
// -------------------------------------------------------
#define KEY_MOD_LCTRL   0x01
#define KEY_MOD_LSHIFT  0x02
#define KEY_MOD_LALT    0x04
#define KEY_MOD_LGUI    0x08
#define KEY_MOD_RCTRL   0x10
#define KEY_MOD_RSHIFT  0x20
#define KEY_MOD_RALT    0x40
#define KEY_MOD_RGUI    0x80

// -------------------------------------------------------
// Special key constants (compatible with Arduino Keyboard)
// -------------------------------------------------------
#define KEY_LEFT_CTRL   0x80
#define KEY_LEFT_SHIFT  0x81
#define KEY_LEFT_ALT    0x82
#define KEY_LEFT_GUI    0x83
#define KEY_RIGHT_CTRL  0x84
#define KEY_RIGHT_SHIFT 0x85
#define KEY_RIGHT_ALT   0x86
#define KEY_RIGHT_GUI   0x87

#define KEY_UP_ARROW    0xDA
#define KEY_DOWN_ARROW  0xD9
#define KEY_LEFT_ARROW  0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_BACKSPACE   0xB2
#define KEY_TAB         0xB3
#define KEY_RETURN      0xB0
#define KEY_ESC         0xB1
#define KEY_INSERT      0xD1
#define KEY_DELETE      0xD4
#define KEY_PAGE_UP     0xD3
#define KEY_PAGE_DOWN   0xD6
#define KEY_HOME        0xD2
#define KEY_END         0xD5
#define KEY_CAPS_LOCK   0xC1
#define KEY_F1          0xC2
#define KEY_F2          0xC3
#define KEY_F3          0xC4
#define KEY_F4          0xC5
#define KEY_F5          0xC6
#define KEY_F6          0xC7
#define KEY_F7          0xC8
#define KEY_F8          0xC9
#define KEY_F9          0xCA
#define KEY_F10         0xCB
#define KEY_F11         0xCC
#define KEY_F12         0xCD
#define KEY_PRINT_SCREEN 0xCE
#define KEY_NUM_LOCK    0xDB

// -------------------------------------------------------

class KeyboardClass {
public:
    void   begin();
    void   end();

    // ASCII / special key (0x80+)
    size_t write(uint8_t key);
    size_t write(const uint8_t *buf, size_t len);

    // press() / release() take same key codes as write()
    size_t press(uint8_t key);
    size_t release(uint8_t key);
    void   releaseAll();

    // Convenience
    void   print(const char *s);
    void   println(const char *s);

private:
    uint8_t _modifier;
    uint8_t _keys[6];

    void    _sendReport();
    bool    _toHID(uint8_t key, uint8_t *hid, uint8_t *modBit);
};

extern KeyboardClass Keyboard;
