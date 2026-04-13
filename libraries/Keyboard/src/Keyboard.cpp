#include "Keyboard.h"
#include "uiapusb.h"
#include "Arduino.h"
#include <string.h>

// -------------------------------------------------------
// ASCII / special-key → USB HID usage code + modifier bit
// Returns true if a valid mapping was found.
// -------------------------------------------------------
bool KeyboardClass::_toHID(uint8_t key, uint8_t *hid, uint8_t *modBit)
{
    *modBit = 0;

    /* --- modifier keys (0x80-0x87) --- */
    if (key >= KEY_LEFT_CTRL && key <= KEY_RIGHT_GUI) {
        *hid    = 0; /* goes into modifier byte, not key slot */
        *modBit = (uint8_t)(1 << (key - KEY_LEFT_CTRL));
        return true;
    }

    /* --- special navigation / function keys (0x90+) --- */
    switch (key) {
        case KEY_RETURN:      *hid = 0x28; return true;
        case KEY_ESC:         *hid = 0x29; return true;
        case KEY_BACKSPACE:   *hid = 0x2a; return true;
        case KEY_TAB:         *hid = 0x2b; return true;
        case KEY_INSERT:      *hid = 0x49; return true;
        case KEY_DELETE:      *hid = 0x4c; return true;
        case KEY_HOME:        *hid = 0x4a; return true;
        case KEY_END:         *hid = 0x4d; return true;
        case KEY_PAGE_UP:     *hid = 0x4b; return true;
        case KEY_PAGE_DOWN:   *hid = 0x4e; return true;
        case KEY_RIGHT_ARROW: *hid = 0x4f; return true;
        case KEY_LEFT_ARROW:  *hid = 0x50; return true;
        case KEY_DOWN_ARROW:  *hid = 0x51; return true;
        case KEY_UP_ARROW:    *hid = 0x52; return true;
        case KEY_CAPS_LOCK:   *hid = 0x39; return true;
        case KEY_NUM_LOCK:    *hid = 0x53; return true;
        case KEY_PRINT_SCREEN:*hid = 0x46; return true;
        case KEY_F1:  *hid = 0x3a; return true;
        case KEY_F2:  *hid = 0x3b; return true;
        case KEY_F3:  *hid = 0x3c; return true;
        case KEY_F4:  *hid = 0x3d; return true;
        case KEY_F5:  *hid = 0x3e; return true;
        case KEY_F6:  *hid = 0x3f; return true;
        case KEY_F7:  *hid = 0x40; return true;
        case KEY_F8:  *hid = 0x41; return true;
        case KEY_F9:  *hid = 0x42; return true;
        case KEY_F10: *hid = 0x43; return true;
        case KEY_F11: *hid = 0x44; return true;
        case KEY_F12: *hid = 0x45; return true;
    }

    /* --- printable ASCII --- */
    if (key >= 'a' && key <= 'z') { *hid = (uint8_t)(key - 'a' + 0x04); return true; }
    if (key >= 'A' && key <= 'Z') { *modBit = KEY_MOD_LSHIFT; *hid = (uint8_t)(key - 'A' + 0x04); return true; }
    if (key >= '1' && key <= '9') { *hid = (uint8_t)(key - '1' + 0x1e); return true; }
    if (key == '0')  { *hid = 0x27; return true; }
    if (key == '\n' || key == '\r') { *hid = 0x28; return true; }
    if (key == '\t') { *hid = 0x2b; return true; }
    if (key == ' ')  { *hid = 0x2c; return true; }

    switch (key) {
        case '!': *modBit = KEY_MOD_LSHIFT; *hid = 0x1e; return true;
        case '@': *modBit = KEY_MOD_LSHIFT; *hid = 0x1f; return true;
        case '#': *modBit = KEY_MOD_LSHIFT; *hid = 0x20; return true;
        case '$': *modBit = KEY_MOD_LSHIFT; *hid = 0x21; return true;
        case '%': *modBit = KEY_MOD_LSHIFT; *hid = 0x22; return true;
        case '^': *modBit = KEY_MOD_LSHIFT; *hid = 0x23; return true;
        case '&': *modBit = KEY_MOD_LSHIFT; *hid = 0x24; return true;
        case '*': *modBit = KEY_MOD_LSHIFT; *hid = 0x25; return true;
        case '(': *modBit = KEY_MOD_LSHIFT; *hid = 0x26; return true;
        case ')': *modBit = KEY_MOD_LSHIFT; *hid = 0x27; return true;
        case '-': *hid = 0x2d; return true;
        case '_': *modBit = KEY_MOD_LSHIFT; *hid = 0x2d; return true;
        case '=': *hid = 0x2e; return true;
        case '+': *modBit = KEY_MOD_LSHIFT; *hid = 0x2e; return true;
        case '[': *hid = 0x2f; return true;
        case '{': *modBit = KEY_MOD_LSHIFT; *hid = 0x2f; return true;
        case ']': *hid = 0x30; return true;
        case '}': *modBit = KEY_MOD_LSHIFT; *hid = 0x30; return true;
        case '\\':*hid = 0x31; return true;
        case '|': *modBit = KEY_MOD_LSHIFT; *hid = 0x31; return true;
        case ';': *hid = 0x33; return true;
        case ':': *modBit = KEY_MOD_LSHIFT; *hid = 0x33; return true;
        case '\'':*hid = 0x34; return true;
        case '"': *modBit = KEY_MOD_LSHIFT; *hid = 0x34; return true;
        case '`': *hid = 0x35; return true;
        case '~': *modBit = KEY_MOD_LSHIFT; *hid = 0x35; return true;
        case ',': *hid = 0x36; return true;
        case '<': *modBit = KEY_MOD_LSHIFT; *hid = 0x36; return true;
        case '.': *hid = 0x37; return true;
        case '>': *modBit = KEY_MOD_LSHIFT; *hid = 0x37; return true;
        case '/': *hid = 0x38; return true;
        case '?': *modBit = KEY_MOD_LSHIFT; *hid = 0x38; return true;
    }

    return false; /* unknown */
}

// -------------------------------------------------------

void KeyboardClass::_sendReport()
{
    uiapkbd_keyboard_set(_modifier,
                         _keys[0], _keys[1], _keys[2],
                         _keys[3], _keys[4], _keys[5]);
}

void KeyboardClass::begin()
{
    _modifier = 0;
    memset(_keys, 0, 6);
    uiapusb_begin();
}

void KeyboardClass::end()
{
    releaseAll();
}

size_t KeyboardClass::press(uint8_t key)
{
    uint8_t hid = 0, modBit = 0;
    if (!_toHID(key, &hid, &modBit)) return 0;

    if (modBit) {
        _modifier |= modBit;
        _sendReport();
        return 1;
    }
    /* find empty slot */
    for (int i = 0; i < 6; i++) {
        if (_keys[i] == 0) {
            _keys[i] = hid;
            _sendReport();
            return 1;
        }
    }
    return 0; /* key buffer full */
}

size_t KeyboardClass::release(uint8_t key)
{
    uint8_t hid = 0, modBit = 0;
    if (!_toHID(key, &hid, &modBit)) return 0;

    if (modBit) {
        _modifier &= (uint8_t)~modBit;
    } else {
        for (int i = 0; i < 6; i++) {
            if (_keys[i] == hid) _keys[i] = 0;
        }
    }
    _sendReport();
    return 1;
}

void KeyboardClass::releaseAll()
{
    _modifier = 0;
    memset(_keys, 0, 6);
    _sendReport();
}

size_t KeyboardClass::write(uint8_t key)
{
    uint8_t hid = 0, modBit = 0;
    if (!_toHID(key, &hid, &modBit)) return 0;

    /* press */
    _modifier = modBit;
    memset(_keys, 0, 6);
    _keys[0] = hid;
    _sendReport();
    delay(20);  /* EP2 ポーリング間隔は 10ms。ポーリングのタイミングは
                 * スケッチの実行と非同期なので、_sendReport() 直後に
                 * ポーリングを逃す恐れがある。20ms 待てば最悪ケースでも
                 * 必ず 1〜2 回のポーリングが key_press を拾う。
                 * 10ms のままだと "ll" ">>>" のような連続する同一文字で
                 * 1文字欠けることがあった。 */

    /* release */
    releaseAll();
    delay(50);  /* ホストが key-up を処理するまで待つ。
                 * 矢印・BackSpace・Enter・Home など特殊キーは
                 * 50ms 未満だと次の key-press を取りこぼす。 */
    return 1;
}

size_t KeyboardClass::write(const uint8_t *buf, size_t len)
{
    size_t n = 0;
    for (size_t i = 0; i < len; i++) n += write(buf[i]);
    return n;
}

void KeyboardClass::print(const char *s)
{
    while (*s) write((uint8_t)*s++);
}

void KeyboardClass::println(const char *s)
{
    print(s);
    write((uint8_t)'\n');
}

KeyboardClass Keyboard;
