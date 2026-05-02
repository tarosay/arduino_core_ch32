#ifndef _UIAPUSB_H
#define _UIAPUSB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 共通: コア初期化・USB 開始 */
void uiap_core_begin(void);
void uiapusb_begin(void);

#ifdef UIAP_COMPOSITE_HID
/* KBD+Mouse モード: レポートセッター */
void uiapkbd_mouse_set(uint8_t buttons, int8_t x, int8_t y, int8_t wheel);
void uiapkbd_keyboard_set(uint8_t modifier,
                           uint8_t k0, uint8_t k1, uint8_t k2,
                           uint8_t k3, uint8_t k4, uint8_t k5);
void uiapkbd_keyboard_clear(void);
#else
/* ターミナルモード: 送受信 */
int  uiapusb_available(void);
int  uiapusb_read(uint8_t *buf, int maxlen);
int  uiapusb_write(const uint8_t *buf, int len);
#endif

/* WebHID モード:
 *   EP3 双方向通信 (UIAP_COMPOSITE_HID + UIAP_WEBHID)
 *   EP1 双方向通信 (UIAP_WEBHID_ONLY)                 */
#if defined(UIAP_WEBHID) || defined(UIAP_WEBHID_ONLY)
void    uiapwebhid_send(const uint8_t *buf, uint8_t len);
uint8_t uiapwebhid_tx_busy(void);
uint8_t uiapwebhid_available(void);
uint8_t uiapwebhid_recv(uint8_t *buf, uint8_t maxlen);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _UIAPUSB_H */
