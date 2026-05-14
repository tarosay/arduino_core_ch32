#include <ch32fun.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "rv003usb.h"
#include "usb_config.h"
#include "uiapusb.h"

static uint8_t g_uiap_core_inited = 0;
static uint8_t g_uiap_usb_inited  = 0;

/*
 * USB-C CC ピン（CC1=PC4, CC2=PD2）を INPUT_FLOATING にして USB 接続を切断する。
 *
 * 本ボードは USB-C コネクタを採用しており、CC1(PC4)/CC2(PD2) に外付け 5.1kΩ
 * Rd 抵抗が接続されている。GPIO を INPUT_FLOATING にすると Rd が CC ラインから
 * 切り離され、USB-C ホストはデバイス未接続と判断して D+/D- の列挙を試みなくなる。
 *
 * D+ (PD3) も INPUT_FLOATING にし、外付けプルアップ（1.5kΩ to 3V3）が残っても
 * ホスト側では CC ピン未検出のため無視される。
 * D- (PD4) は OUTPUT LOW のままにしておく（任意；SE0 回避のために明示する）。
 */
static void usb_disconnect(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    /* CC1 (PC4) = floating input: CNF=01 MODE=00 → 0x4 */
    GPIOC->CFGLR = (GPIOC->CFGLR & ~(0xfu << (4 * 4)))
                 | (0x4u << (4 * 4));

    /* CC2 (PD2) = floating input,  D+ (PD3) = floating input,
     * D-  (PD4) = 2MHz PP output LOW                          */
    GPIOD->CFGLR = (GPIOD->CFGLR
                  & ~((0xfu << (2 * 4)) | (0xfu << (3 * 4)) | (0xfu << (4 * 4))))
                  | (0x4u << (2 * 4))   /* PD2 CC2: floating input  */
                  | (0x4u << (3 * 4))   /* PD3 D+:  floating input  */
                  | (0x2u << (4 * 4));  /* PD4 D-:  2MHz PP output  */
    GPIOD->BCR = (1u << 4);             /* D- = Low */
}

void uiap_core_begin(void)
{
    if (g_uiap_core_inited) return;
    SystemInit();
    usb_disconnect();
    SysTick->CNT = 0;
    g_uiap_core_inited = 1;
}

void uiapusb_begin(void)
{
    if (g_uiap_usb_inited) return;
    uiap_core_begin();

#ifndef UIAP_NO_USB
    /* CC1 (PC4), CC2 (PD2) を OUTPUT LOW にして外付け Rd を有効化する。
     * USB-C ホストが CC ラインで Rd を検出し、D+/D- の列挙を開始する。 */
    /* CC1 (PC4) = 2MHz PP output LOW */
    GPIOC->CFGLR = (GPIOC->CFGLR & ~(0xfu << (4 * 4))) | (0x2u << (4 * 4));
    GPIOC->BCR = (1u << 4);
    /* CC2 (PD2) = 2MHz PP output LOW */
    GPIOD->CFGLR = (GPIOD->CFGLR & ~(0xfu << (2 * 4))) | (0x2u << (2 * 4));
    GPIOD->BCR = (1u << 2);

    Delay_Ms(1);
    usb_setup();
#endif /* UIAP_NO_USB */

    g_uiap_usb_inited = 1;
}

// ============================================================
#ifdef UIAP_COMPOSITE_HID
// ============================================================
// KBD+Mouse report buffers (written from main, read from USB ISR)
// ============================================================

static volatile uint8_t mouse_report[4]    = {0, 0, 0, 0};
static volatile uint8_t keyboard_report[8] = {0, 0, 0, 0, 0, 0, 0, 0};

#ifdef UIAP_WEBHID
// ============================================================
// WebHID バッファ
// ============================================================
// EP3 IN: UIAPduino → Web (8 bytes)
static volatile uint8_t webhid_tx_buf[8] = {0};
// TX フラグ: main がセット、ISR がクリア (1フラグのみ・競合なし)
static volatile uint8_t webhid_tx_pending = 0;
// Feature Report 受信: Web → UIAPduino (最大 32 bytes)
static volatile uint8_t webhid_rx_buf[32];
static volatile uint8_t webhid_rx_len  = 0;
static volatile uint8_t webhid_rx_ready = 0;
// SET_REPORT が来たことを示すフラグ (ISR セット、main クリア)
static volatile uint8_t webhid_set_report_pending = 0;
// マルチパケット対応: EP0 max packet = 8 bytes なので 32 bytes は 4 分割で届く
static volatile uint8_t webhid_rx_offset       = 0;
static volatile uint8_t webhid_rx_expected_len = 0;
#endif // UIAP_WEBHID

/*
 * rv003usb が RV003USB_HANDLE_IN_REQUEST=1 のとき呼ぶコールバック。
 * endp=1: マウス (4 bytes)
 * endp=2: キーボード (8 bytes)
 * endp=3: WebHID TX (8 bytes)  ← UIAP_WEBHID のとき
 */
void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                 int endp, uint32_t sendtok,
                                 struct rv003usb_internal *ist)
{
    if (endp == 1) {
        /* マウス: 相対移動なので送信後に X/Y/Wheel をクリア */
        uint8_t report[4];
        report[0] = mouse_report[0];
        report[1] = mouse_report[1];
        report[2] = mouse_report[2];
        report[3] = mouse_report[3];
        usb_send_data(report, 4, 0, sendtok);
        mouse_report[1] = 0;
        mouse_report[2] = 0;
        mouse_report[3] = 0;
    } else if (endp == 2) {
        /* キーボード: ホストが解放を確認するまで同じ状態を保持 */
        usb_send_data((uint8_t *)keyboard_report, 8, 0, sendtok);
#ifdef UIAP_WEBHID
    } else if (endp == 3) {
        /* WebHID TX: 新データがあれば送信、なければ NAK */
        if (webhid_tx_pending) {
            usb_send_data((uint8_t *)webhid_tx_buf, 8, 0, sendtok);
            webhid_tx_pending = 0;
        } else {
            usb_send_empty(sendtok);
        }
#endif
    } else {
        usb_send_empty(sendtok);
    }
}

#ifdef UIAP_WEBHID
/*
 * ホストが SET_REPORT (Feature) を送ってきた: バッファ準備
 * lValueLSBIndexMSB: lower byte = reportID, next byte = reportType(3=Feature)
 */
void usb_handle_hid_set_report_start(struct usb_endpoint *e, int reqLen,
                                      uint32_t lValueLSBIndexMSB)
{
    (void)e;
    (void)lValueLSBIndexMSB;
    /* reqLen = wLength (期待するデータ長) を保存し、オフセットをリセット */
    webhid_rx_expected_len = (reqLen > 32) ? 32 : (uint8_t)reqLen;
    webhid_rx_offset       = 0;
    webhid_set_report_pending = 1;
}

/*
 * ホストが GET_REPORT (Feature) を要求: TX バッファを返す
 */
void usb_handle_hid_get_report_start(struct usb_endpoint *e, int reqLen,
                                      uint32_t lValueLSBIndexMSB)
{
    (void)lValueLSBIndexMSB;
    e->opaque   = (uint8_t *)webhid_tx_buf;
    e->max_len  = (reqLen < 8) ? reqLen : 8;
}

/*
 * EP0 OUT データ受信 (Feature Report データ本体)
 * current_endpoint == 0 のとき SET_REPORT のデータ
 */
void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                           uint8_t *data, int len,
                           struct rv003usb_internal *ist)
{
    (void)e; (void)ist;
    if (current_endpoint == 0 && webhid_set_report_pending)
    {
        /* EP0 max packet = 8 bytes のため、16 byte Feature Report は
         * 2 回に分けて届く。オフセットを使って全パケットを集約する。 */
        uint8_t space = webhid_rx_expected_len - webhid_rx_offset;
        uint8_t copy_len = ((uint8_t)len > space) ? space : (uint8_t)len;
        for (int i = 0; i < copy_len; i++)
            webhid_rx_buf[webhid_rx_offset + i] = data[i];
        webhid_rx_offset += copy_len;

        if (webhid_rx_offset >= webhid_rx_expected_len)
        {
            /* 全データ受信完了 */
            webhid_set_report_pending = 0;
            webhid_rx_len   = webhid_rx_expected_len;
            webhid_rx_ready = 1;
        }
        /* 未完了の場合は webhid_set_report_pending を維持し次パケットを待つ */
    }
}

/* ---- メイン側 API ---- */

void uiapwebhid_send(const uint8_t *buf, uint8_t len)
{
    /* 前の送信が完了するまで待つ（ホスト未接続時のハング防止のためタイムアウト付き） */
    for (uint32_t t = 0; webhid_tx_pending && t < 200000UL; t++) {}
    if (len > 8) len = 8;
    for (int i = 0; i < len; i++) webhid_tx_buf[i] = buf[i];
    for (int i = len; i < 8;  i++) webhid_tx_buf[i] = 0;
    webhid_tx_pending = 1;
}

uint8_t uiapwebhid_tx_busy(void)
{
    return webhid_tx_pending;
}

uint8_t uiapwebhid_available(void)
{
    return webhid_rx_ready;
}

uint8_t uiapwebhid_recv(uint8_t *buf, uint8_t maxlen)
{
    if (!webhid_rx_ready) return 0;
    uint8_t len = (webhid_rx_len < maxlen) ? webhid_rx_len : maxlen;
    for (int i = 0; i < len; i++) buf[i] = webhid_rx_buf[i];
    webhid_rx_ready = 0;
    return len;
}
#endif // UIAP_WEBHID

void uiapkbd_mouse_set(uint8_t buttons, int8_t x, int8_t y, int8_t wheel)
{
    mouse_report[0] = buttons;
    mouse_report[1] = (uint8_t)x;
    mouse_report[2] = (uint8_t)y;
    mouse_report[3] = (uint8_t)wheel;
}

void uiapkbd_keyboard_set(uint8_t modifier,
                           uint8_t k0, uint8_t k1, uint8_t k2,
                           uint8_t k3, uint8_t k4, uint8_t k5)
{
    keyboard_report[0] = modifier;
    keyboard_report[1] = 0; /* reserved */
    keyboard_report[2] = k0;
    keyboard_report[3] = k1;
    keyboard_report[4] = k2;
    keyboard_report[5] = k3;
    keyboard_report[6] = k4;
    keyboard_report[7] = k5;
}

void uiapkbd_keyboard_clear(void)
{
    memset((void *)keyboard_report, 0, 8);
}

// ============================================================
#elif defined(UIAP_WEBHID_ONLY) \
   || (!defined(UIAP_NO_USB) && !defined(UIAP_USB_TERMINAL))
// ============================================================
// WebHID-only mode: EP1 IN (UIAPduino→Web) + EP0 Feature (Web→UIAPduino)
//
// このブロックは以下の2つのケースをカバーする:
//   1. UIAP_WEBHID_ONLY が定義されている場合
//      → boards.txt: Tools > USB > WebHID Only (-DUIAP_WEBHID_ONLY)
//   2. 上記に加え、他のどのモードフラグも定義されていない場合（安全網）
//      → UIAP_COMPOSITE_HID / UIAP_NO_USB / UIAP_USB_TERMINAL のいずれでもない
//      → 将来新しいモードを追加する際は、そのフラグをこの条件に明示的に追加すること
//         例: || (!defined(UIAP_NO_USB) && !defined(UIAP_USB_TERMINAL) && !defined(UIAP_NEW_MODE))
// ============================================================

static volatile uint8_t webhid_tx_buf[8]        = {0};
static volatile uint8_t webhid_tx_pending        = 0;
static volatile uint8_t webhid_rx_buf[32];
static volatile uint8_t webhid_rx_len            = 0;
static volatile uint8_t webhid_rx_ready          = 0;
static volatile uint8_t webhid_set_report_pending = 0;
static volatile uint8_t webhid_rx_offset         = 0;
static volatile uint8_t webhid_rx_expected_len   = 0;

void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                 int endp, uint32_t sendtok,
                                 struct rv003usb_internal *ist)
{
    if (endp == 1) {
        if (webhid_tx_pending) {
            usb_send_data((uint8_t *)webhid_tx_buf, 8, 0, sendtok);
            webhid_tx_pending = 0;
        } else {
            usb_send_empty(sendtok);
        }
    } else {
        usb_send_empty(sendtok);
    }
}

void usb_handle_hid_set_report_start(struct usb_endpoint *e, int reqLen,
                                      uint32_t lValueLSBIndexMSB)
{
    (void)e; (void)lValueLSBIndexMSB;
    webhid_rx_expected_len = (reqLen > 32) ? 32 : (uint8_t)reqLen;
    webhid_rx_offset       = 0;
    webhid_set_report_pending = 1;
}

void usb_handle_hid_get_report_start(struct usb_endpoint *e, int reqLen,
                                      uint32_t lValueLSBIndexMSB)
{
    (void)lValueLSBIndexMSB;
    e->opaque  = (uint8_t *)webhid_tx_buf;
    e->max_len = (reqLen < 8) ? reqLen : 8;
}

void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                           uint8_t *data, int len,
                           struct rv003usb_internal *ist)
{
    (void)e; (void)ist;
    if (current_endpoint == 0 && webhid_set_report_pending) {
        uint8_t space    = webhid_rx_expected_len - webhid_rx_offset;
        uint8_t copy_len = ((uint8_t)len > space) ? space : (uint8_t)len;
        for (int i = 0; i < copy_len; i++)
            webhid_rx_buf[webhid_rx_offset + i] = data[i];
        webhid_rx_offset += copy_len;
        if (webhid_rx_offset >= webhid_rx_expected_len) {
            webhid_set_report_pending = 0;
            webhid_rx_len   = webhid_rx_expected_len;
            webhid_rx_ready = 1;
        }
    }
}

void uiapwebhid_send(const uint8_t *buf, uint8_t len)
{
    /* ホスト未接続時のハング防止のためタイムアウト付き */
    for (uint32_t t = 0; webhid_tx_pending && t < 200000UL; t++) {}
    if (len > 8) len = 8;
    for (int i = 0; i < len; i++) webhid_tx_buf[i] = buf[i];
    for (int i = len; i < 8;  i++) webhid_tx_buf[i] = 0;
    webhid_tx_pending = 1;
}

uint8_t uiapwebhid_tx_busy(void)   { return webhid_tx_pending; }
uint8_t uiapwebhid_available(void) { return webhid_rx_ready; }

uint8_t uiapwebhid_recv(uint8_t *buf, uint8_t maxlen)
{
    if (!webhid_rx_ready) return 0;
    uint8_t len = (webhid_rx_len < maxlen) ? webhid_rx_len : maxlen;
    for (int i = 0; i < len; i++) buf[i] = webhid_rx_buf[i];
    webhid_rx_ready = 0;
    return len;
}

// ============================================================
#elif defined(UIAP_NO_USB)
// ============================================================
// No USB mode: no callbacks, no USB stack beyond disconnect
// ============================================================

// ============================================================
#elif defined(UIAP_USB_TERMINAL)
// ============================================================

#define UIAPUSB_RX_BUF_SIZE 256

static volatile uint8_t  rxbuf[UIAPUSB_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

static inline uint16_t rb_next(uint16_t v)
{
    return (uint16_t)((v + 1) % UIAPUSB_RX_BUF_SIZE);
}

static inline int rb_count(void)
{
    if (rx_head >= rx_tail) return (int)(rx_head - rx_tail);
    return (int)(UIAPUSB_RX_BUF_SIZE - rx_tail + rx_head);
}

/*
 * demo_terminal.c と同じ受信入口
 * WebLink タブから来たデータはここに入る
 */
void handle_debug_input(int numbytes, uint8_t *data)
{
    for (int i = 0; i < numbytes; i++)
    {
        uint16_t n = rb_next(rx_head);
        if (n == rx_tail)
        {
            break; /* overflow したら捨てる */
        }
        rxbuf[rx_head] = data[i];
        rx_head = n;
    }
}

int uiapusb_available(void)
{
    poll_input();
    return rb_count();
}

int uiapusb_read(uint8_t *buf, int maxlen)
{
    poll_input();
    int n = 0;
    while (n < maxlen && rx_tail != rx_head)
    {
        buf[n++] = rxbuf[rx_tail];
        rx_tail  = rb_next(rx_tail);
    }
    return n;
}

int uiapusb_write(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        putchar(buf[i]);
    }
    return len;
}

#endif // USB mode selection
